[CmdletBinding()]
param()

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Run this script from an elevated PowerShell session."
}

$devcon = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Tools' -Filter devcon.exe -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\x64\\devcon\.exe$' } |
    Select-Object -First 1

if ($devcon) {
    Write-Host "Removing root-enumerated FramePulse device..." -ForegroundColor Cyan
    & $devcon.FullName remove 'Root\FramePulseDriver' | Out-Host
}

$publishedNames = @()
$lines = pnputil /enum-drivers
$currentPublishedName = $null

foreach ($line in $lines) {
    if ($line -match '^Published Name\s*:\s*(.+)$') {
        $currentPublishedName = $Matches[1].Trim()
        continue
    }

    if ($line -match '^Original Name\s*:\s*(.+)$') {
        if ($Matches[1].Trim().ToLowerInvariant() -eq 'framepulsedriver.inf' -and $currentPublishedName) {
            $publishedNames += $currentPublishedName
        }
    }
}

foreach ($publishedName in $publishedNames | Select-Object -Unique) {
    Write-Host "Deleting driver package $publishedName ..." -ForegroundColor Cyan
    pnputil /delete-driver $publishedName /uninstall /force | Out-Host
}

Write-Host "Uninstall complete." -ForegroundColor Green
