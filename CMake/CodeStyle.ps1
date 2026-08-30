[CmdletBinding()]
param(
    [ValidateSet("Check", "Format")]
    [string] $Mode = "Check",
    [ValidateSet("All", "Cpp", "Shaders")]
    [string] $SourceFamily = "All",
    [string] $ClangFormatPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$requiredClangFormatVersion = "22.1.3"
$ownedExtensions = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
@(".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".hlsl", ".hlsli") | ForEach-Object {
    [void] $ownedExtensions.Add($_)
}

function Resolve-ClangFormatExecutable
{
    param([string] $ConfiguredPath)

    $candidates = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($ConfiguredPath))
    {
        $candidates.Add($ConfiguredPath)
    }

    $environmentPath = [Environment]::GetEnvironmentVariable("SPARKLE_CLANG_FORMAT")
    if (-not [string]::IsNullOrWhiteSpace($environmentPath))
    {
        $candidates.Add($environmentPath)
    }

    $pathCommand = Get-Command clang-format -ErrorAction SilentlyContinue
    if ($null -ne $pathCommand)
    {
        $candidates.Add($pathCommand.Source)
    }

    foreach ($visualStudioVersion in @("18", "17"))
    {
        foreach ($visualStudioEdition in @("Community", "Professional", "Enterprise", "BuildTools"))
        {
            $candidates.Add(
                "C:\Program Files\Microsoft Visual Studio\$visualStudioVersion\$visualStudioEdition\VC\Tools\Llvm\x64\bin\clang-format.exe")
        }
    }

    foreach ($candidate in $candidates)
    {
        if (Test-Path -LiteralPath $candidate -PathType Leaf)
        {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "clang-format $requiredClangFormatVersion was not found. Pass -ClangFormatPath or set SPARKLE_CLANG_FORMAT."
}

function Test-TopLevelComma
{
    param([string] $Text)

    $angleDepth = 0
    $parenthesisDepth = 0
    $bracketDepth = 0
    foreach ($character in $Text.ToCharArray())
    {
        switch ($character)
        {
            "<" { ++$angleDepth; continue }
            ">" { if ($angleDepth -gt 0) { --$angleDepth }; continue }
            "(" { ++$parenthesisDepth; continue }
            ")" { if ($parenthesisDepth -gt 0) { --$parenthesisDepth }; continue }
            "[" { ++$bracketDepth; continue }
            "]" { if ($bracketDepth -gt 0) { --$bracketDepth }; continue }
            ","
            {
                if ($angleDepth -eq 0 -and $parenthesisDepth -eq 0 -and $bracketDepth -eq 0)
                {
                    return $true
                }
            }
        }
    }

    return $false
}

function Add-PolicyViolation
{
    param(
        [System.Collections.Generic.List[string]] $Violations,
        [string] $RelativePath,
        [int] $LineNumber,
        [string] $Message
    )

    $Violations.Add("${RelativePath}:${LineNumber}: $Message")
}

$clangFormat = Resolve-ClangFormatExecutable -ConfiguredPath $ClangFormatPath
$clangFormatVersion = (& $clangFormat --version 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $clangFormatVersion -notmatch "version $([regex]::Escape($requiredClangFormatVersion))(?:\s|$)")
{
    throw "Expected clang-format $requiredClangFormatVersion, got '$clangFormatVersion'."
}

$trackedPaths = @(& git -C $repositoryRoot ls-files -- Engine Tools Projects)
if ($LASTEXITCODE -ne 0)
{
    throw "Failed to enumerate tracked source files."
}

$ownedSourcePaths = @(
    $trackedPaths | Where-Object {
        $absolutePath = Join-Path $repositoryRoot ($_ -replace "/", [IO.Path]::DirectorySeparatorChar)
        $extension = [IO.Path]::GetExtension($_)
        $isShader = $extension -in @(".hlsl", ".hlsli")
        $matchesSourceFamily = $SourceFamily -eq "All" -or ($SourceFamily -eq "Shaders" -and $isShader) -or
            ($SourceFamily -eq "Cpp" -and -not $isShader)
        $ownedExtensions.Contains($extension) -and
        $matchesSourceFamily -and
        -not $_.StartsWith("Engine/RHI/Private/D3D12/ThirdParty/", [StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $absolutePath -PathType Leaf)
    } | Sort-Object
)
if ($ownedSourcePaths.Count -eq 0)
{
    throw "The tracked owned-source manifest is empty."
}

$policyViolations = [System.Collections.Generic.List[string]]::new()
$multipleInheritancePattern = [regex]::new(
    "\b(?:class|struct)\s+[^:;{}]+\s*:\s*(?<Bases>[^;{}]+)\{",
    [Text.RegularExpressions.RegexOptions]::Multiline -bor [Text.RegularExpressions.RegexOptions]::Singleline)

foreach ($relativePath in $ownedSourcePaths)
{
    $absolutePath = Join-Path $repositoryRoot ($relativePath -replace "/", [IO.Path]::DirectorySeparatorChar)
    $lines = @(Get-Content -LiteralPath $absolutePath)
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; ++$lineIndex)
    {
        $line = $lines[$lineIndex]
        if ($line -match "^\s*}\s*//\s*namespace(?:\s+\S+)?\s*$")
        {
            Add-PolicyViolation $policyViolations $relativePath ($lineIndex + 1) "namespace-end comments are prohibited"
        }
        if ([IO.Path]::GetExtension($relativePath) -in @(".hlsl", ".hlsli") -and $line -match "^\s*\[[^\]]+\]\s+\S")
        {
            Add-PolicyViolation $policyViolations $relativePath ($lineIndex + 1) "shader attributes must occupy their own line"
        }
    }

    if ([IO.Path]::GetExtension($relativePath) -notin @(".hlsl", ".hlsli"))
    {
        $sourceText = [IO.File]::ReadAllText($absolutePath)
        $searchableText = [regex]::Replace($sourceText, "/\*.*?\*/", " ", [Text.RegularExpressions.RegexOptions]::Singleline)
        $searchableText = [regex]::Replace($searchableText, "//.*$", "", [Text.RegularExpressions.RegexOptions]::Multiline)
        foreach ($match in [regex]::Matches($searchableText, "\bnamespace\s*\{"))
        {
            $lineNumber = [regex]::Matches($searchableText.Substring(0, $match.Index), "\n").Count + 1
            Add-PolicyViolation $policyViolations $relativePath $lineNumber "anonymous namespaces are prohibited"
        }
        foreach ($match in $multipleInheritancePattern.Matches($searchableText))
        {
            if (Test-TopLevelComma $match.Groups["Bases"].Value)
            {
                $lineNumber = [regex]::Matches($searchableText.Substring(0, $match.Index), "\n").Count + 1
                Add-PolicyViolation $policyViolations $relativePath $lineNumber "owned classes and structs may have at most one direct base"
            }
        }
    }
}

$formatFailed = $false
Push-Location $repositoryRoot
try
{
    for ($offset = 0; $offset -lt $ownedSourcePaths.Count; $offset += 64)
    {
        $lastIndex = [Math]::Min($offset + 63, $ownedSourcePaths.Count - 1)
        $batch = $ownedSourcePaths[$offset..$lastIndex]
        $arguments = if ($Mode -eq "Check")
        {
            @("--dry-run", "--Werror", "--style=file") + $batch
        }
        else
        {
            @("-i", "--Werror", "--style=file") + $batch
        }

        & $clangFormat $arguments
        if ($LASTEXITCODE -ne 0)
        {
            $formatFailed = $true
        }
    }
}
finally
{
    Pop-Location
}

foreach ($violation in $policyViolations)
{
    [Console]::Error.WriteLine($violation)
}

if ($formatFailed -or $policyViolations.Count -gt 0)
{
    throw "Code-style $($Mode.ToLowerInvariant()) failed: formatterFailure=$formatFailed; policyViolations=$($policyViolations.Count)."
}

Write-Host "Code-style $($Mode.ToLowerInvariant()) passed for $($ownedSourcePaths.Count) tracked owned $($SourceFamily.ToLowerInvariant()) files with clang-format $requiredClangFormatVersion."
