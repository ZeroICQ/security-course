#include "precomp.h"

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    NDIS_PROTOCOL_CHARACTERISTICS PChars;
    NDIS_MINIPORT_CHARACTERISTICS MChars;
    PNDIS_CONFIGURATION_PARAMETER Param;

    NDIS_STRING Name;
    NDIS_HANDLE WrapperHandle;
    // ���� ������ ����� ��������� ��� ��������� ������� ��� �����������.
    PDRIVER_DISPATCH MajorFunctions[IRP_MJ_MAXIMUM_FUNCTION + 1];

    NDIS_STRING ntDeviceName; //��� ��� ������ ������������ device-� ��� NT
    NDIS_STRING win32DeviceName; // ���� ��� win32
    PDEVICE_OBJECT deviceObject; // � ������.

    //
    // Register the miniport with NDIS. Note that it is the miniport
    // which was started as a driver and not the protocol. Also the miniport
    // must be registered prior to the protocol since the protocol's BindAdapter
    // handler can be initiated anytime and when it is, it must be ready to
    // start driver instances.
    //

    // ������� ��������� ������� NDIS, ��� ������ ����� ������������ miniport service � �� �������.
    // ������������ �������� ���������� ��������� �� �������.
    // ����������� ���� �������� ��������, ��� ���� ���������� ������ ��� ������������� ������ �������, 
    // �� ��� ��� ��������� ������������ ������� NdisMInitializeWrapper ����� ������� NdisTerminateWrapper ��� ������������� �������.
    NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);
    // function fills a block of memory with zeros.
    NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

    MChars.MajorNdisVersion = 4;
    MChars.MinorNdisVersion = 0;
    MChars.InitializeHandler = MPInitialize;
    MChars.QueryInformationHandler = MPQueryInformation;
    MChars.SetInformationHandler = MPSetInformation;
    MChars.ResetHandler = MPReset;
    MChars.TransferDataHandler = MPTransferData;
    MChars.HaltHandler = MPHalt;

    //
    // We will disable the check for hang timeout so we do not
    // need a check for hang handler!
    //

    MChars.CheckForHangHandler = NULL;
    MChars.SendHandler = MPSend;
    MChars.ReturnPacketHandler = MPReturnPacket;

    //
    // Either the Send or the SendPackets handler should be specified.
    // If SendPackets handler is specified, SendHandler is ignored
    //
    // MChars.SendPacketsHandler = MPSendPackets;
    // �������, �������������� ��� ������� ������ miniport
    Status = NdisIMRegisterLayeredMiniport(WrapperHandle, &MChars, sizeof(MChars), &DriverHandle);
    ASSERT(Status == NDIS_STATUS_SUCCESS);
    // ������ ��� ���������� �������� ������� ������ miniport
    NdisMRegisterUnloadHandler(WrapperHandle, PtUnload);

    //
    // Now register the protocol.
    //

    NdisZeroMemory(&PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    PChars.MajorNdisVersion = 4;
    PChars.MinorNdisVersion = 0;

    //
    // Make sure the protocol-name matches the service-name under which this protocol is installed.
    // This is needed to ensure that NDIS can correctly determine the binding and call us to bind
    // to miniports below.
    //

    NdisInitUnicodeString(&Name, L"SFilter"); // Protocol name
    PChars.Name = Name;
    PChars.OpenAdapterCompleteHandler = PtOpenAdapterComplete;
    PChars.CloseAdapterCompleteHandler = PtCloseAdapterComplete;
    PChars.SendCompleteHandler = PtSendComplete;
    PChars.TransferDataCompleteHandler = PtTransferDataComplete;
    PChars.ResetCompleteHandler = PtResetComplete;
    PChars.RequestCompleteHandler = PtRequestComplete;
    PChars.ReceiveHandler = PtReceive;
    PChars.ReceiveCompleteHandler = PtReceiveComplete;
    PChars.StatusHandler = PtStatus;
    PChars.StatusCompleteHandler = PtStatusComplete;
    PChars.BindAdapterHandler = PtBindAdapter;
    PChars.UnbindAdapterHandler = PtUnbindAdapter;
    PChars.UnloadHandler = NULL;
    PChars.ReceivePacketHandler = PtReceivePacket;
    PChars.PnPEventHandler = PtPNPHandler;
    // �������, �������������� ��� ������� ���������. ������������ ������
    NdisRegisterProtocol(&Status, &ProtHandle, &PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    ASSERT(Status == NDIS_STATUS_SUCCESS);

    // Name of control deviceObject.
    // DeviceName that names the device object.
    NdisInitUnicodeString(&ntDeviceName, L"\Device\passthru");

    // SymbolicName that is the Win32-visible name of the device
    NdisInitUnicodeString(&win32DeviceName, L"\DosDevices\passthru");
    //������� ������ �����

    NdisZeroMemory(MajorFunctions, sizeof(MajorFunctions));
    //��������� ����� ������� � ��������
    MajorFunctions[IRP_MJ_CREATE] = FilterOpen;
    MajorFunctions[IRP_MJ_CLOSE] = FilterClose;
    MajorFunctions[IRP_MJ_READ] = FilterRead;
    MajorFunctions[IRP_MJ_WRITE] = FilterWrite;
    MajorFunctions[IRP_MJ_DEVICE_CONTROL] = FilterIoControl;
    //������������ ��
    Status = NdisMRegisterDevice(WrapperHandle, &ntDeviceName,
        &win32DeviceName, MajorFunctions,
        &deviceObject, &GlobalData.NdisDeviceHandle);
    // ��������� ������
    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (GlobalData.ProtHandle)
            NdisDeregisterProtocol(&Status, GlobalData.ProtHandle);

        if (GlobalData.NdisDeviceHandle)
            NdisMDeregisterDevice(GlobalData.NdisDeviceHandle);

        if (WrapperHandle)
            NdisTerminateWrapper(WrapperHandle, NULL);
        return (Status);
    }

    // set access method into deviceObject ( received from NdisMRegisterDevice() )
    // ���������� ����������� ��� ����������� ��������
    deviceObject->Flags |= DO_BUFFERED_IO;


    // �������, ������������� NDIS � ���, ��� ����, ���������� ��� ������, �������� � ��������, � ��������� �� �������� �������, ���� ������� ������������.
    // ��������� �������� � ��������
    NdisIMAssociateMiniport(DriverHandle, ProtHandle);

    return(Status);
}

NTSTATUS FilterOpen(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    return NDIS_STATUS_SUCCESS;
}

NTSTATUS FilterClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    return NDIS_STATUS_SUCCESS;
}

NTSTATUS FilterRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    return NDIS_STATUS_SUCCESS;
}
NTSTATUS FilterWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    return NDIS_STATUS_SUCCESS;
}
NTSTATUS FilterIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    return NDIS_STATUS_SUCCESS;
}
