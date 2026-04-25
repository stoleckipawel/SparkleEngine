# Purpose:
#   Prove Phase 1 shader cooker cache behavior is green:
#     1. Cold cook on an empty cache invokes the backend for every stage.
#     2. Warm cook with no source changes invokes zero backends.
#     3. Editing one shader source invalidates only that shader's stages.
#     4. --no-cache forces a full backend recook.
#
# Usage (from repo root, after building Debug ShaderCompiler):
#   pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts\CI\RunShaderCacheCheck.ps1
# ============================================================================

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$ShaderCompiler = Join-Path $RepoRoot 'bin\Debug\ShaderCompiler.exe'
$ShowcaseRoot = Join-Path $RepoRoot 'Projects\Showcase'
$CacheRoot = Join-Path $RepoRoot 'bin\Cache\Shaders\Phase1CacheValidation'
$TargetShader = Join-Path $RepoRoot 'Engine\Assets\Shaders\HelloWorld\HelloTriangle.hlsl'

if (-not (Test-Path $ShaderCompiler))
{
    Write-Host "[CI][ERROR] ShaderCompiler.exe not found at $ShaderCompiler. Build Debug ShaderCompiler before running this check."
    exit 1
}

if (-not (Test-Path $TargetShader))
{
    Write-Host "[CI][ERROR] Targeted invalidation shader not found at $TargetShader."
    exit 1
}

function Invoke-ShaderCook
{
    param(
        [Parameter(Mandatory=$true)][string]$Label,
        [Parameter(Mandatory=$true)][string[]]$Arguments
    )

    Write-Host ''
    Write-Host "[CI] $Label"

    $stdoutPath = [System.IO.Path]::GetTempFileName()
    $stderrPath = [System.IO.Path]::GetTempFileName()
    try
    {
        $process = Start-Process `
            -FilePath $ShaderCompiler `
            -ArgumentList $Arguments `
            -WorkingDirectory $ShowcaseRoot `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath `
            -NoNewWindow `
            -Wait `
            -PassThru

        $stdout = Get-Content -Raw -Path $stdoutPath
        $stderr = Get-Content -Raw -Path $stderrPath
        $exitCode = $process.ExitCode
    }
    finally
    {
        Remove-Item -Force $stdoutPath, $stderrPath -ErrorAction SilentlyContinue
    }

    $text = $stderr + $stdout
    Write-Host $text

    if ($exitCode -ne 0)
    {
        Write-Host "[CI][ERROR] $Label failed (rc=$exitCode)."
        exit $exitCode
    }

    return [PSCustomObject]@{
        Label = $Label
        Text = $text
        BackendInvocations = Read-CookMetric -Text $text -Name 'backendInvocations'
        CacheHits = Read-CookMetric -Text $text -Name 'cacheHits'
        CacheMisses = Read-CookMetric -Text $text -Name 'cacheMisses'
    }
}

function Read-CookMetric
{
    param(
        [Parameter(Mandatory=$true)][string]$Text,
        [Parameter(Mandatory=$true)][string]$Name
    )

    $match = [regex]::Match($Text, "${Name}=([0-9]+)")
    if (-not $match.Success)
    {
        Write-Host "[CI][ERROR] Could not find $Name in ShaderCompiler output."
        exit 1
    }

    return [int]$match.Groups[1].Value
}

function Assert-CookMetrics
{
    param(
        [Parameter(Mandatory=$true)]$Result,
        [Parameter(Mandatory=$true)][int]$ExpectedBackendInvocations,
        [Parameter(Mandatory=$true)][int]$ExpectedCacheHits,
        [Parameter(Mandatory=$true)][int]$ExpectedCacheMisses
    )

    if ($Result.BackendInvocations -ne $ExpectedBackendInvocations -or
        $Result.CacheHits -ne $ExpectedCacheHits -or
        $Result.CacheMisses -ne $ExpectedCacheMisses)
    {
        Write-Host "[CI][ERROR] Unexpected metrics for $($Result.Label)."
        Write-Host "[CI][ERROR] Expected backendInvocations=$ExpectedBackendInvocations cacheHits=$ExpectedCacheHits cacheMisses=$ExpectedCacheMisses."
        Write-Host "[CI][ERROR] Actual   backendInvocations=$($Result.BackendInvocations) cacheHits=$($Result.CacheHits) cacheMisses=$($Result.CacheMisses)."
        exit 1
    }
}

if (Test-Path $CacheRoot)
{
    Remove-Item -Recurse -Force $CacheRoot
}

$cookArgs = @('cook', '--backend', 'dxc', '--target', 'DxilSm66', '--cache-dir', $CacheRoot)

$originalShaderBytes = [System.IO.File]::ReadAllBytes($TargetShader)
$didModifyShader = $false
try
{
    $cold = Invoke-ShaderCook -Label 'Cold cook on empty cache' -Arguments $cookArgs
    Assert-CookMetrics -Result $cold -ExpectedBackendInvocations 7 -ExpectedCacheHits 0 -ExpectedCacheMisses 7

    $warm = Invoke-ShaderCook -Label 'Warm cook with populated cache' -Arguments $cookArgs
    Assert-CookMetrics -Result $warm -ExpectedBackendInvocations 0 -ExpectedCacheHits 7 -ExpectedCacheMisses 0

    $validationCommentBytes = [System.Text.Encoding]::UTF8.GetBytes("`r`n// Phase1 cache invalidation validation $(Get-Date -Format o)`r`n")
    $modifiedShaderBytes = New-Object byte[] ($originalShaderBytes.Length + $validationCommentBytes.Length)
    [System.Array]::Copy($originalShaderBytes, 0, $modifiedShaderBytes, 0, $originalShaderBytes.Length)
    [System.Array]::Copy($validationCommentBytes, 0, $modifiedShaderBytes, $originalShaderBytes.Length, $validationCommentBytes.Length)
    [System.IO.File]::WriteAllBytes($TargetShader, $modifiedShaderBytes)
    $didModifyShader = $true

    $targeted = Invoke-ShaderCook -Label 'Targeted source invalidation cook' -Arguments $cookArgs
    Assert-CookMetrics -Result $targeted -ExpectedBackendInvocations 2 -ExpectedCacheHits 5 -ExpectedCacheMisses 2
}
finally
{
    if ($didModifyShader)
    {
        [System.IO.File]::WriteAllBytes($TargetShader, $originalShaderBytes)
    }
}

$noCacheArgs = @('cook', '--backend', 'dxc', '--target', 'DxilSm66', '--cache-dir', $CacheRoot, '--no-cache')
$noCache = Invoke-ShaderCook -Label '--no-cache full recook' -Arguments $noCacheArgs
Assert-CookMetrics -Result $noCache -ExpectedBackendInvocations 7 -ExpectedCacheHits 0 -ExpectedCacheMisses 7

Write-Host ''
Write-Host '[CI][OK] Shader cache behavior is green.'
exit 0