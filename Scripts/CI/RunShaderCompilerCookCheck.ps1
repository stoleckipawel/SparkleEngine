# Purpose:
#   Prove on every PR that the shader cooker baseline is green:
#     1. CMake configures.
#     2. ShaderCompiler builds in Debug AND Release.
#     3. ValidateShaderCompilerBoundary passes (custom target runs as part
#        of the build).
#     4. ShaderCompiler.exe cook succeeds on the Showcase project.
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
        cmake --build build --config $config --target ShaderCompiler
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
    Invoke-CIStep -Label 'Cooking shaders for Showcase project' -Action {
        & $ShaderCompiler cook --no-cache
    }
}
finally
{
    Pop-Location
}

Write-Host ''
Write-Host '[CI][OK] Shader cooker baseline is green.'
exit 0
