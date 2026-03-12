[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$requiredFiles = @(
    'README.md',
    'KernelModePortfolio.sln',
    'src/Shared/FramePulseShared.h',
    'src/Driver/FramePulseDriver.vcxproj',
    'src/UserClient/FramePulseClient.vcxproj',
    'docs/setup-guide.md',
    'docs/architecture.md',
    'docs/windbg-playbook.md',
    'docs/interview-guide.md'
)

foreach ($relativePath in $requiredFiles) {
    $absolutePath = Join-Path $repoRoot $relativePath
    if (-not (Test-Path $absolutePath)) {
        throw "Missing required repository file: $relativePath"
    }
}

$parseErrors = @()
$scriptFiles = Get-ChildItem -Path (Join-Path $repoRoot 'scripts') -Filter *.ps1 -File
foreach ($scriptFile in $scriptFiles) {
    $tokens = $null
    $errors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($scriptFile.FullName, [ref]$tokens, [ref]$errors) | Out-Null
    if ($errors) {
        $parseErrors += $errors | ForEach-Object { "{0}:{1}:{2} {3}" -f $scriptFile.Name, $_.Extent.StartLineNumber, $_.Extent.StartColumnNumber, $_.Message }
    }
}

if ($parseErrors.Count -gt 0) {
    $parseErrors | ForEach-Object { Write-Error $_ }
    throw 'PowerShell syntax validation failed.'
}

$readmePath = Join-Path $repoRoot 'README.md'
$readmeContent = Get-Content -Path $readmePath -Raw
if ($readmeContent -notmatch 'FramePulse KMDF Driver Portfolio Project') {
    throw 'README.md is missing the expected project title.'
}

Write-Host 'Repository hygiene validation completed successfully.' -ForegroundColor Green
