#include "Driver.h"

extern "C" {
#include <ntstrsafe.h>
}

namespace {

uint64_t CaptureTimestamp100ns()
{
    LARGE_INTEGER systemTime;
    KeQuerySystemTimePrecise(&systemTime);
    return static_cast<uint64_t>(systemTime.QuadPart);
}

FRAMEPULSE_EVENT_KIND MapIoctlToEvent(_In_ uint32_t ioctlCode)
{
    switch (ioctlCode) {
    case IOCTL_FRAMEPULSE_GET_VERSION:
        return FramePulseEventGetVersion;
    case IOCTL_FRAMEPULSE_PING:
        return FramePulseEventPing;
    case IOCTL_FRAMEPULSE_GET_STATS:
        return FramePulseEventGetStats;
    case IOCTL_FRAMEPULSE_RESET_STATS:
        return FramePulseEventResetStats;
    case IOCTL_FRAMEPULSE_READ_LOG:
        return FramePulseEventReadLog;
    default:
        return FramePulseEventUnknown;
    }
}

void PushLogEntry(
    _Inout_ PFRAMEPULSE_DEVICE_CONTEXT context,
    _In_ uint32_t ioctlCode,
    _In_ FRAMEPULSE_EVENT_KIND eventKind,
    _In_ NTSTATUS status,
    _In_ uint64_t payload,
    _In_ uint64_t timestamp100ns,
    _In_ uint64_t processId,
    _In_ uint64_t threadId)
{
    uint32_t index;

    if (context->LogCount == FRAMEPULSE_LOG_CAPACITY) {
        index = context->LogHead;
        context->LogHead = (context->LogHead + 1u) % FRAMEPULSE_LOG_CAPACITY;
        context->Stats.DroppedLogEntries += 1u;
    } else {
        index = (context->LogHead + context->LogCount) % FRAMEPULSE_LOG_CAPACITY;
        context->LogCount += 1u;
    }

    context->Log[index].SequenceNumber = context->NextSequenceNumber++;
    context->Log[index].Timestamp100ns = timestamp100ns;
    context->Log[index].IoctlCode = ioctlCode;
    context->Log[index].NtStatus = static_cast<int32_t>(status);
    context->Log[index].EventKind = static_cast<uint32_t>(eventKind);
    context->Log[index].ProcessId = processId;
    context->Log[index].ThreadId = threadId;
    context->Log[index].Payload = payload;
}

} // namespace

NTSTATUS FramePulseCreateQueue(_In_ WDFDEVICE device)
{
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES queueAttributes;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = FramePulseEvtIoDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT(&queueAttributes);
    queueAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    return WdfIoQueueCreate(device, &queueConfig, &queueAttributes, WDF_NO_HANDLE);
}

void FramePulseInitializeState(_Inout_ PFRAMEPULSE_DEVICE_CONTEXT context)
{
    RtlZeroMemory(context, sizeof(*context));
    context->Stats.StructVersion = FRAMEPULSE_PROTOCOL_VERSION;
    context->NextSequenceNumber = 1u;
}

void FramePulseRecordIoctl(
    _In_ WDFDEVICE device,
    _In_ uint32_t ioctlCode,
    _In_ FRAMEPULSE_EVENT_KIND eventKind,
    _In_ NTSTATUS status,
    _In_ uint64_t payload)
{
    PFRAMEPULSE_DEVICE_CONTEXT context = FramePulseGetDeviceContext(device);
    const uint64_t timestamp100ns = CaptureTimestamp100ns();
    const uint64_t processId = static_cast<uint64_t>(reinterpret_cast<ULONG_PTR>(PsGetCurrentProcessId()));
    const uint64_t threadId = static_cast<uint64_t>(reinterpret_cast<ULONG_PTR>(PsGetCurrentThreadId()));

    WdfWaitLockAcquire(context->Lock, nullptr);

    context->Stats.StructVersion = FRAMEPULSE_PROTOCOL_VERSION;
    context->Stats.TotalIoctls += 1u;
    context->Stats.LastIoctlCode = ioctlCode;
    context->Stats.LastTimestamp100ns = timestamp100ns;
    context->Stats.LastPayload = payload;
    context->Stats.LastProcessId = processId;
    context->Stats.LastThreadId = threadId;

    if (NT_SUCCESS(status)) {
        context->Stats.SuccessfulIoctls += 1u;
    } else {
        context->Stats.FailedIoctls += 1u;
    }

    switch (eventKind) {
    case FramePulseEventPing:
        context->Stats.PingRequests += 1u;
        break;
    case FramePulseEventResetStats:
        context->Stats.ResetRequests += 1u;
        break;
    case FramePulseEventReadLog:
        context->Stats.LogReads += 1u;
        break;
    default:
        break;
    }

    PushLogEntry(context, ioctlCode, eventKind, status, payload, timestamp100ns, processId, threadId);
    WdfWaitLockRelease(context->Lock);
}

void FramePulseCopyStats(_In_ WDFDEVICE device, _Out_ FRAMEPULSE_DRIVER_STATS* stats)
{
    PFRAMEPULSE_DEVICE_CONTEXT context = FramePulseGetDeviceContext(device);

    WdfWaitLockAcquire(context->Lock, nullptr);
    *stats = context->Stats;
    WdfWaitLockRelease(context->Lock);
}

void FramePulseResetState(_In_ WDFDEVICE device)
{
    PFRAMEPULSE_DEVICE_CONTEXT context = FramePulseGetDeviceContext(device);

    WdfWaitLockAcquire(context->Lock, nullptr);
    RtlZeroMemory(context->Log, sizeof(context->Log));
    context->LogHead = 0u;
    context->LogCount = 0u;
    context->NextSequenceNumber = 1u;
    RtlZeroMemory(&context->Stats, sizeof(context->Stats));
    context->Stats.StructVersion = FRAMEPULSE_PROTOCOL_VERSION;
    WdfWaitLockRelease(context->Lock);
}

void FramePulseCopyLogSnapshot(_In_ WDFDEVICE device, _Out_ FRAMEPULSE_LOG_SNAPSHOT* snapshot)
{
    PFRAMEPULSE_DEVICE_CONTEXT context = FramePulseGetDeviceContext(device);
    uint32_t count;

    RtlZeroMemory(snapshot, sizeof(*snapshot));

    WdfWaitLockAcquire(context->Lock, nullptr);

    snapshot->StructVersion = FRAMEPULSE_PROTOCOL_VERSION;
    count = context->LogCount;
    if (count > FRAMEPULSE_LOG_SNAPSHOT_CAPACITY) {
        count = FRAMEPULSE_LOG_SNAPSHOT_CAPACITY;
    }

    snapshot->EntryCount = count;

    for (uint32_t index = 0; index < count; ++index) {
        const uint32_t sourceIndex = (context->LogHead + context->LogCount - count + index) % FRAMEPULSE_LOG_CAPACITY;
        snapshot->Entries[index] = context->Log[sourceIndex];
    }

    WdfWaitLockRelease(context->Lock);
}

void FramePulseEvtIoDeviceControl(
    _In_ WDFQUEUE queue,
    _In_ WDFREQUEST request,
    _In_ size_t outputBufferLength,
    _In_ size_t inputBufferLength,
    _In_ ULONG ioControlCode)
{
    UNREFERENCED_PARAMETER(outputBufferLength);
    UNREFERENCED_PARAMETER(inputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;
    size_t bytesReturned = 0;
    uint64_t payload = 0;
    WDFDEVICE device = WdfIoQueueGetDevice(queue);
    FRAMEPULSE_EVENT_KIND eventKind = MapIoctlToEvent(ioControlCode);

    switch (ioControlCode) {
    case IOCTL_FRAMEPULSE_GET_VERSION:
    {
        FRAMEPULSE_VERSION_OUTPUT* versionOutput = nullptr;
        status = WdfRequestRetrieveOutputBuffer(
            request,
            sizeof(FRAMEPULSE_VERSION_OUTPUT),
            reinterpret_cast<PVOID*>(&versionOutput),
            nullptr);
        if (NT_SUCCESS(status)) {
            RtlZeroMemory(versionOutput, sizeof(*versionOutput));
            versionOutput->StructVersion = FRAMEPULSE_PROTOCOL_VERSION;
            versionOutput->DriverVersionMajor = FRAMEPULSE_DRIVER_VERSION_MAJOR;
            versionOutput->DriverVersionMinor = FRAMEPULSE_DRIVER_VERSION_MINOR;
            versionOutput->DriverVersionPatch = FRAMEPULSE_DRIVER_VERSION_PATCH;
#if DBG
            versionOutput->BuildConfiguration = 1u;
#else
            versionOutput->BuildConfiguration = 0u;
#endif
            status = RtlStringCchCopyA(
                versionOutput->BuildStamp,
                RTL_NUMBER_OF(versionOutput->BuildStamp),
                __DATE__ " " __TIME__);
            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(*versionOutput);
            }
        }
        break;
    }

    case IOCTL_FRAMEPULSE_PING:
    {
        FRAMEPULSE_PING_REQUEST* pingRequest = nullptr;
        FRAMEPULSE_PING_RESPONSE* pingResponse = nullptr;

        status = WdfRequestRetrieveInputBuffer(
            request,
            sizeof(FRAMEPULSE_PING_REQUEST),
            reinterpret_cast<PVOID*>(&pingRequest),
            nullptr);
        if (!NT_SUCCESS(status)) {
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(
            request,
            sizeof(FRAMEPULSE_PING_RESPONSE),
            reinterpret_cast<PVOID*>(&pingResponse),
            nullptr);
        if (!NT_SUCCESS(status)) {
            break;
        }

        payload = pingRequest->Value;
        pingResponse->EchoedValue = pingRequest->Value ^ 0xA5A5A5A5A5A5A5A5ull;
        pingResponse->Timestamp100ns = CaptureTimestamp100ns();
        bytesReturned = sizeof(*pingResponse);
        break;
    }

    case IOCTL_FRAMEPULSE_GET_STATS:
    {
        FRAMEPULSE_DRIVER_STATS* statsOutput = nullptr;

        status = WdfRequestRetrieveOutputBuffer(
            request,
            sizeof(FRAMEPULSE_DRIVER_STATS),
            reinterpret_cast<PVOID*>(&statsOutput),
            nullptr);
        if (NT_SUCCESS(status)) {
            FramePulseCopyStats(device, statsOutput);
            bytesReturned = sizeof(*statsOutput);
        }
        break;
    }

    case IOCTL_FRAMEPULSE_RESET_STATS:
        FramePulseResetState(device);
        break;

    case IOCTL_FRAMEPULSE_READ_LOG:
    {
        FRAMEPULSE_LOG_SNAPSHOT* logOutput = nullptr;

        status = WdfRequestRetrieveOutputBuffer(
            request,
            sizeof(FRAMEPULSE_LOG_SNAPSHOT),
            reinterpret_cast<PVOID*>(&logOutput),
            nullptr);
        if (NT_SUCCESS(status)) {
            FramePulseCopyLogSnapshot(device, logOutput);
            bytesReturned = sizeof(*logOutput);
        }
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (ioControlCode == IOCTL_FRAMEPULSE_RESET_STATS && NT_SUCCESS(status)) {
        FramePulseRecordIoctl(device, ioControlCode, eventKind, status, payload);
    } else if (ioControlCode != IOCTL_FRAMEPULSE_READ_LOG || NT_SUCCESS(status)) {
        FramePulseRecordIoctl(device, ioControlCode, eventKind, status, payload);
    } else {
        FramePulseRecordIoctl(device, ioControlCode, eventKind, status, payload);
    }

    WdfRequestCompleteWithInformation(request, status, bytesReturned);
}
