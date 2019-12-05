#include "precomp.h"

VOID
PtOpenAdapterComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                Status,
    IN NDIS_STATUS                OpenErrorStatus
    ) {}

VOID
PtCloseAdapterComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                Status
    ) {}

VOID
PtSendComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_PACKET               Packet,
    IN NDIS_STATUS                Status
)
{
    PADAPT pAdapt = (PADAPT)ProtocolBindingContext;
    PNDIS_PACKET Pkt;
    PRSVD Rsvd;
    //pAdapt = pAdapt->pPrimaryAdapt;

    Rsvd = (PRSVD)(Packet->ProtocolReserved);
    Pkt = Rsvd->OriginalPkt;

    NdisIMCopySendCompletePerPacketInfo(Pkt, Packet);

    NdisDprFreePacket(Packet);

    NdisMSendComplete(pAdapt->MiniportHandle, Pkt, Status);
}

VOID
PtTransferDataComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_PACKET               Packet,
    IN NDIS_STATUS                Status,
    IN UINT                       BytesTransferred
    ) {}

VOID
PtResetComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                Status
    ) {}

VOID
PtRequestComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_REQUEST              NdisRequest,
    IN NDIS_STATUS                Status
) {}

NDIS_STATUS
PtReceive(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_HANDLE                MacReceiveContext,
    IN PVOID                      HeaderBuffer,
    IN UINT                       HeaderBufferSize,
    IN PVOID                      LookAheadBuffer,
    IN UINT                       LookaheadBufferSize,
    IN UINT                       PacketSize
    )
{
   //DbgPrint("RCV");

    PADAPT pAdapt = (PADAPT)ProtocolBindingContext;

    //Контекст адаптера
    PNDIS_PACKET MyPacket, Packet;  //Пакеты.
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    //Статус
    if (!pAdapt->MiniportHandle)
    {
        Status = NDIS_STATUS_FAILURE;
    }
    else
        do
        {
            //Эта часть работает при наличии второго адаптера :)
            if (pAdapt->Next)
            {
                //DbgPrint("PASSTHRU GETTING RECIEVES ON SECONDARY\n");
                ASSERT(0);
            }

            //Забираем указатель на пакет в NDIS.
            Packet = NdisGetReceivedPacket(pAdapt->BindingHandle, MacReceiveContext);

            //Если пакета нет то мы выходим иначе продолжаем как и в случае с отправкой.
            if (Packet != NULL)
            {
                //Резервируем пакет для себя.
                NdisDprAllocatePacket(&Status, &MyPacket, pAdapt->RecvPacketPoolHandle);
                if (Status == NDIS_STATUS_SUCCESS)
                {
                    //Копируем данные, как служебные, так и сами данные передаваемые наверх.
                    MyPacket->Private.Head = Packet->Private.Head;
                    MyPacket->Private.Tail = Packet->Private.Tail;
                    NDIS_SET_ORIGINAL_PACKET(MyPacket, NDIS_GET_ORIGINAL_PACKET(Packet));
                    NDIS_SET_PACKET_HEADER_SIZE(MyPacket, HeaderBufferSize);
                    NdisGetPacketFlags(MyPacket) = NdisGetPacketFlags(Packet);
                    NDIS_SET_PACKET_STATUS(MyPacket, NDIS_STATUS_RESOURCES);

                    //В этом случае мы не посылаем пакет как при отправке, а просто указываем
                    //NDIS что MyPacket готов к передаче наверх.
                    NdisMIndicateReceivePacket(pAdapt->MiniportHandle, &MyPacket, 1);
                    ASSERT(NDIS_GET_PACKET_STATUS(MyPacket) == NDIS_STATUS_RESOURCES);
                    //Освобождение пакета при нормальной передаче.
                    NdisDprFreePacket(MyPacket);
                    break;
                }
            }
            pAdapt->IndicateRcvComplete = TRUE;
            switch (pAdapt->Medium)
            {
            case NdisMedium802_3:

                NdisMEthIndicateReceive(pAdapt->MiniportHandle, MacReceiveContext, HeaderBuffer,
                    HeaderBufferSize, LookAheadBuffer, LookaheadBufferSize, PacketSize);
                break;

            case NdisMedium802_5:
                NdisMTrIndicateReceive(pAdapt->MiniportHandle, MacReceiveContext, HeaderBuffer,
                    HeaderBufferSize, LookAheadBuffer, LookaheadBufferSize, PacketSize);
                break;

            //case NdisMediumFddi:
            //    NdisMFddiIndicateReceive(pAdapt->MiniportHandle, MacReceiveContext, HeaderBuffer,
            //        HeaderBufferSize, LookAheadBuffer, LookaheadBufferSize, PacketSize);
                break;

            default:
                //Это в случае если тип сети неизвестен.
                ASSERT(0);
                break;
            }

        } while (FALSE);

}

VOID
PtReceiveComplete(
    IN NDIS_HANDLE                ProtocolBindingContext
    ) {}

VOID
PtStatus(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                GeneralStatus,
    IN PVOID                      StatusBuffer,
    IN UINT                       StatusBufferSize
    ) {}

VOID
PtStatusComplete(
    IN NDIS_HANDLE                ProtocolBindingContext
    ) {}

VOID
PtBindAdapter(
    OUT PNDIS_STATUS              Status,
    IN  NDIS_HANDLE               BindContext,
    IN  PNDIS_STRING              DeviceName,
    IN  PVOID                     SystemSpecific1,
    IN  PVOID                     SystemSpecific2
    ) {}

VOID
PtUnbindAdapter(
    OUT PNDIS_STATUS              Status,
    IN  NDIS_HANDLE               ProtocolBindingContext,
    IN  NDIS_HANDLE               UnbindContext
    ) {}

INT
PtReceivePacket(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_PACKET               Packet
    )
{
    return 1;
}

NDIS_STATUS
PtPNPHandler(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNET_PNP_EVENT             pNetPnPEvent
    )
{
    return NDIS_STATUS_SUCCESS;
}

VOID
PtUnload(
    IN PDRIVER_OBJECT             DriverObject
    ) {}


