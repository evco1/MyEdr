#pragma once

#include <fltKernel.h>

struct MY_EDR_DATA {
    PDRIVER_OBJECT DriverObject;
    PFLT_FILTER Filter;
};

FLT_POSTOP_CALLBACK_STATUS MyEdrPostCreate(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
);

FLT_POSTOP_CALLBACK_STATUS MyEdrPostWrite(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
);
