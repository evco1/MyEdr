#include <fltKernel.h>

#include "MyEdrEvent.h"
#include "Queue.h"
#include "Result.h"
#include "AutoDeletedPointer.h"
#include "Statuses.h"
#include "Debug.h"

#include <wdm.h>

const UINT32 CONTEXT_POOL_TAG = 'chem';
const size_t MAX_EVENT_QUEUE_ENTRY_COUNT = 10000;

struct MyEdrData final
{
    PDRIVER_OBJECT DriverObject;
    PFLT_FILTER Filter;
    Queue<AutoDeletedPointer<MyEdrEvent>> EventQueue;
};

MyEdrData* g_myEdrData{ nullptr };

FLT_POSTOP_CALLBACK_STATUS HandleFilterCallback(
    MyEdrEventId EventId,
    PFLT_CALLBACK_DATA Data
)
{
    RETURN_ON_CONDITION(
        !NT_SUCCESS(Data->IoStatus.Status) || (STATUS_REPARSE == Data->IoStatus.Status),
        FLT_POSTOP_FINISHED_PROCESSING
    );

    PFLT_FILE_NAME_INFORMATION nameInfo;

    RETURN_ON_BAD_STATUS(
        FltGetFileNameInformation(
            Data,
            FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
            &nameInfo
        ),
        FLT_POSTOP_FINISHED_PROCESSING
    );

    AutoDeletedPointer<MyEdrEvent> event;
    
    event.allocate();
    *event = {
        EventId,
        FltGetRequestorProcessId(Data)
    };

    KeQuerySystemTime(&event->TimeStamp);
    UNICODE_STRING temp{ 0, MAX_PATH_SIZE - 1, event->Name };
    RtlCopyUnicodeString(&temp, &nameInfo->Name);
    FltReleaseFileNameInformation(nameInfo);
    event->Name[temp.Length] = '\0';

    if (g_myEdrData->EventQueue.isFull())
    {
        g_myEdrData->EventQueue.popHead();
    }

    DEBUG_PRINT("%u, %u, %llu, %wZ", event->Id, event->ProcessId, event->TimeStamp, temp);

    g_myEdrData->EventQueue.pushTail(move(event));

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

    FltUnregisterFilter(g_myEdrData->Filter);
    delete g_myEdrData;
}

extern "C" NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    AutoDeletedPointer<MyEdrData> myEdrData{ new MyEdrData{ nullptr, nullptr, { MAX_EVENT_QUEUE_ENTRY_COUNT } } };

    if (nullptr == myEdrData.get())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    const FLT_OPERATION_REGISTRATION FilterOperationRegistration[] = \
    {
        { IRP_MJ_CREATE, 0, nullptr, MyEdrPostCreate },
        { IRP_MJ_WRITE, 0, nullptr, MyEdrPostWrite },
        { IRP_MJ_OPERATION_END }
    };

    const FLT_CONTEXT_REGISTRATION FilterContextRegistration[] = {
        { FLT_STREAMHANDLE_CONTEXT,
          0,
          nullptr,
          0,
          CONTEXT_POOL_TAG },
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
        &myEdrData->Filter
    ));

    RETURN_STATUS_ON_BAD_STATUS(FltStartFiltering(myEdrData->Filter));

    g_myEdrData = myEdrData.release();
    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}
