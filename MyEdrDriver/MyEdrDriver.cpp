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

struct MyEdrData final
{
    PDRIVER_OBJECT DriverObject;
    Queue<AutoDeletedPointer<MyEdrEvent>> EventQueue;
    AvlTable<ULONG> BlacklistProcessIds;
    Mutex Mutex;
    AutoDeletedPointer<remove_reference_t<PCREATE_PROCESS_NOTIFY_ROUTINE_EX>> ProcessNotifyRoutine;
    AutoDeletedPointer<remove_reference_t<PFLT_FILTER>> Filter;
};

MyEdrData* g_myEdrData{ nullptr };

void MyEdrProcessNotifyRoutine(
    PEPROCESS process,
    HANDLE processId,
    PPS_CREATE_NOTIFY_INFO createInfo
)
{
    UNREFERENCED_PARAMETER(process);
    UNREFERENCED_PARAMETER(processId);
    UNREFERENCED_PARAMETER(createInfo);

    DEBUG_PRINT("%p %p %wZ", process, processId, nullptr == createInfo ? UNICODE_STRING{ 0, 0 } : *createInfo->ImageFileName);
}

FLT_POSTOP_CALLBACK_STATUS HandleFilterCallback(
    MyEdrEventId eventId,
    PFLT_CALLBACK_DATA data
)
{
    RETURN_ON_BAD_STATUS(data->IoStatus.Status, FLT_POSTOP_FINISHED_PROCESSING);
    RETURN_ON_CONDITION(STATUS_REPARSE == data->IoStatus.Status, FLT_POSTOP_FINISHED_PROCESSING);

    AutoDeletedPointer<FLT_FILE_NAME_INFORMATION> nameInfo{ nullptr, FltReleaseFileNameInformation };

    RETURN_ON_BAD_STATUS(
        FltGetFileNameInformation(
            data,
            FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
            &nameInfo.get()
        ),
        FLT_POSTOP_FINISHED_PROCESSING
    );

    AutoDeletedPointer<MyEdrEvent> event;
    RETURN_ON_BAD_STATUS(event.allocate(), FLT_POSTOP_FINISHED_PROCESSING);

    event->Id = eventId;
    event->ProcessId = FltGetRequestorProcessId(data);
    KeQuerySystemTime(&event->TimeStamp);
    UNICODE_STRING temp{ 0, sizeof(event->Name) - sizeof(WCHAR), event->Name };
    RtlCopyUnicodeString(&temp, &nameInfo->Name);
    event->Name[temp.Length / sizeof(WCHAR)] = '\0';

    {
        Lock lock(g_myEdrData->Mutex);

        if (g_myEdrData->EventQueue.isFull())
        {
            g_myEdrData->EventQueue.popHead();
        }

        RETURN_ON_BAD_RESULT(g_myEdrData->EventQueue.pushTail(move(event)), FLT_POSTOP_FINISHED_PROCESSING);
    }

    event.release();

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

    AutoDeletedPointer<MyEdrData> myEdrData{ new MyEdrData{
        DriverObject,
        { MAX_EVENT_QUEUE_ENTRY_COUNT },
        {},
        {},
        { nullptr, [](PCREATE_PROCESS_NOTIFY_ROUTINE_EX processNotifyRoutine) { PsSetCreateProcessNotifyRoutineEx(processNotifyRoutine, TRUE); } },
        { nullptr, FltUnregisterFilter }
    } };

    if (nullptr == myEdrData.get())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RETURN_STATUS_ON_BAD_STATUS(PsSetCreateProcessNotifyRoutineEx(MyEdrProcessNotifyRoutine, FALSE));
    g_myEdrData->ProcessNotifyRoutine = MyEdrProcessNotifyRoutine;

    const FLT_OPERATION_REGISTRATION FilterOperationRegistration[] = \
    {
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

    g_myEdrData = myEdrData.release();
    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}
