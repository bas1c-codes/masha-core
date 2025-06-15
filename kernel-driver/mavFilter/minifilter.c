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
        KdPrint(("Driver loaded"));
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
    
    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION FileNameInformation = NULL;
    PFLT_VOLUME RetVolume = FltObjects->Volume;
    PDEVICE_OBJECT DiskDeviceObject = NULL;
    UNICODE_STRING Dos = { 0 };
   

    if (!Data->Iopb || !Data->Iopb->TargetFileObject || !Data->Iopb->TargetFileObject->DeviceObject) /*Handling a rare edge case in which data is null*/
        return FLT_POSTOP_FINISHED_PROCESSING;
   

    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInformation);

    if (!NT_SUCCESS(status)) goto Cleanup;
    status = FltParseFileNameInformation(FileNameInformation);
    if (!NT_SUCCESS(status)) goto Cleanup;   
    status = FltGetDiskDeviceObject(RetVolume, &DiskDeviceObject);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = IoVolumeDeviceToDosName(DiskDeviceObject, &Dos);
    if (!NT_SUCCESS(status)) goto Cleanup;
    WCHAR fullPathBuffer[1024];
    UNICODE_STRING fullPath;
    fullPath.Buffer = fullPathBuffer;
    fullPath.Length = 0;
    fullPath.MaximumLength = sizeof(fullPathBuffer);

    RtlCopyUnicodeString(&fullPath, &Dos);                          // Copy C:
    RtlAppendUnicodeStringToString(&fullPath, &FileNameInformation->Name);    // Append \Users\file.txt

    KdPrint(("Full File Path: %wZ\n", &fullPath));

    /*Cleanup*/
    Cleanup:
        if (FileNameInformation) FltReleaseFileNameInformation(FileNameInformation);
        if (Dos.Buffer) ExFreePoolWithTag(Dos.Buffer, 'TDnI');
        if (DiskDeviceObject) ObDereferenceObject(DiskDeviceObject);
        return FLT_POSTOP_FINISHED_PROCESSING;
   
}
