#include <fltKernel.h>

PFLT_FILTER FilterHandle = NULL;
PFLT_PORT serverport = NULL;
PFLT_PORT clientPort;
FLT_PREOP_CALLBACK_STATUS createPreOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext
);
NTSTATUS MessageNotifyCallback(
    PVOID PortCookie,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnOutputBufferLength
);
FLT_POSTOP_CALLBACK_STATUS createPostOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

NTSTATUS UnloadFilter(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS ConnectNotifyCallback(
    PFLT_PORT ClientPort,
    PVOID ServerPortCookie,
    PVOID ConnectionContext,
    ULONG SizeOfContext,
    PVOID* ConnectionPortCookie
);
VOID DisconnectNotifyCallback(
    PVOID ConnectionCookie
);
UNICODE_STRING portName = RTL_CONSTANT_STRING(L"\\mavport");
// callbacks
FLT_OPERATION_REGISTRATION Callbacks[] = {
    {IRP_MJ_CREATE, 0, createPreOp, createPostOp},
    {IRP_MJ_OPERATION_END}
};
OBJECT_ATTRIBUTES oa;
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
    if (!NT_SUCCESS(status)) {
        return status;
    }

    InitializeObjectAttributes(&oa, &portName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = FltCreateCommunicationPort(
        FilterHandle,
        &serverport,
        &oa,
        NULL,
        ConnectNotifyCallback,
        DisconnectNotifyCallback,
        MessageNotifyCallback,
        1
    );

    if (!NT_SUCCESS(status)) {
        FltUnregisterFilter(FilterHandle);
        return status;
    }

    status = FltStartFiltering(FilterHandle);
    if (!NT_SUCCESS(status)) {
        FltCloseCommunicationPort(serverport);
        FltUnregisterFilter(FilterHandle);
        return status;
    }

    KdPrint(("Driver loaded\n"));
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
    typedef struct _KERNEL_MESSAGE_
    {
        FILTER_MESSAGE_HEADER header;
        WCHAR msg[1024];
    }KERNEL_MESSAGE;
    typedef struct _USER_MESSAGE_ {
        FILTER_REPLY_HEADER header;
        BOOLEAN isMalware;
    }USER_MESSAGE;

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION FileNameInformation = NULL;
    PFLT_VOLUME RetVolume = FltObjects->Volume;
    PDEVICE_OBJECT DiskDeviceObject = NULL;
    UNICODE_STRING Dos = { 0 };


    if (!Data->Iopb || !Data->Iopb->TargetFileObject || !Data->Iopb->TargetFileObject->DeviceObject) /*Handling a rare edge case in which data is null*/
        return FLT_PREOP_SUCCESS_NO_CALLBACK;


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

    if (wcsstr(FileNameInformation->Name.Buffer, L"\\Windows\\System32") != NULL) {
        goto Cleanup;
    }
    UNICODE_STRING extension;
    RtlInitUnicodeString(&extension, L".exe");
    /*if (!RtlSuffixUnicodeString(&FileNameInformation->Name, &extension, TRUE)) {
        goto Cleanup; // not an .exe file
    }*/
    if (wcsstr(FileNameInformation->Name.Buffer, L".exe") == NULL) {
        goto Cleanup;
    }
    KdPrint(("%wZ",&FileNameInformation->Name));
    KERNEL_MESSAGE kernelMsg = { 0 };
    USER_MESSAGE userMsg = { 0 };
    ULONG replyLength = sizeof(USER_MESSAGE);
    wcsncpy(kernelMsg.msg, fullPath.Buffer, 1023); //copied fullPath.buffer into kernelMsg.msg
    kernelMsg.header.ReplyLength = sizeof(USER_MESSAGE);
    kernelMsg.msg[1023] = L'\0';
    status = FltSendMessage(FilterHandle, &clientPort, &kernelMsg, sizeof(kernelMsg), &userMsg, &replyLength, NULL);
    if (!NT_SUCCESS(status)) goto Cleanup;
    if (userMsg.isMalware) {
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
        KdPrint(("Malware Deleted"));
        return FLT_PREOP_COMPLETE;
    }
    else
    {
        goto Cleanup;
    }
    /*Cleanup*/
    Cleanup:
        if (FileNameInformation) FltReleaseFileNameInformation(FileNameInformation);
        if (Dos.Buffer) ExFreePoolWithTag(Dos.Buffer, 'TDnI');
        if (DiskDeviceObject) ObDereferenceObject(DiskDeviceObject);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
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
/*    typedef struct _KERNEL_MESSAGE_
    {
        WCHAR msg[1024];
    }KERNEL_MESSAGE;
    typedef struct _USER_MESSAGE_ {
        BOOLEAN isMalware;
    }USER_MESSAGE;

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION FileNameInformation = NULL;
    PFLT_VOLUME RetVolume = FltObjects->Volume;
    PDEVICE_OBJECT DiskDeviceObject = NULL;
    UNICODE_STRING Dos = { 0 };
   

    if (!Data->Iopb || !Data->Iopb->TargetFileObject || !Data->Iopb->TargetFileObject->DeviceObject) /*Handling a rare edge case in which data is null*//*..
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

    if (wcsstr(FileNameInformation->Name.Buffer, L"\\Windows\\System32") == NULL) {
        goto Cleanup;
    }
    KERNEL_MESSAGE kernelMsg = { 0 };
    USER_MESSAGE userMsg = { 0 };
    ULONG replyLength = sizeof(USER_MESSAGE);
    wcsncpy(kernelMsg.msg, fullPath.Buffer,1023); //copied fullPath.buffer into kernelMsg.msg
    kernelMsg.msg[1023] = L'\0';
    status = FltSendMessage(FilterHandle, &clientPort, &kernelMsg, sizeof(kernelMsg), &userMsg, &replyLength, NULL);
    if (!NT_SUCCESS(status)) goto Cleanup;
    if (userMsg.isMalware) {
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    else
    {
        goto Cleanup;
    }
    /*Cleanup*/
   /*.. Cleanup:
        if (FileNameInformation) FltReleaseFileNameInformation(FileNameInformation);
        if (Dos.Buffer) ExFreePoolWithTag(Dos.Buffer, 'TDnI');
        if (DiskDeviceObject) ObDereferenceObject(DiskDeviceObject);
        return FLT_POSTOP_FINISHED_PROCESSING; */

   
}

NTSTATUS ConnectNotifyCallback(
    PFLT_PORT ClientPort,
    PVOID ServerPortCookie,
    PVOID ConnectionContext,
    ULONG SizeOfContext,
    PVOID* ConnectionPortCookie
)
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    if (clientPort != NULL) {
        FltCloseClientPort(FilterHandle, &clientPort);
        clientPort = NULL;
    }
    clientPort = ClientPort;
    *ConnectionPortCookie = NULL;
    KdPrint(("User-mode connected to communication port\n"));
    return STATUS_SUCCESS;
}
VOID DisconnectNotifyCallback(
    PVOID ConnectionCookie
)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);
    if (clientPort != NULL) {
        FltCloseClientPort(FilterHandle, &clientPort);
        clientPort = NULL;
    }
    KdPrint(("User-mode disconnected from communication port\n"));

}
NTSTATUS MessageNotifyCallback(
    PVOID PortCookie,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnOutputBufferLength
)
{
    UNREFERENCED_PARAMETER(PortCookie);
    if (InputBuffer == NULL || InputBufferLength == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    const char* msg = (CHAR*)InputBuffer;
    KdPrint(("msg %s \n", msg));
    return STATUS_SUCCESS;
}