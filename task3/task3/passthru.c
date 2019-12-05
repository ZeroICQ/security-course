#include "precomp.h"
#include "ntddk.h"

typedef struct _GDATA {
    NDIS_HANDLE NdisDeviceHandle;
    NDIS_HANDLE ProtHandle;
} GDATA, * PGDATA;

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

    NDIS_HANDLE DriverHandle = NULL;
    NDIS_HANDLE ProtHandle = NULL;
    GDATA GlobalData;

    NDIS_STRING Name;
    NDIS_HANDLE WrapperHandle;
    // Этот массив будет содержать все указатели функций для регистрации.
    PDRIVER_DISPATCH MajorFunctions[IRP_MJ_MAXIMUM_FUNCTION + 1];

    NDIS_STRING ntDeviceName; //имя для вызова виртуального device-а для NT
    NDIS_STRING win32DeviceName; // тоже для win32
    PDEVICE_OBJECT deviceObject; // и объект.

    //
    // Register the miniport with NDIS. Note that it is the miniport
    // which was started as a driver and not the protocol. Also the miniport
    // must be registered prior to the protocol since the protocol's BindAdapter
    // handler can be initiated anytime and when it is, it must be ready to
    // start driver instances.
    //

    // функция указывает системе NDIS, что пришло время инициировать miniport service в ее системе.
    // Возвращаемое значение необходимо сохранить на будущее.
    // Обязательно надо обратить внимание, что если происходит ошибка при инициализации любого объекта, 
    // то при уже нормально отработавшей функции NdisMInitializeWrapper нужно вызвать NdisTerminateWrapper для высвобождения ресурса.
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
    // Функция, регистрирующая все функции уровня miniport
    Status = NdisIMRegisterLayeredMiniport(WrapperHandle, &MChars, sizeof(MChars), &DriverHandle);
    ASSERT(Status == NDIS_STATUS_SUCCESS);
    // хэдлер для правильной выгрузки функций группы miniport
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
    // Функция, регистрирующая все функции протокола. Протокольная группа
    NdisRegisterProtocol(&Status, &ProtHandle, &PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    ASSERT(Status == NDIS_STATUS_SUCCESS);

    // Name of control deviceObject.
    // DeviceName that names the device object.
    NdisInitUnicodeString(&ntDeviceName, L"\\Device\\passthru");

    // SymbolicName that is the Win32-visible name of the device
    NdisInitUnicodeString(&win32DeviceName, L"\\DosDevices\\passthru");
    //Создаем строку имени

    NdisZeroMemory(MajorFunctions, sizeof(MajorFunctions));
    //Связываем имена функций с массивом
    MajorFunctions[IRP_MJ_CREATE] = FilterOpen;
    MajorFunctions[IRP_MJ_CLOSE] = FilterClose;
    MajorFunctions[IRP_MJ_READ] = FilterRead;
    MajorFunctions[IRP_MJ_WRITE] = FilterWrite;
    MajorFunctions[IRP_MJ_DEVICE_CONTROL] = FilterIoControl;
    //Регистрируем их
    Status = NdisMRegisterDevice(WrapperHandle, &ntDeviceName,
        &win32DeviceName, MajorFunctions,
        &deviceObject, &GlobalData.NdisDeviceHandle);
    // проверяем статус
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
    // объявление буферизации для связывающих операций
    deviceObject->Flags |= DO_BUFFERED_IO;


    // Функция, информирующая NDIS о том, что есть, существует два уровня, минипорт и протокол, и говорящая об экспорте функций, если таковой присутствует.
    // Связываем протокол и минипорт
    NdisIMAssociateMiniport(DriverHandle, ProtHandle);

    return(Status);
}

NTSTATUS FilterOpen(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    // инициализация любых управляющих параметров, например структуры управляющих данных

    // Возращаемое значение - ошибка или нормальное.
    // При нормальном завершении - будет передан HANDLE на устройство, в нашем случае на драйвер.
    Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;
    // Количество переданных вверх байт - в нашем случае 0 потому как нет передаваемых параметров.
    Irp->IoStatus.Information = 0;
    // Завершение работы.
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return NDIS_STATUS_SUCCESS; // Нормальный код возврата.
}

NTSTATUS FilterClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    // Код окончания работы с драйвером.

    Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return NDIS_STATUS_SUCCESS;
}

unsigned char TestStr[10] = "abcdefghi";

NTSTATUS FilterRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG BytesReturned = 0;

    ////////////////////////////////////////////////////////////////////////
    // Get input parameters
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp); // Содержит стек Irp
    // Длина принятых данных -равна параметру максимально запрошенной длины в функции ReadFile
    ULONG BytesRequested = IrpStack->Parameters.Read.Length;

    if (BytesRequested < 10) // Если запрошено меньше нашей тестовой длины - вернуть ошибку
    {
        BytesReturned = 0;
        Status = STATUS_INVALID_BUFFER_SIZE;
    }
        else
        {
        // Если все в порядке - копировать буфер в выходной буфер стека.
        NdisMoveMemory(Irp->AssociatedIrp.SystemBuffer, TestStr, 10);
        BytesReturned = 10;
        Status = NDIS_STATUS_SUCCESS;
        }

        // Отправить
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = BytesReturned;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
}

///////////////////////////////////////////////////////////////////////////////////////////////

NTSTATUS FilterWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG BytesReturned = 0;

    // Здесь все работает аналогично.

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

#define IOCTL_SET_COMMAND1 CTL_CODE(FILE_DEVICE_UNKNOWN, 0xF07, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS FilterIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG BytesReturned = 0;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    PVOID InfoBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLen = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputBufferLen = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (IoControlCode)
    {
    case IOCTL_SET_COMMAND1:
        // Здесь мы можем сменить наш номер порта с 80 на, к примеру, 25.
        break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
