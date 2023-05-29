#include <fltKernel.h>
#include <ntddk.h>

#include "Queue.h"
#include "Result.h"
#include "AutoDeletedPointer.h"
#include "Statuses.h"
#include "Debug.h"
#include "MyEdrEvent.h"
#include "Mutex.h"
#include "Lock.h"
#include "AvlTable.h"

#include <wdm.h>

const UINT32 CONTEXT_POOL_TAG = 'chem';
const size_t MAX_EVENT_QUEUE_ENTRY_COUNT = 10000;

NTSTATUS CreateProcessNotifyRoutineDeleter(PCREATE_PROCESS_NOTIFY_ROUTINE_EX processNotifyRoutine)
{
    return PsSetCreateProcessNotifyRoutineEx(processNotifyRoutine, TRUE);
}

struct MyEdrData final
{
    PDRIVER_OBJECT DriverObject;
    Queue<AutoDeletedPointer<MyEdrEvent>> EventQueue;
    AvlTable<ULONG> BlacklistProcessIds;
    Mutex Mutex;
    AutoDeletedPointer<
        remove_reference_t<PCREATE_PROCESS_NOTIFY_ROUTINE_EX>,
        decltype(CreateProcessNotifyRoutineDeleter),
        CreateProcessNotifyRoutineDeleter
    > CreateProcessNotifyRoutine;
    AutoDeletedPointer<
        remove_reference_t<PFLT_FILTER>,
        decltype(FltUnregisterFilter),
        FltUnregisterFilter
    > Filter;
};

MyEdrData* g_myEdrData{ nullptr };

AutoDeletedPointer<MyEdrEvent> MyEdrCreateEvent(
    const MyEdrEventId eventId,
    const ULONG processId,
    const UNICODE_STRING* name
)
{
    AutoDeletedPointer<MyEdrEvent> myEdrEvent = new MyEdrEvent{
        eventId,
        processId
    };
    RETURN_ON_CONDITION(nullptr == myEdrEvent, {});

    KeQuerySystemTime(&myEdrEvent->TimeStamp);

    UNICODE_STRING copiedName{ 0, sizeof(myEdrEvent->Name) - sizeof(WCHAR), myEdrEvent->Name };
    RtlCopyUnicodeString(&copiedName, name);
    myEdrEvent->Name[copiedName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    return myEdrEvent;
}

NTSTATUS MyEdrPushEventToQueue(AutoDeletedPointer<MyEdrEvent>&& myEdrEvent)
{
    const Lock lock(g_myEdrData->Mutex);

    if (g_myEdrData->EventQueue.isFull()) {
        g_myEdrData->EventQueue.popHead();
    }

    RETURN_STATUS_ON_BAD_STATUS(g_myEdrData->EventQueue.pushTail(move(myEdrEvent)));

    return STATUS_SUCCESS;
}

void MyEdrCreateProcessNotifyRoutine(
    const PEPROCESS process,
    const HANDLE processId,
    const PPS_CREATE_NOTIFY_INFO createInfo
)
{
    UNREFERENCED_PARAMETER(process);

    AutoDeletedPointer<MyEdrEvent> myEdrEvent = MyEdrCreateEvent(
        nullptr == createInfo ? ProcessExit : ProcessCreate,
        static_cast<ULONG>(reinterpret_cast<ULONG64>(processId)),
        nullptr == createInfo ? nullptr : createInfo->ImageFileName
    );

    RETURN_ON_CONDITION(nullptr == myEdrEvent, );

    MyEdrPushEventToQueue(move(myEdrEvent));
    myEdrEvent.release();

    const Lock lock(g_myEdrData->Mutex);
    g_myEdrData->BlacklistProcessIds.insertElement(static_cast<ULONG>(reinterpret_cast<ULONG64>(processId)));
}

FLT_POSTOP_CALLBACK_STATUS HandleFilterCallback(
    const MyEdrEventId eventId,
    const PFLT_CALLBACK_DATA data
)
{
    RETURN_ON_BAD_STATUS(data->IoStatus.Status, FLT_POSTOP_FINISHED_PROCESSING);
    RETURN_ON_CONDITION(STATUS_REPARSE == data->IoStatus.Status, FLT_POSTOP_FINISHED_PROCESSING);

    AutoDeletedPointer<
        FLT_FILE_NAME_INFORMATION,
        decltype(FltReleaseFileNameInformation),
        FltReleaseFileNameInformation
    > nameInfo;

    RETURN_ON_BAD_STATUS(
        FltGetFileNameInformation(
            data,
            FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
            &nameInfo.get()
        ),
        FLT_POSTOP_FINISHED_PROCESSING
    );

    const ULONG processId = FltGetRequestorProcessId(data);

    {
        const Lock lock(g_myEdrData->Mutex);
        RETURN_ON_CONDITION(g_myEdrData->BlacklistProcessIds.containsElement(processId), FLT_POSTOP_FINISHED_PROCESSING);
    }

    auto myEdrEvent = MyEdrCreateEvent(eventId, processId, &nameInfo->Name);
    RETURN_ON_CONDITION(nullptr == myEdrEvent, FLT_POSTOP_FINISHED_PROCESSING);

    MyEdrPushEventToQueue(move(myEdrEvent));
    myEdrEvent.release();

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_POSTOP_CALLBACK_STATUS MyEdrPostCreate(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    return HandleFilterCallback(FileCreate, Data);
}

FLT_POSTOP_CALLBACK_STATUS MyEdrPostWrite(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    return HandleFilterCallback(FileWrite, Data);
}

void DriverUnload(
    DRIVER_OBJECT * DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    delete g_myEdrData;
}

extern "C" NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    AutoDeletedPointer<MyEdrData> myEdrData = new MyEdrData {
        DriverObject,
        { MAX_EVENT_QUEUE_ENTRY_COUNT }
    };

    if (nullptr == myEdrData.get()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // It's very important that g_myEdrData will be already initialized here because from this point,
    // a callback can be called and g_myEdrData is being accessed from the callbacks.
    g_myEdrData = myEdrData.get();

    RETURN_STATUS_ON_BAD_STATUS(PsSetCreateProcessNotifyRoutineEx(MyEdrCreateProcessNotifyRoutine, FALSE));
    myEdrData->CreateProcessNotifyRoutine = MyEdrCreateProcessNotifyRoutine;

    const FLT_OPERATION_REGISTRATION FilterOperationRegistration[] = {
        { IRP_MJ_CREATE, 0, nullptr, MyEdrPostCreate },
        { IRP_MJ_WRITE, 0, nullptr, MyEdrPostWrite },
        { IRP_MJ_OPERATION_END }
    };

    const FLT_CONTEXT_REGISTRATION FilterContextRegistration[] = {
        { FLT_STREAMHANDLE_CONTEXT, 0, nullptr, 0, CONTEXT_POOL_TAG },
        { FLT_CONTEXT_END }
    };

    const FLT_REGISTRATION FilterRegistration = {
        sizeof(FLT_REGISTRATION),
        FLT_REGISTRATION_VERSION,
        0,
        FilterContextRegistration,
        FilterOperationRegistration
    };

    RETURN_STATUS_ON_BAD_STATUS(FltRegisterFilter(
        DriverObject,
        &FilterRegistration,
        &myEdrData->Filter.get()
    ));

    RETURN_STATUS_ON_BAD_STATUS(FltStartFiltering(myEdrData->Filter.get()));
    myEdrData.release();

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}
