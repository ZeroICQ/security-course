﻿extern NTSTATUS FilterOpen(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);


//
// Miniport proto-types
//
NDIS_STATUS
MPInitialize(
    OUT PNDIS_STATUS             OpenErrorStatus,
    OUT PUINT                    SelectedMediumIndex,
    IN PNDIS_MEDIUM              MediumArray,
    IN UINT                      MediumArraySize,
    IN NDIS_HANDLE               MiniportAdapterHandle,
    IN NDIS_HANDLE               WrapperConfigurationContext
);

NDIS_STATUS
MPSend(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN PNDIS_PACKET               Packet,
    IN UINT                       Flags
);

VOID
MPSendPackets(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN PPNDIS_PACKET              PacketArray,
    IN UINT                       NumberOfPackets
);

NDIS_STATUS
MPTransferData(
    OUT PNDIS_PACKET              Packet,
    OUT PUINT                     BytesTransferred,
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN NDIS_HANDLE                MiniportReceiveContext,
    IN UINT                       ByteOffset,
    IN UINT                       BytesToTransfer
);

VOID
MPReturnPacket(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN PNDIS_PACKET               Packet
);

NDIS_STATUS
MPQueryInformation(
    IN NDIS_HANDLE                MiniportAdapterContext,
    IN NDIS_OID                   Oid,
    IN PVOID                      InformationBuffer,
    IN ULONG                      InformationBufferLength,
    OUT PULONG                    BytesWritten,
    OUT PULONG                    BytesNeeded
);

NDIS_STATUS
MPSetInformation(
    IN NDIS_HANDLE                                      MiniportAdapterContext,
    IN NDIS_OID                                         Oid,
    __in_bcount(InformationBufferLength) IN PVOID       InformationBuffer,
    IN ULONG                                            InformationBufferLength,
    OUT PULONG                                          BytesRead,
    OUT PULONG                                          BytesNeeded
);

VOID
MPHalt(
    IN NDIS_HANDLE                MiniportAdapterContext
);

NDIS_STATUS
MPReset(
    OUT PBOOLEAN AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
);

//
// Protocol proto-types
//
extern
VOID
PtOpenAdapterComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                Status,
    IN NDIS_STATUS                OpenErrorStatus
);

extern
VOID
PtCloseAdapterComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                Status
);

extern
VOID
PtSendComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_PACKET               Packet,
    IN NDIS_STATUS                Status
);

extern
VOID
PtTransferDataComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_PACKET               Packet,
    IN NDIS_STATUS                Status,
    IN UINT                       BytesTransferred
);

extern
VOID
PtResetComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                Status
);

extern
VOID
PtRequestComplete(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_REQUEST              NdisRequest,
    IN NDIS_STATUS                Status
);

extern
NDIS_STATUS
PtReceive(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_HANDLE                MacReceiveContext,
    IN PVOID                      HeaderBuffer,
    IN UINT                       HeaderBufferSize,
    IN PVOID                      LookAheadBuffer,
    IN UINT                       LookaheadBufferSize,
    IN UINT                       PacketSize
);

extern
VOID
PtReceiveComplete(
    IN NDIS_HANDLE                ProtocolBindingContext
);

extern
VOID
PtStatus(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN NDIS_STATUS                GeneralStatus,
    IN PVOID                      StatusBuffer,
    IN UINT                       StatusBufferSize
);

extern
VOID
PtStatusComplete(
    IN NDIS_HANDLE                ProtocolBindingContext
);

extern
VOID
PtBindAdapter(
    OUT PNDIS_STATUS              Status,
    IN  NDIS_HANDLE               BindContext,
    IN  PNDIS_STRING              DeviceName,
    IN  PVOID                     SystemSpecific1,
    IN  PVOID                     SystemSpecific2
);

extern
VOID
PtUnbindAdapter(
    OUT PNDIS_STATUS              Status,
    IN  NDIS_HANDLE               ProtocolBindingContext,
    IN  NDIS_HANDLE               UnbindContext
);

extern
INT
PtReceivePacket(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNDIS_PACKET               Packet
);

extern
NDIS_STATUS
PtPNPHandler(
    IN NDIS_HANDLE                ProtocolBindingContext,
    IN PNET_PNP_EVENT             pNetPnPEvent
);

VOID
PtUnload(
    IN PDRIVER_OBJECT             DriverObject
);

//
// Structure used by both the miniport as well as the protocol part of the intermediate driver
// to represent an adapter and its corres. lower bindings
//
typedef struct _ADAPT
{
    struct _ADAPT* Next;

    NDIS_HANDLE                    BindingHandle;    // To the lower miniport
    NDIS_HANDLE                    MiniportHandle;    // NDIS Handle to for miniport up-calls
    NDIS_HANDLE                    SendPacketPoolHandle;
    NDIS_HANDLE                    RecvPacketPoolHandle;
    NDIS_STATUS                    Status;            // Open Status
    NDIS_EVENT                     Event;            // Used by bind/halt for Open/Close Adapter synch.
    NDIS_MEDIUM                    Medium;
    NDIS_REQUEST                   Request;        // This is used to wrap a request coming down
                                                // to us. This exploits the fact that requests
                                                // are serialized down to us.
    PULONG                         BytesNeeded;
    PULONG                         BytesReadOrWritten;
    BOOLEAN                        ReceivedIndicationFlags[32];
    BOOLEAN                        IndicateRcvComplete;
    BOOLEAN                        OutstandingRequests;      // TRUE iff a request is pending
                                                        // at the miniport below
    BOOLEAN                        QueuedRequest;            // TRUE iff a request is queued at
                                                        // this IM miniport

    BOOLEAN                        StandingBy;                // True - When the miniport or protocol is transitioning from a D0 to Standby (>D0) State
    BOOLEAN                        UnbindingInProcess;
    NDIS_SPIN_LOCK                 Lock;
    // False - At all other times, - Flag is cleared after a transition to D0

    NDIS_DEVICE_POWER_STATE        MPDeviceState;            // Miniport's Device State 
    NDIS_DEVICE_POWER_STATE        PTDeviceState;            // Protocol's Device State 
    NDIS_STRING                    DeviceName;                // For initializing the miniport edge
    NDIS_EVENT                     MiniportInitEvent;        // For blocking UnbindAdapter while
                                                        // an IM Init is in progress.
    BOOLEAN                        MiniportInitPending;    // TRUE iff IMInit in progress
    NDIS_STATUS                    LastIndicatedStatus;    // The last indicated media status
    NDIS_STATUS                    LatestUnIndicateStatus; // The latest suppressed media status
    ULONG                          OutstandingSends;
    LONG                           RefCount;
    BOOLEAN                        MiniportIsHalted;
} ADAPT, * PADAPT;

typedef struct _RSVD
{
    PNDIS_PACKET    OriginalPkt;
} RSVD, *PRSVD;
