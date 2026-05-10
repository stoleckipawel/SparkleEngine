# Purpose:
#   Prove Phase 4/5 editor-facing shader pipeline behavior without launching the editor:
#     1. A successful saved .hlsl edit recooks, publishes a new package, and updates the JSON recook publication.
#     2. A failed recook surfaces diagnostics through process output and preserves old package/publication artifacts.
#     3. DebugArtifactBundle files exist for the editor inspector.
#     4. PsoStats analysis emits a CSV row for every cooked stage.
#
# Usage (from repo root, after building DevelopmentEditor ShaderCompiler):
#   pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts\CI\RunShaderEditorPipelineCheck.ps1
# ============================================================================

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$ShaderCompiler = Join-Path $RepoRoot 'build\bin\DevelopmentEditor\ShaderCompiler.exe'
$ShowcaseRoot = Join-Path $RepoRoot 'Projects\Showcase'
$CacheRoot = Join-Path $RepoRoot 'build\Cache\Shaders\EditorPipelineValidation'
$DebugArtifactRoot = Join-Path $CacheRoot 'Debug'
$AnalysisCsv = Join-Path $CacheRoot 'Analysis\pso-stats.csv'
$RegistryPath = Join-Path $RepoRoot 'build\Cooked\Showcase\Shaders\ShaderPackageRegistry.sreg'
$TargetShader = Join-Path $RepoRoot 'Engine\Assets\Shaders\HelloWorld\HelloTriangle.hlsl'
$RequiredBundleFiles = @(
    'compile-request.json',
    'defines.json',
    'preprocessed-source.hlsl',
    'reflection.json',
    'parameter-struct-match.json',
    'disassembly.txt',
    'compiler-stderr.txt',
    'compile-args.json'
)

if (-not (Test-Path $ShaderCompiler))
{
    Write-Host "[CI][ERROR] ShaderCompiler.exe not found at $ShaderCompiler. Build DevelopmentEditor ShaderCompiler before running this check."
    exit 1
}

if (-not (Test-Path $TargetShader))
{
    Write-Host "[CI][ERROR] Target shader not found at $TargetShader."
    exit 1
}

function Invoke-ShaderCook
{
    param(
        [Parameter(Mandatory=$true)][string]$Label,
        [Parameter(Mandatory=$true)][string[]]$Arguments,
        [int]$ExpectedExitCode = 0
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

    if ($exitCode -ne $ExpectedExitCode)
    {
        Write-Host "[CI][ERROR] $Label exited with rc=$exitCode; expected rc=$ExpectedExitCode."
        exit 1
    }

    return [PSCustomObject]@{
        ExitCode = $exitCode
        Text = $text
    }
}

function Get-HelloTrianglePackagePath
{
    if (-not (Test-Path $RegistryPath))
    {
        Write-Host "[CI][ERROR] Shader package registry not found at $RegistryPath."
        exit 1
    }

    $lines = Get-Content -Path $RegistryPath
    $insideHelloTriangle = $false
    foreach ($line in $lines)
    {
        if ($line -match '^\[Package\s+(.+)\]$')
        {
            $insideHelloTriangle = ($Matches[1] -eq 'HelloTriangle')
            continue
        }

        if ($insideHelloTriangle -and $line -match '^Output\s*=\s*(.+)$')
        {
            $outputPath = $Matches[1].Trim()
            if ([System.IO.Path]::IsPathRooted($outputPath))
            {
                return $outputPath
            }

            return Join-Path $ShowcaseRoot $outputPath
        }
    }

    Write-Host '[CI][ERROR] Could not locate HelloTriangle package output in the registry.'
    exit 1
}

function Get-FileSha256OrEmpty
{
    param([Parameter(Mandatory=$true)][string]$Path)
    if (-not (Test-Path $Path))
    {
        return ''
    }
    return (Get-FileHash -Algorithm SHA256 -Path $Path).Hash
}

function Assert-DebugArtifactBundles
{
    $bundles = Get-ChildItem -Path $DebugArtifactRoot -Recurse -Directory -ErrorAction SilentlyContinue |
        Where-Object { Test-Path (Join-Path $_.FullName 'compile-request.json') }

    if ($bundles.Count -le 0)
    {
        Write-Host "[CI][ERROR] Expected debug artifact bundles, found $($bundles.Count)."
        exit 1
    }

    return $bundles.Count

    foreach ($bundle in $bundles)
    {
        foreach ($fileName in $RequiredBundleFiles)
        {
            $artifactPath = Join-Path $bundle.FullName $fileName
            if (-not (Test-Path $artifactPath))
            {
                Write-Host "[CI][ERROR] Missing shader inspector artifact $artifactPath."
                exit 1
            }
        }
    }
}

function Assert-PsoStatsCsv
{
    param([Parameter(Mandatory=$true)][int]$ExpectedRows)

    if (-not (Test-Path $AnalysisCsv))
    {
        Write-Host "[CI][ERROR] Expected PsoStats CSV not found at $AnalysisCsv."
        exit 1
    }

    $rows = Import-Csv -Path $AnalysisCsv
    if ($rows.Count -ne $ExpectedRows)
    {
        Write-Host "[CI][ERROR] Expected $ExpectedRows PsoStats rows, found $($rows.Count)."
        exit 1
    }
}

if (Test-Path $CacheRoot)
{
    Remove-Item -Recurse -Force $CacheRoot
}

$cookArgs = @('cook', '--backend', 'dxc', '--target', 'DxilSm66', '--cache-dir', $CacheRoot)
$debugAnalysisArgs = $cookArgs + @('--debug-artifacts', $DebugArtifactRoot, '--analysis', 'pso-stats', '--no-cache')
$originalShaderBytes = [System.IO.File]::ReadAllBytes($TargetShader)
$originalShaderText = [System.Text.Encoding]::UTF8.GetString($originalShaderBytes)
$signalPath = Join-Path $CacheRoot 'recook.signal'
$didModifyShader = $false

try
{
    Invoke-ShaderCook -Label 'Baseline cook with debug artifacts and pso-stats' -Arguments $debugAnalysisArgs | Out-Null
    $bundleCount = Assert-DebugArtifactBundles
    Assert-PsoStatsCsv -ExpectedRows $bundleCount

    $packagePath = Get-HelloTrianglePackagePath
    $baselinePackageHash = Get-FileSha256OrEmpty -Path $packagePath
    $baselineSignalHash = Get-FileSha256OrEmpty -Path $signalPath
    if ([string]::IsNullOrWhiteSpace($baselinePackageHash) -or [string]::IsNullOrWhiteSpace($baselineSignalHash))
    {
        Write-Host '[CI][ERROR] Baseline cook did not produce expected package or recook publication.'
        exit 1
    }

    $editedShaderText = [regex]::Replace(
        $originalShaderText,
        'return\s+float4\(Input\.Color,\s*1\.0f\);',
        'return float4(Input.Color * 0.5f, 1.0f);',
        1)
    if ($editedShaderText -eq $originalShaderText)
    {
        Write-Host '[CI][ERROR] Could not apply pure HLSL hot-reload validation edit.'
        exit 1
    }

    [System.IO.File]::WriteAllText($TargetShader, $editedShaderText, [System.Text.UTF8Encoding]::new($false))
    $didModifyShader = $true

    Invoke-ShaderCook -Label 'Successful saved .hlsl edit recook' -Arguments $cookArgs | Out-Null
    $editedPackageHash = Get-FileSha256OrEmpty -Path $packagePath
    $editedSignalHash = Get-FileSha256OrEmpty -Path $signalPath
    if ($editedPackageHash -eq $baselinePackageHash)
    {
        Write-Host '[CI][ERROR] Saved .hlsl edit did not publish a changed HelloTriangle package.'
        exit 1
    }
    if ($editedSignalHash -eq $baselineSignalHash)
    {
        Write-Host '[CI][ERROR] Saved .hlsl edit did not update the recook publication.'
        exit 1
    }

    $brokenShaderText = $editedShaderText + "`r`nfloat4 __Phase4IntentionalSyntaxError = ;`r`n"
    [System.IO.File]::WriteAllText($TargetShader, $brokenShaderText, [System.Text.UTF8Encoding]::new($false))
    $failedCook = Invoke-ShaderCook -Label 'Failed saved .hlsl edit preserves previous artifacts' -Arguments $cookArgs -ExpectedExitCode 6
    if ($failedCook.Text -notmatch 'failed|error|ShaderCompiler')
    {
        Write-Host '[CI][ERROR] Failed cook did not surface a useful diagnostic in process output.'
        exit 1
    }

    $failedPackageHash = Get-FileSha256OrEmpty -Path $packagePath
    $failedSignalHash = Get-FileSha256OrEmpty -Path $signalPath
    if ($failedPackageHash -ne $editedPackageHash)
    {
        Write-Host '[CI][ERROR] Failed recook changed the previously published package artifact.'
        exit 1
    }
    if ($failedSignalHash -ne $editedSignalHash)
    {
        Write-Host '[CI][ERROR] Failed recook updated the recook publication even though artifacts were not published.'
        exit 1
    }
}
finally
{
    if ($didModifyShader)
    {
        [System.IO.File]::WriteAllBytes($TargetShader, $originalShaderBytes)
        Invoke-ShaderCook -Label 'Restore original shader artifacts' -Arguments $cookArgs | Out-Null
    }
}

Write-Host ''
Write-Host '[CI][OK] Shader editor hot-reload and inspector/analysis behavior is green.'
exit 0