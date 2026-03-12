#pragma once

#include <guiddef.h>
#include <stdint.h>

#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x00000022
#endif

#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED 0
#endif

#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS 0
#endif

#define FRAMEPULSE_PROTOCOL_VERSION 1u
#define FRAMEPULSE_DRIVER_VERSION_MAJOR 1u
#define FRAMEPULSE_DRIVER_VERSION_MINOR 0u
#define FRAMEPULSE_DRIVER_VERSION_PATCH 0u

#define FRAMEPULSE_DEVICE_TYPE FILE_DEVICE_UNKNOWN
#define FRAMEPULSE_IOCTL_BASE 0x900u

#define IOCTL_FRAMEPULSE_GET_VERSION CTL_CODE(FRAMEPULSE_DEVICE_TYPE, FRAMEPULSE_IOCTL_BASE + 0u, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FRAMEPULSE_PING CTL_CODE(FRAMEPULSE_DEVICE_TYPE, FRAMEPULSE_IOCTL_BASE + 1u, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FRAMEPULSE_GET_STATS CTL_CODE(FRAMEPULSE_DEVICE_TYPE, FRAMEPULSE_IOCTL_BASE + 2u, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FRAMEPULSE_RESET_STATS CTL_CODE(FRAMEPULSE_DEVICE_TYPE, FRAMEPULSE_IOCTL_BASE + 3u, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FRAMEPULSE_READ_LOG CTL_CODE(FRAMEPULSE_DEVICE_TYPE, FRAMEPULSE_IOCTL_BASE + 4u, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FRAMEPULSE_LOG_CAPACITY 64u
#define FRAMEPULSE_LOG_SNAPSHOT_CAPACITY 32u

typedef enum _FRAMEPULSE_EVENT_KIND {
    FramePulseEventUnknown = 0,
    FramePulseEventGetVersion = 1,
    FramePulseEventPing = 2,
    FramePulseEventGetStats = 3,
    FramePulseEventResetStats = 4,
    FramePulseEventReadLog = 5
} FRAMEPULSE_EVENT_KIND;

typedef struct _FRAMEPULSE_VERSION_OUTPUT {
    uint32_t StructVersion;
    uint32_t DriverVersionMajor;
    uint32_t DriverVersionMinor;
    uint32_t DriverVersionPatch;
    uint32_t BuildConfiguration;
    char BuildStamp[32];
} FRAMEPULSE_VERSION_OUTPUT;

typedef struct _FRAMEPULSE_PING_REQUEST {
    uint64_t Value;
} FRAMEPULSE_PING_REQUEST;

typedef struct _FRAMEPULSE_PING_RESPONSE {
    uint64_t EchoedValue;
    uint64_t Timestamp100ns;
} FRAMEPULSE_PING_RESPONSE;

typedef struct _FRAMEPULSE_DRIVER_STATS {
    uint32_t StructVersion;
    uint32_t TotalIoctls;
    uint32_t SuccessfulIoctls;
    uint32_t FailedIoctls;
    uint32_t PingRequests;
    uint32_t ResetRequests;
    uint32_t LogReads;
    uint32_t DroppedLogEntries;
    uint32_t LastIoctlCode;
    uint64_t LastTimestamp100ns;
    uint64_t LastPayload;
    uint64_t LastProcessId;
    uint64_t LastThreadId;
} FRAMEPULSE_DRIVER_STATS;

typedef struct _FRAMEPULSE_LOG_ENTRY {
    uint64_t SequenceNumber;
    uint64_t Timestamp100ns;
    uint32_t IoctlCode;
    int32_t NtStatus;
    uint32_t EventKind;
    uint64_t ProcessId;
    uint64_t ThreadId;
    uint64_t Payload;
} FRAMEPULSE_LOG_ENTRY;

typedef struct _FRAMEPULSE_LOG_SNAPSHOT {
    uint32_t StructVersion;
    uint32_t EntryCount;
    FRAMEPULSE_LOG_ENTRY Entries[FRAMEPULSE_LOG_SNAPSHOT_CAPACITY];
} FRAMEPULSE_LOG_SNAPSHOT;

EXTERN_C const GUID GUID_DEVINTERFACE_FRAMEPULSE;
