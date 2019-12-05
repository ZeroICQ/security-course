extern NTSTATUS FilterOpen(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

extern NTSTATUS FilterIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
