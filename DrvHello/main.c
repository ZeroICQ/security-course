#include "ntddk.h"

#pragma code_seg("INIT")
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    // int a = *((int *)0);
    DbgPrint("Hello, world!");                  // Выдача отладочного сообщения
    return STATUS_DEVICE_CONFIGURATION_ERROR;   // Выдача ошибки заставит систему сразу же выгрузить драйвер
}
#pragma code_seg()
