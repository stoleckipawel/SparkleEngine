# Purpose:
#   Prove on every PR that the shader cooker baseline is green:
#     1. CMake configures.
#     2. ShaderCompiler builds in Debug AND Release.
#     3. ValidateShaderCompilerBoundary passes (custom target runs as part
#        of the build).
#     4. ShaderCompiler.exe exposes reviewer-friendly backend, target, shader,
#        package inspection, and negative-test diagnostics.
#     5. ShaderCompiler.exe cook succeeds on the Showcase project.
#
#
# Usage (from repo root):
#   pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts\CI\RunShaderCompilerCookCheck.ps1
#
# Exit codes:
#   0  baseline green
#   non-zero  configure / build / validator / cook failed
# ============================================================================

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $RepoRoot

function Invoke-CIStep
{
    param(
        [Parameter(Mandatory=$true)][string]$Label,
        [Parameter(Mandatory=$true)][scriptblock]$Action
    )

    Write-Host ''
    Write-Host "[CI] $Label"
    & $Action
    if ($LASTEXITCODE -ne 0)
    {
        Write-Host "[CI][ERROR] $Label failed (rc=$LASTEXITCODE)."
        exit $LASTEXITCODE
    }
}

function Get-RegistryPackageOutput
{
    param(
        [Parameter(Mandatory=$true)][string]$RegistryPath,
        [Parameter(Mandatory=$true)][string]$ProjectRoot,
        [Parameter(Mandatory=$true)][string]$PackageId
    )

    if (-not (Test-Path $RegistryPath))
    {
        Write-Host "[CI][ERROR] Shader package registry not found at $RegistryPath."
        exit 1
    }

    $insidePackage = $false
    foreach ($line in Get-Content -Path $RegistryPath)
    {
        if ($line -match '^\[Package\s+(.+)\]$')
        {
            $insidePackage = ($Matches[1] -eq $PackageId)
            continue
        }

        if ($insidePackage -and $line -match '^Output\s*=\s*(.+)$')
        {
            $outputPath = $Matches[1].Trim()
            if ([System.IO.Path]::IsPathRooted($outputPath))
            {
                return $outputPath
            }

            return Join-Path $ProjectRoot $outputPath
        }
    }

    Write-Host "[CI][ERROR] Could not locate package '$PackageId' in $RegistryPath."
    exit 1
}

function Invoke-NegativeCIStep
{
    param(
        [Parameter(Mandatory=$true)][string]$Label,
        [Parameter(Mandatory=$true)][scriptblock]$Action
    )

    Write-Host ''
    Write-Host "[CI] $Label"
    & $Action
    $actualExitCode = $LASTEXITCODE
    if ($actualExitCode -eq 0)
    {
        Write-Host "[CI][ERROR] $Label unexpectedly succeeded."
        exit 1
    }
    Write-Host "[CI][OK] $Label failed as expected (rc=$actualExitCode)."
    $global:LASTEXITCODE = 0
}

# 1. Configure CMake if needed.
if (-not (Test-Path (Join-Path $RepoRoot 'build')))
{
    Invoke-CIStep -Label 'Configuring CMake (Visual Studio 17 2022, x64)' -Action {
        cmake -S . -B build -G 'Visual Studio 17 2022' -A x64
    }
}

# 2. Build Debug + Release. The boundary validator runs as part of the build.
foreach ($config in @('Debug', 'Release'))
{
    Invoke-CIStep -Label "Building ShaderCompiler ($config)" -Action {
        cmake --build build --config $config --target ShaderCompiler -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false
    }
}

# 3. Cook the Showcase project using the Debug binary.
$ShaderCompiler = Join-Path $RepoRoot 'build\bin\Debug\ShaderCompiler.exe'
if (-not (Test-Path $ShaderCompiler))
{
    Write-Host "[CI][ERROR] ShaderCompiler.exe not found at $ShaderCompiler."
    exit 1
}

$ShowcaseRoot = Join-Path $RepoRoot 'Projects\Showcase'
Push-Location $ShowcaseRoot
try
{
    Invoke-CIStep -Label 'Listing shader backends' -Action {
        & $ShaderCompiler list-backends
    }

    Invoke-CIStep -Label 'Listing shader targets' -Action {
        & $ShaderCompiler list-targets
    }

    Invoke-CIStep -Label 'Validating typed shader registrations' -Action {
        & $ShaderCompiler list-shaders --validate
    }

    Invoke-CIStep -Label 'Inspecting HelloTriangle shader registration' -Action {
        & $ShaderCompiler inspect-shader HelloTriangle
    }

    Invoke-CIStep -Label 'Cooking shaders for Showcase project' -Action {
        & $ShaderCompiler cook --no-cache --backend dxc --target DxilSm66
    }

    Invoke-CIStep -Label 'Cooking HelloTriangle multi-format package' -Action {
        & $ShaderCompiler cook --no-cache --backend dxc --target DxilSm66 --target SpirV16 --shader HelloTriangle
    }

    $registryPath = Join-Path $RepoRoot 'build\Cooked\Showcase\Shaders\ShaderPackageRegistry.sreg'
    $helloTrianglePackage = Get-RegistryPackageOutput -RegistryPath $registryPath -ProjectRoot $ShowcaseRoot -PackageId 'HelloTriangle'
    Invoke-CIStep -Label 'Inspecting HelloTriangle cooked package' -Action {
        & $ShaderCompiler inspect-package $helloTrianglePackage
    }

    Invoke-NegativeCIStep -Label 'Running parameter mismatch negative test' -Action {
        & $ShaderCompiler cook --verification-self-test parameter-mismatch
    }
}
finally
{
    Pop-Location
}

Write-Host ''
Write-Host '[CI][OK] Shader cooker baseline is green.'
exit 0
