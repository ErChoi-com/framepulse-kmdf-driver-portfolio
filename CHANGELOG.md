# Changelog

## v1.0.1 - 2026-03-12

- Fixed shared device interface GUID linkage so the user-mode validation client builds successfully in GitHub Actions.

## v1.0.0 - 2026-03-12

- Added the FramePulse KMDF driver solution with a root-enumerated software device, a versioned IOCTL contract, and bounded kernel telemetry.
- Added a SetupAPI-based C++ validation client for version, ping, stats, reset, and log workflows.
- Added install, uninstall, and test-signing PowerShell automation for VM-based bring-up.
- Added setup, architecture, WinDbg, and interview-positioning documentation aligned to Windows kernel driver hiring loops.
- Added GitHub Actions CI for the user-mode client and a repository hygiene validation pass.
