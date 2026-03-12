#pragma once

extern "C" {
#include <ntddk.h>
#include <wdf.h>
}

#include "..\Shared\FramePulseShared.h"

typedef struct _FRAMEPULSE_DEVICE_CONTEXT {
    WDFWAITLOCK Lock;
    FRAMEPULSE_DRIVER_STATS Stats;
    FRAMEPULSE_LOG_ENTRY Log[FRAMEPULSE_LOG_CAPACITY];
    uint32_t LogHead;
    uint32_t LogCount;
    uint64_t NextSequenceNumber;
} FRAMEPULSE_DEVICE_CONTEXT, *PFRAMEPULSE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FRAMEPULSE_DEVICE_CONTEXT, FramePulseGetDeviceContext);

EVT_WDF_DRIVER_DEVICE_ADD FramePulseEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP FramePulseEvtDriverContextCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL FramePulseEvtIoDeviceControl;

NTSTATUS FramePulseCreateQueue(_In_ WDFDEVICE device);
void FramePulseInitializeState(_Inout_ PFRAMEPULSE_DEVICE_CONTEXT context);
void FramePulseRecordIoctl(
    _In_ WDFDEVICE device,
    _In_ uint32_t ioctlCode,
    _In_ FRAMEPULSE_EVENT_KIND eventKind,
    _In_ NTSTATUS status,
    _In_ uint64_t payload);
void FramePulseCopyStats(_In_ WDFDEVICE device, _Out_ FRAMEPULSE_DRIVER_STATS* stats);
void FramePulseResetState(_In_ WDFDEVICE device);
void FramePulseCopyLogSnapshot(_In_ WDFDEVICE device, _Out_ FRAMEPULSE_LOG_SNAPSHOT* snapshot);
