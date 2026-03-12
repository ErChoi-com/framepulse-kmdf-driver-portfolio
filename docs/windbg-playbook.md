# WinDbg Playbook

## Attach strategy

Use a dedicated Windows VM and attach from the host with WinDbg Preview. Start with the driver built in `Debug | x64`.

If you only need `DbgPrintEx` output, DebugView is faster. If you need call stacks, breakpoints, WDF object inspection, or bugcheck analysis, use WinDbg.

## First commands to run

```text
.symfix
.reload /f
!sym noisy
lmvm FramePulseDriver
x FramePulseDriver!*
```

These confirm symbols loaded correctly and expose the exported/internal symbol names available for breakpoints.

## High-value breakpoints

```text
bu FramePulseDriver!DriverEntry
bu FramePulseDriver!FramePulseEvtDeviceAdd
bu FramePulseDriver!FramePulseEvtIoDeviceControl
```

Then from the guest, run `FramePulseClient.exe demo` and step through the queue handler.

## Useful WDF inspection

```text
!wdfkd.wdfdevice
!wdfkd.wdfqueue
!wdfkd.wdflogdump FramePulseDriver
```

If the extension is present, these commands help inspect framework objects and recent WDF activity.

## Driver-centric commands

```text
!drvobj FramePulseDriver 7
!devobj <device_object>
!thread
kv
```

Use these when validating object state, request context, and current thread execution.

## Watch the IOCTL flow

At `FramePulseEvtIoDeviceControl`, inspect the arguments and the shared device context:

```text
dv
dt FramePulseDriver!_FRAMEPULSE_DEVICE_CONTEXT <address>
```

Step through the switch statement and watch how the stats block and ring buffer change after each request.

## Common debugging stories to tell in an interview

- How you validated that the device interface was published correctly
- How you differentiated an install problem from a runtime request-path bug
- How you used symbols and breakpoints to confirm the IOCTL dispatch path
- How the bounded log helped reason about user-mode activity without depending only on live stepping

## Good extension exercises

If you want harder debugging material, intentionally add one bug at a time and root-cause it:

- wrong output buffer size handling
- stale sequence-number logic in the ring buffer
- lock omission around shared telemetry state
- incorrect user/kernel structure versioning

That produces the kind of concrete debugging narrative AMD teams tend to probe for.
