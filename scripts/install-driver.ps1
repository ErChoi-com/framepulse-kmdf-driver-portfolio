[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64')]
    [string]$Platform = 'x64'
)

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Run this script from an elevated PowerShell session."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$driverOutput = Join-Path $repoRoot ("build\FramePulseDriver\{0}" -f $Configuration)
$infPath = Join-Path $driverOutput 'FramePulseDriver.inf'

if (-not (Test-Path $infPath)) {
    throw "Driver INF not found at $infPath. Build the solution in Visual Studio first."
}

$devcon = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Tools' -Filter devcon.exe -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\x64\\devcon\.exe$' } |
    Select-Object -First 1

Write-Host "Staging driver package with PnPUtil..." -ForegroundColor Cyan
pnputil /add-driver $infPath /install | Out-Host

if ($devcon) {
    Write-Host "Using DevCon to create or update the root-enumerated device..." -ForegroundColor Cyan
    & $devcon.FullName update $infPath 'Root\FramePulseDriver' | Out-Host
    if ($LASTEXITCODE -ne 0) {
        & $devcon.FullName install $infPath 'Root\FramePulseDriver' | Out-Host
    }
} else {
    Write-Warning "DevCon was not found. The package is staged, but creating the root device may require Device Manager or a WDK install."
}

Write-Host "Installation attempt complete." -ForegroundColor Green
Write-Host "Next steps:" -ForegroundColor Green
Write-Host "  1. Verify the device exists in Device Manager under System devices."
Write-Host "  2. Run build\FramePulseClient\$Configuration\FramePulseClient.exe demo"
Write-Host "  3. Attach WinDbg Preview and set breakpoints on FramePulseDriver symbols."
