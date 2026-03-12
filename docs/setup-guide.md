# Setup Guide

## Host machine

Install the following on your development host:

1. Visual Studio 2022 Community or Professional with Desktop development with C++
2. Windows 10/11 SDK
3. Windows Driver Kit 10
4. WinDbg Preview from the Microsoft Store

Recommended host extras:

- Hyper-V or VMware Workstation for a dedicated Windows test VM
- Sysinternals DebugView for quick DbgPrint visibility when you do not need full kernel debugging

## Test VM

Create a clean Windows 10 or Windows 11 x64 VM dedicated to driver testing.

Required VM settings:

- Snapshot before first driver install
- Administrator account available
- Secure Boot disabled if your environment blocks test signing
- Network access if you want to use KDNET with WinDbg Preview

## Build the solution

Open KernelModePortfolio.sln in Visual Studio 2022 and build `Debug | x64` first.

Expected output folders:

- build/FramePulseDriver/Debug
- build/FramePulseClient/Debug

The driver output should include at least:

- FramePulseDriver.sys
- FramePulseDriver.inf
- FramePulseDriver.cat

## Enable test signing

Inside the test VM, run PowerShell as Administrator:

```powershell
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
.\scripts\enable-test-mode.ps1
```

Reboot the VM after the script completes.

## Install the driver

From an elevated PowerShell window in the VM:

```powershell
.\scripts\install-driver.ps1 -Configuration Debug
```

If DevCon is installed with the WDK, the script creates the root-enumerated device automatically. If DevCon is unavailable, the script still stages the package with PnPUtil and prints the fallback path.

## Exercise the driver

Run the client from the VM:

```powershell
.\build\FramePulseClient\Debug\FramePulseClient.exe demo
.\build\FramePulseClient\Debug\FramePulseClient.exe ping 0xCAFEBABE
.\build\FramePulseClient\Debug\FramePulseClient.exe stats
.\build\FramePulseClient\Debug\FramePulseClient.exe log
```

## Uninstall the driver

When you want to reset the environment:

```powershell
.\scripts\uninstall-driver.ps1
```

## Common failures

- `CreateFileW failed`: the device interface was not published because the driver is not installed or did not start correctly.
- `Access is denied`: run the install script from an elevated shell.
- `A certificate was explicitly revoked`: refresh your test signing flow and rebuild the driver package in Visual Studio.
- `No device interface found`: confirm the root device exists in Device Manager and that the driver loaded without Code 52 or Code 39.
