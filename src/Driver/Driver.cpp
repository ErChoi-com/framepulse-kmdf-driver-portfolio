#include "Driver.h"

extern "C"
DRIVER_INITIALIZE DriverEntry;

extern "C"
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT driverObject,
    _In_ PUNICODE_STRING registryPath)
{
    WDF_DRIVER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_DRIVER_CONFIG_INIT(&config, FramePulseEvtDeviceAdd);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = FramePulseEvtDriverContextCleanup;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[FramePulse] DriverEntry invoked\n");

    return WdfDriverCreate(driverObject, registryPath, &attributes, &config, WDF_NO_HANDLE);
}

void FramePulseEvtDriverContextCleanup(_In_ WDFOBJECT driverObject)
{
    UNREFERENCED_PARAMETER(driverObject);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[FramePulse] Driver cleanup invoked\n");
}
