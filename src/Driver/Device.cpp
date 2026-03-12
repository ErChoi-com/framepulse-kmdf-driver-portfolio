#include <initguid.h>

#include "Driver.h"

DEFINE_GUID(
    GUID_DEVINTERFACE_FRAMEPULSE,
    0x52e2d8dc,
    0x455c,
    0x4299,
    0xa7,
    0x49,
    0x4f,
    0x58,
    0x29,
    0x55,
    0x0d,
    0xc0);

NTSTATUS FramePulseEvtDeviceAdd(
    _In_ WDFDRIVER driver,
    _Inout_ PWDFDEVICE_INIT deviceInit)
{
    UNREFERENCED_PARAMETER(driver);

    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES lockAttributes;
    PFRAMEPULSE_DEVICE_CONTEXT context;

    WdfDeviceInitSetDeviceType(deviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetCharacteristics(deviceInit, FILE_DEVICE_SECURE_OPEN, FALSE);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FRAMEPULSE_DEVICE_CONTEXT);
    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    status = WdfDeviceCreate(&deviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[FramePulse] WdfDeviceCreate failed: 0x%08X\n", status);
        return status;
    }

    context = FramePulseGetDeviceContext(device);
    FramePulseInitializeState(context);

    WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);
    lockAttributes.ParentObject = device;

    status = WdfWaitLockCreate(&lockAttributes, &context->Lock);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[FramePulse] WdfWaitLockCreate failed: 0x%08X\n", status);
        return status;
    }

    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_FRAMEPULSE, nullptr);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[FramePulse] WdfDeviceCreateDeviceInterface failed: 0x%08X\n", status);
        return status;
    }

    status = FramePulseCreateQueue(device);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[FramePulse] FramePulseCreateQueue failed: 0x%08X\n", status);
        return status;
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[FramePulse] Device added and interface published\n");
    return STATUS_SUCCESS;
}
