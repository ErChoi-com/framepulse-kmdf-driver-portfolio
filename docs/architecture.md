# Architecture

## Overview

FramePulse is a KMDF, root-enumerated software device that exposes a single device interface to a user-mode validation tool.

The driver accepts a small set of buffered IOCTLs:

- `IOCTL_FRAMEPULSE_GET_VERSION`: returns a version block and build stamp
- `IOCTL_FRAMEPULSE_PING`: validates request/response flow with a payload and kernel timestamp
- `IOCTL_FRAMEPULSE_GET_STATS`: returns aggregate counters and last-operation metadata
- `IOCTL_FRAMEPULSE_RESET_STATS`: clears state and starts a new telemetry window
- `IOCTL_FRAMEPULSE_READ_LOG`: returns a snapshot of a bounded ring buffer of recent activity

## Why this design matters

This project is not trying to mimic a full graphics stack. It is designed to surface the engineering decisions interviewers care about:

- How requests enter the driver
- How shared state is protected
- How user/kernel contracts are versioned
- How you keep telemetry bounded and safe
- How you package and deploy the driver for bring-up testing

## Request path

1. `DriverEntry` initializes KMDF and registers `FramePulseEvtDeviceAdd`.
2. `FramePulseEvtDeviceAdd` creates the device object, publishes a device interface, initializes state, and creates a default sequential queue.
3. `FramePulseEvtIoDeviceControl` handles each IOCTL, fills an output structure when required, and records a telemetry entry.
4. The user-mode client locates the device interface through SetupAPI and communicates with `DeviceIoControl`.

## Synchronization strategy

The default queue is sequential to keep the request path simple and deterministic for a first-pass portfolio driver. Shared telemetry state is still protected by a `WDFWAITLOCK` so the design can evolve toward additional callbacks or parallel dispatch without rewriting the bookkeeping logic.

That gives you a strong interview talking point: queue serialization reduces complexity, while the explicit lock keeps the state management honest and extensible.

## Telemetry model

The telemetry block stores:

- total, successful, and failed IOCTL counts
- per-feature counters for ping, reset, and log reads
- last seen IOCTL code, payload, timestamp, PID, and TID
- a ring buffer of the most recent 64 events, with a 32-entry user snapshot window

The ring buffer is bounded to avoid untrusted growth and to keep the interface stable for debugging and demos.

## Natural extensions

If you want to evolve this into a second-round or final-round project, the cleanest next steps are:

- add ETW tracing to correlate kernel events with GPUView timelines
- add cancellation or asynchronous completion paths to discuss IRQL and concurrency tradeoffs
- add shared memory or section objects for low-overhead telemetry export
- add a user-mode service that stress tests the driver from multiple threads
- add fault injection modes to exercise error handling in WinDbg
