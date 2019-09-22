extern "C"
{
    #include "ntddk.h"
}

#include "ntddkbd.h"

typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT pLowerDO;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

int gnRequests;

NTSTATUS DispatchThru(PDEVICE_OBJECT theDeviceObject, PIRP theIrp)
{
    IoSkipCurrentIrpStackLocation(theIrp);
    return IoCallDriver(((PDEVICE_EXTENSION) theDeviceObject->DeviceExtension)->pLowerDO, theIrp);
}

NTSTATUS InstallFilter(IN PDRIVER_OBJECT theDO)
{
    PDEVICE_OBJECT pKeyboardDevice;
    NTSTATUS status = {0};

    status = IoCreateDevice(theDO, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_KEYBOARD, 0, FALSE, 
                            &pKeyboardDevice);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("IoCreate device error.");
        return status;
    }

    pKeyboardDevice->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE) & ~DO_DEVICE_INITIALIZING;
    RtlZeroMemory(pKeyboardDevice->DeviceExtension, sizeof(DEVICE_EXTENSION));

    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) pKeyboardDevice->DeviceExtension;
    UNICODE_STRING ustrDeviceName;
    RtlInitUnicodeString(&ustrDeviceName, L"\\Device\\KeyboardClass0");
    IoAttachDevice(pKeyboardDevice, &ustrDeviceName, &pdx->pLowerDO);
    DbgPrint("After IoAttach.");

    return status;
}

NTSTATUS ReadCompletionRoutine(IN PDEVICE_OBJECT pDeviceObject, IN PIRP irp, IN PVOID context)
{
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;
    PKEYBOARD_INPUT_DATA kidData;

    if (NT_SUCCESS(irp->IoStatus.Status))
    {
        kidData = (PKEYBOARD_INPUT_DATA) irp->AssociatedIrp.SystemBuffer;
        int n = irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

        for (int i = 0; i < n; ++i)
        {
            DbgPrint("Code %x\n", kidData[i].MakeCode);
        }
    }

    if (irp->PendingReturned)
        IoMarkIrpPending(irp);

    __asm
    {
        lock dec gnRequests;
    }

    return irp->IoStatus.Status;
}

NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
    __asm
    {
        lock inc gnRequests
    }
    IoCopyCurrentIrpStackLocationToNext(pIrp);
    IoSetCompletionRoutine(pIrp, ReadCompletionRoutine, pDeviceObject, TRUE, TRUE, TRUE);
    return IoCallDriver(((PDEVICE_EXTENSION) pDeviceObject->DeviceExtension)->pLowerDO, pIrp);
}

VOID DriverUnload(IN PDRIVER_OBJECT theDO)
{
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) theDO->DeviceObject->DeviceObjectExtension;
    IoDetachDevice(pdx->pLowerDO);
    IoDeleteDevice(theDO->DeviceObject);

    if (gnRequests != 0)
    {
        KTIMER ktTimer;
        LARGE_INTEGER liTimeout;
        liTimeout.QuadPart = 1000000;
        KeInitializeTimer(&ktTimer);

        while (gnRequests > 0)
        {
            KeSetTimer(&ktTimer, liTimeout, NULL);
            KeWaitForSingleObject(&ktTimer, Executive, KernelMode, FALSE, NULL);
        }
    }
}

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING ustrRegistryPath)
{
    gnRequests = 0;
    NTSTATUS status = 0;

    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
    {
        theDriverObject->MajorFunction[i] = DispatchThru;
    }
    theDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
    theDriverObject->DriverUnload = DriverUnload;

    status = InstallFilter(theDriverObject);
    return status;
}


