#include "precomp.h"

NDIS_STATUS
MPInitialize(
    OUT PNDIS_STATUS             OpenErrorStatus,
    OUT PUINT                    SelectedMediumIndex,
    IN PNDIS_MEDIUM              MediumArray,
    IN UINT                      MediumArraySize,
    IN NDIS_HANDLE               MiniportAdapterHandle,
    IN NDIS_HANDLE               WrapperConfigurationContext
    ) 
{
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MPSend(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN PNDIS_PACKET               Packet,
    IN UINT                       Flags
    )
{
    PADAPT pAdapt = (PADAPT)MiniportAdapterContext;
    //Контекст адаптера приходящий в качестве параметра. Присвоим его своему типизированному указателю.
    NDIS_STATUS Status;

    //Возвращаемый статус.
    PNDIS_PACKET MyPacket;

    //Наш пакет - пока только указатель.
    PRSVD Rsvd;

    //Резервный указатель.
    PVOID MediaSpecificInfo = NULL;

    //Тип адаптера с которым будем работать.
    ULONG MediaSpecificInfoSize = 0;

    //Размер типа адаптера.
    // изменено
    ASSERT(pAdapt->Next);
    pAdapt = pAdapt->Next;

    ////Проверка наличия второго сетевого адаптера. Вверху я говорил, что его наличие необходимо предусматривать.
    //if (IsIMDeviceStateOn(pAdapt) == FALSE)
    //{
    //    return NDIS_STATUS_FAILURE;
    //}

    //Проверка наличия и состояния.
    NdisAllocatePacket(&Status, &MyPacket, pAdapt->SendPacketPoolHandle);

    //Выделение места под размер полученного пакета (Pool) данных.
    if (Status == NDIS_STATUS_SUCCESS)
    {
        PNDIS_PACKET_EXTENSION Old, New;
        Rsvd = (PRSVD)(MyPacket->ProtocolReserved);
        Rsvd->OriginalPkt = Packet;
        MyPacket->Private.Flags = Flags;
        MyPacket->Private.Head = Packet->Private.Head;
        MyPacket->Private.Tail = Packet->Private.Tail;

        //Собственно копирование всей служебной информации
        NdisSetPacketFlags(MyPacket, NDIS_FLAGS_DONT_LOOPBACK);


        //Установка ее в наш внутренний буфер.

        NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(MyPacket), NDIS_OOB_DATA_FROM_PACKET(Packet),
            sizeof(NDIS_PACKET_OOB_DATA));

        //Перенос данных в сам пакет.
        NdisIMCopySendPerPacketInfo(MyPacket, Packet);

        //Копирование служебных данных по пересылке пакета.
        //Копирование данных о типе адаптера, куда пересылать данные.

        NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(Packet, &MediaSpecificInfo, &MediaSpecificInfoSize);
        if (MediaSpecificInfo || MediaSpecificInfoSize)
        {
            NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(MyPacket, MediaSpecificInfo, MediaSpecificInfoSize);
        }

        //Собственно пересылка имеющихся данных в NDIS, что вызовет нормальное
        //прохождение пакета далее по цепочке драйверов.
        NdisSend(&Status, pAdapt->BindingHandle, MyPacket);
        if (Status != NDIS_STATUS_PENDING)
        {
            NdisIMCopySendCompletePerPacketInfo(Packet, MyPacket);
            NdisFreePacket(MyPacket);
        }

        //Если нет задержки на отсылке освободить пакет.
    }
    else
    {
        //Это говорит об отсутствии пакета в системе - ничего не надо делать.
    }
    return(Status);
}

VOID
MPSendPackets(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN PPNDIS_PACKET              PacketArray,
    IN UINT                       NumberOfPackets
    ) {}

NDIS_STATUS
MPTransferData(
    OUT PNDIS_PACKET              Packet,
    OUT PUINT                     BytesTransferred,
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN NDIS_HANDLE                MiniportReceiveContext,
    IN UINT                       ByteOffset,
    IN UINT                       BytesToTransfer
    )
{
    return NDIS_STATUS_SUCCESS;
}

VOID
MPReturnPacket(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN PNDIS_PACKET               Packet
    ) {}

NDIS_STATUS
MPQueryInformation(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN NDIS_OID                   Oid,
    IN PVOID                      InformationBuffer,
    IN ULONG                      InformationBufferLength,
    OUT PULONG                    BytesWritten,
    OUT PULONG                    BytesNeeded
    )
{
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MPSetInformation(
    IN NDIS_HANDLE                                      MiniportAdapterContext,
    IN NDIS_OID                                         Oid,
    __in_bcount(InformationBufferLength) IN PVOID       InformationBuffer,
    IN ULONG                                            InformationBufferLength,
    OUT PULONG                                          BytesRead,
    OUT PULONG                                          BytesNeeded
    )
{
    return NDIS_STATUS_SUCCESS;
}

VOID
MPHalt(
    IN NDIS_HANDLE                MiniportAdapterContext
    ) {}

NDIS_STATUS
MPReset(
    OUT PBOOLEAN AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    )
{
    return NDIS_STATUS_SUCCESS;
}