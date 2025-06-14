#include <fltKernel.h>

PFLT_FILTER FilterHandle = NULL;

FLT_PREOP_CALLBACK_STATUS createPreOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS createPostOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

NTSTATUS UnloadFilter(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

// callbacks
FLT_OPERATION_REGISTRATION Callbacks[] = {
    {IRP_MJ_CREATE, 0, createPreOp, createPostOp},
    {IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegister = {
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    0,
    NULL,
    Callbacks,
    UnloadFilter,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;
    status = FltRegisterFilter(DriverObject, &FilterRegister, &FilterHandle);
    if (NT_SUCCESS(status)) {
        status = FltStartFiltering(FilterHandle);
        if (!NT_SUCCESS(status)) {
            FltUnregisterFilter(FilterHandle);
        }
    }
    return status;
}

NTSTATUS UnloadFilter(_In_ FLT_FILTER_UNLOAD_FLAGS Flags) {
    UNREFERENCED_PARAMETER(Flags);
    FltUnregisterFilter(FilterHandle);
    KdPrint(("Driver Unloaded\n"));
    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS createPreOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext
) {
    UNREFERENCED_PARAMETER(FltObjects); // Avoid warning
    *CompletionContext = NULL; // Fix uninitialized out parameter (C6101)

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION FileNameInformation;

    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInformation);
    if (NT_SUCCESS(status)) {
        status = FltParseFileNameInformation(FileNameInformation);
        if (NT_SUCCESS(status)) {
            KdPrint(("Opened/Created file: %wZ\n", &FileNameInformation->Name));
            FltReleaseFileNameInformation(FileNameInformation);
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
        FltReleaseFileNameInformation(FileNameInformation);
    }
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS createPostOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
) {
    /*unrefereced paramter to fix warning*/
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    return FLT_POSTOP_FINISHED_PROCESSING;
}
