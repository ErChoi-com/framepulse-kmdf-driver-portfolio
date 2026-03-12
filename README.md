# FramePulse KMDF Driver Portfolio Project

FramePulse is a root-enumerated KMDF driver and companion C++ test client built to showcase the exact skills a Windows kernel-mode graphics driver team screens for: kernel C/C++, versioned user-kernel interfaces, synchronization, telemetry, installation, and WinDbg-driven debugging.

This project is intentionally portfolio-focused rather than hardware-dependent. It gives you a credible kernel artifact you can build, install in a Windows VM, exercise from user mode, and walk through in depth during an AMD or Microsoft driver interview.

## What this demonstrates

- Kernel-mode development with C++ and KMDF
- A versioned IOCTL contract shared by kernel and user mode
- Sequential queue dispatch and explicit state synchronization with WDF wait locks
- Kernel telemetry collection with a bounded ring buffer
- Root-enumerated driver packaging with INF-based installation
- A user-mode validation harness that discovers the device interface and exercises every IOCTL
- A WinDbg workflow for driver bring-up and issue investigation

## Repository layout

- src/Driver: KMDF driver source, INF, and Visual Studio project
- src/UserClient: C++ console client for device enumeration and IOCTL validation
- src/Shared: shared protocol definitions used by both sides of the boundary
- scripts: test-signing, install, and uninstall automation for a VM workflow
- docs: setup guide, architecture notes, WinDbg playbook, and interview positioning

## Suggested demo flow

1. Build the solution in Visual Studio 2022 with WDK 10 installed.
2. Copy the built artifacts into a Windows 11 test VM with test signing enabled.
3. Run scripts/enable-test-mode.ps1 once, reboot, then run scripts/install-driver.ps1.
4. Run build/FramePulseClient/Debug/FramePulseClient.exe demo.
5. Attach WinDbg Preview, set a breakpoint in FramePulseEvtIoDeviceControl, and replay the client commands.

## Why this is strong for AMD

The AMD posting emphasizes Windows kernel-mode C/C++, debugging with WinDbg, OS internals, user-mode and OS integration, and shipping stability-focused software in a large codebase. FramePulse gives you concrete evidence for each of those areas without pretending to be a GPU driver.

The project is honest about scope while still giving you advanced talking points: request flow, synchronization choices, interface versioning, packaging, telemetry, and how you would extend the design toward DMA, fence tracking, ETW, and GPUView correlation in a real graphics stack.

## Fast start

See docs/setup-guide.md first, then docs/windbg-playbook.md, then docs/interview-guide.md.
