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

const UINT32 MY_EDR_FILTER_CONTEXT_POOL_TAG = 'cfem';
const size_t MAX_EVENT_QUEUE_ENTRY_COUNT = 10000;
const MyEdrVersion VERSION = { 1, 0, 0 };

NTSTATUS CreateProcessNotifyRoutineDeleter(PCREATE_PROCESS_NOTIFY_ROUTINE_EX processNotifyRoutine)
{
    return PsSetCreateProcessNotifyRoutineEx(processNotifyRoutine, TRUE);
}

struct MyEdrData final
{
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
    AutoDeletedPointer<
        DEVICE_OBJECT,
        decltype(IoDeleteDevice),
        IoDeleteDevice
    > Device;
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

    AutoDeletedPointer<MyEdrEvent> myEdrEvent = MyEdrCreateEvent(eventId, processId, &nameInfo->Name);
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

void CompleteRequest(PIRP irp)
{
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

NTSTATUS MyEdrCreateClose(
    PDEVICE_OBJECT deviceObject,
    PIRP irp
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS MyEdrDeviceControl(
    PDEVICE_OBJECT deviceObject,
    PIRP irp
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    AutoDeletedPointer<IRP, decltype(CompleteRequest), CompleteRequest> request;

    const PIO_STACK_LOCATION irpStackLocation = IoGetCurrentIrpStackLocation(irp);

    PVOID buffer = irp->AssociatedIrp.SystemBuffer;
    RETURN_ON_CONDITION(nullptr == buffer, STATUS_INVALID_PARAMETER);

    switch (irpStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
    case SYSCTL_GET_VERSION:
    {
        RETURN_ON_CONDITION(sizeof(MyEdrVersion) > irpStackLocation->Parameters.DeviceIoControl.OutputBufferLength, STATUS_INVALID_PARAMETER);
        RtlCopyMemory(buffer, &VERSION, sizeof(VERSION));
        break;
    }
    case SYSCTL_GET_EVENTS:
    {
        RETURN_ON_CONDITION(sizeof(MyEdrEvent) > irpStackLocation->Parameters.DeviceIoControl.OutputBufferLength, STATUS_INVALID_PARAMETER);
        Result<AutoDeletedPointer<MyEdrEvent>> myEdrEvent = g_myEdrData->EventQueue.popHead();
        RETURN_STATUS_ON_BAD_STATUS(myEdrEvent.getStatus());
        RtlCopyMemory(buffer, &*myEdrEvent, sizeof(MyEdrEvent));
        break;
    }
    case SYSCTL_ADD_BLACK:
    {
        RETURN_ON_CONDITION(sizeof(MyEdrBlacklistProcess) > irpStackLocation->Parameters.DeviceIoControl.InputBufferLength, STATUS_INVALID_PARAMETER);
        g_myEdrData->BlacklistProcessIds.insertElement(static_cast<MyEdrBlacklistProcess*>(buffer)->ProcessId);
        break;
    }
    default:
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    }

    return STATUS_SUCCESS;
}

void DriverUnload(
    DRIVER_OBJECT* driverObject
)
{
    UNREFERENCED_PARAMETER(driverObject);

    delete g_myEdrData;
}

extern "C" NTSTATUS DriverEntry(
    PDRIVER_OBJECT driverObject,
    PUNICODE_STRING registryPath
)
{
    UNREFERENCED_PARAMETER(driverObject);
    UNREFERENCED_PARAMETER(registryPath);

    AutoDeletedPointer<MyEdrData> myEdrData = new MyEdrData {
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
        { FLT_STREAMHANDLE_CONTEXT, 0, nullptr, 0, MY_EDR_FILTER_CONTEXT_POOL_TAG },
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
        driverObject,
        &FilterRegistration,
        &myEdrData->Filter.get()
    ));

    RETURN_STATUS_ON_BAD_STATUS(FltStartFiltering(myEdrData->Filter.get()));

    UNICODE_STRING deviceName;
    RtlInitUnicodeString(&deviceName, MY_EDR_DEVICE_NAME);
    RETURN_STATUS_ON_BAD_STATUS(IoCreateDevice(
        driverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &myEdrData->Device.get()
    ));

    driverObject->DriverUnload = DriverUnload;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = driverObject->MajorFunction[IRP_MJ_CREATE] = MyEdrCreateClose;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyEdrDeviceControl;

    myEdrData.release();

    return STATUS_SUCCESS;
}
