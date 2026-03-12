[CmdletBinding()]
param()

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Run this script from an elevated PowerShell session."
}

Write-Host "Enabling Windows test signing and kernel debugging..." -ForegroundColor Cyan

bcdedit /set testsigning on | Out-Host
bcdedit /debug on | Out-Host

Write-Host "Test signing and kernel debugging are enabled. Reboot the VM before installing the driver." -ForegroundColor Green
