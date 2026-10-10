[CmdletBinding()]
param(
    [ValidateSet("Check", "Format")]
    [string] $Mode = "Check",
    [ValidateSet("All", "Cpp", "Shaders")]
    [string] $SourceFamily = "All",
    [switch] $Staged,
    [string] $Path = "",
    [string] $ClangFormatPath = "",
    [string] $ClangTidyPath = ""
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

function Resolve-ClangTidyExecutable
{
    param([string] $ConfiguredPath)

    $candidates = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($ConfiguredPath))
    {
        $candidates.Add($ConfiguredPath)
    }

    $environmentPath = [Environment]::GetEnvironmentVariable("SPARKLE_CLANG_TIDY")
    if (-not [string]::IsNullOrWhiteSpace($environmentPath))
    {
        $candidates.Add($environmentPath)
    }

    $pathCommand = Get-Command clang-tidy -ErrorAction SilentlyContinue
    if ($null -ne $pathCommand)
    {
        $candidates.Add($pathCommand.Source)
    }

    foreach ($visualStudioVersion in @("18", "17"))
    {
        foreach ($visualStudioEdition in @("Community", "Professional", "Enterprise", "BuildTools"))
        {
            $candidates.Add(
                "C:\Program Files\Microsoft Visual Studio\$visualStudioVersion\$visualStudioEdition\VC\Tools\Llvm\x64\bin\clang-tidy.exe")
        }
    }

    foreach ($candidate in $candidates)
    {
        if (Test-Path -LiteralPath $candidate -PathType Leaf)
        {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "clang-tidy $requiredClangFormatVersion was not found. Pass -ClangTidyPath or set SPARKLE_CLANG_TIDY."
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

function ConvertTo-CanonicalShaderText
{
    param([string] $Text)

    $canonicalText = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    $canonicalText = [regex]::Replace(
        $canonicalText,
        '^(?<Indent>[\t ]*)}\s*//\s*namespace(?:\s+\S+)?\s*$',
        '${Indent}}',
        [Text.RegularExpressions.RegexOptions]::Multiline)
    $canonicalText = [regex]::Replace(
        $canonicalText,
        '^[\t ]*(?<Attribute>\[(?:shader|numthreads)[^\]\r\n]*\])[\t ]+(?<Code>\S[^\r\n]*)$',
        '${Attribute}' + "`n" + '${Code}',
        [Text.RegularExpressions.RegexOptions]::Multiline)
    $canonicalText = [regex]::Replace(
        $canonicalText,
        '^(?<Indent>[\t ]*)(?<Attribute>\[[^\]\r\n]+\])[\t ]+(?<Code>\S[^\r\n]*)$',
        '${Indent}${Attribute}' + "`n" + '${Indent}${Code}',
        [Text.RegularExpressions.RegexOptions]::Multiline)

    $lines = $canonicalText.Split([string[]] @("`n"), [StringSplitOptions]::None)
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; ++$lineIndex)
    {
        if ($lines[$lineIndex] -notmatch '^\[(?:shader|numthreads)')
        {
            continue
        }

        $signatureLineIndex = $lineIndex + 1
        if ($signatureLineIndex -ge $lines.Count)
        {
            continue
        }

        $lines[$signatureLineIndex] = $lines[$signatureLineIndex].TrimStart()
        for (++$signatureLineIndex; $signatureLineIndex -lt $lines.Count -and $lines[$signatureLineIndex].Trim() -ne "{"; ++$signatureLineIndex)
        {
            $lines[$signatureLineIndex] = "`t" + $lines[$signatureLineIndex].TrimStart()
        }
    }

    $canonicalText = $lines -join "`n"

    return $canonicalText
}

function Get-SelectedSourcePaths
{
    if ($Staged -and -not [string]::IsNullOrWhiteSpace($Path))
    {
        throw "Use either -Staged or -Path, not both."
    }

    if (-not [string]::IsNullOrWhiteSpace($Path))
    {
        $absolutePath = [IO.Path]::GetFullPath((Join-Path $repositoryRoot $Path))
        $rootPrefix = $repositoryRoot + [IO.Path]::DirectorySeparatorChar
        if (-not $absolutePath.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase))
        {
            throw "-Path must identify a tracked owned source file inside the repository."
        }

        return $absolutePath.Substring($rootPrefix.Length).Replace('\', '/')
    }

    if ($Staged)
    {
        $selectedPaths = @(& git -C $repositoryRoot -c core.quotepath=false diff --cached --name-only --diff-filter=ACMR -- Engine Tools Projects)
        if ($LASTEXITCODE -ne 0)
        {
            throw "Failed to enumerate staged source files."
        }

        return $selectedPaths
    }

    $selectedPaths = @(& git -C $repositoryRoot -c core.quotepath=false ls-files -- Engine Tools Projects)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Failed to enumerate tracked source files."
    }

    return $selectedPaths
}

function Test-ClangFormatChanges
{
    param([string[]] $Paths, [string] $ReplacementXml)

    $documents = @([regex]::Split($ReplacementXml, '(?=<\?xml)') | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($documents.Count -ne $Paths.Count)
    {
        throw "clang-format returned an unexpected replacement-document count."
    }

    $changed = $false
    for ($fileIndex = 0; $fileIndex -lt $Paths.Count; ++$fileIndex)
    {
        $relativePath = $Paths[$fileIndex]
        $absolutePath = Join-Path $repositoryRoot $relativePath
        $bytes = [IO.File]::ReadAllBytes($absolutePath)
        $document = [xml] $documents[$fileIndex]
        if ($document.replacements.incomplete_format -eq "true")
        {
            throw "${relativePath}: clang-format could not complete formatting."
        }

        foreach ($replacement in $document.SelectNodes('/replacements/replacement'))
        {
            $offset = [int] $replacement.offset
            $length = [int] $replacement.length
            $existing = [Text.Encoding]::UTF8.GetString($bytes, $offset, $length)
            if ($existing -ceq $replacement.InnerText)
            {
                continue
            }

            [Console]::Error.WriteLine("${relativePath}: code should be clang-formatted")
            $changed = $true
            break
        }
    }

    return $changed
}

$selectedSourcePaths = @(Get-SelectedSourcePaths)
$ownedSourcePaths = @(
    $selectedSourcePaths | Where-Object {
        $absolutePath = Join-Path $repositoryRoot ($_ -replace "/", [IO.Path]::DirectorySeparatorChar)
        $extension = [IO.Path]::GetExtension($_)
        $isShader = $extension -in @(".hlsl", ".hlsli")
        $matchesSourceFamily = $SourceFamily -eq "All" -or ($SourceFamily -eq "Shaders" -and $isShader) -or
            ($SourceFamily -eq "Cpp" -and -not $isShader)
        $ownedExtensions.Contains($extension) -and
        $matchesSourceFamily -and
        $_ -match '^(Engine|Tools|Projects)/' -and
        -not $_.StartsWith("Engine/RHI/Private/D3D12/ThirdParty/", [StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $absolutePath -PathType Leaf)
    } | Sort-Object -Unique
)

if (-not [string]::IsNullOrWhiteSpace($Path))
{
    & git -C $repositoryRoot ls-files --error-unmatch -- $selectedSourcePaths[0] 1>$null 2>$null
    if ($LASTEXITCODE -ne 0 -or $ownedSourcePaths.Count -ne 1)
    {
        throw "-Path must identify an existing tracked owned source file matching -SourceFamily."
    }
}

if ($ownedSourcePaths.Count -eq 0)
{
    if ($Staged)
    {
        Write-Host "No staged owned source files match the selected source family."
        exit 0
    }

    throw "The tracked owned-source manifest is empty."
}

$clangFormat = Resolve-ClangFormatExecutable -ConfiguredPath $ClangFormatPath
$clangFormatVersion = (& $clangFormat --version 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $clangFormatVersion -notmatch "version $([regex]::Escape($requiredClangFormatVersion))(?:\s|$)")
{
    throw "Expected clang-format $requiredClangFormatVersion, got '$clangFormatVersion'."
}

$clangTidy = $null
if ($Mode -eq "Check" -and $SourceFamily -ne "Shaders")
{
    $clangTidy = Resolve-ClangTidyExecutable -ConfiguredPath $ClangTidyPath
    $clangTidyVersion = (& $clangTidy --version 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $clangTidyVersion -notmatch "version $([regex]::Escape($requiredClangFormatVersion))(?:\s|$)")
    {
        throw "Expected clang-tidy $requiredClangFormatVersion, got '$clangTidyVersion'."
    }

    Push-Location $repositoryRoot
    try
    {
        & $clangTidy --verify-config
        if ($LASTEXITCODE -ne 0)
        {
            throw ".clang-tidy is invalid for clang-tidy $requiredClangFormatVersion."
        }
    }
    finally
    {
        Pop-Location
    }
}

$ownedCppPaths = @($ownedSourcePaths | Where-Object { [IO.Path]::GetExtension($_) -notin @(".hlsl", ".hlsli") })
$ownedShaderPaths = @($ownedSourcePaths | Where-Object { [IO.Path]::GetExtension($_) -in @(".hlsl", ".hlsli") })

Add-Type -Path (Join-Path $PSScriptRoot "CodeStyleParagraphSpacing.cs")

$formatFailed = $false
Push-Location $repositoryRoot
try
{
    for ($offset = 0; $offset -lt $ownedCppPaths.Count; $offset += 64)
    {
        $lastIndex = [Math]::Min($offset + 63, $ownedCppPaths.Count - 1)
        $batch = $ownedCppPaths[$offset..$lastIndex]
        $arguments = if ($Mode -eq "Check")
        {
            @("--output-replacements-xml", "--Werror", "--style=file") + $batch
        }
        else
        {
            @("-i", "--Werror", "--style=file") + $batch
        }

        if ($Mode -eq "Check")
        {
            $replacementLines = @(& $clangFormat $arguments)
            if ($LASTEXITCODE -ne 0)
            {
                $formatFailed = $true
            }
            elseif (Test-ClangFormatChanges -Paths $batch -ReplacementXml ($replacementLines -join "`n"))
            {
                $formatFailed = $true
            }
        }
        else
        {
            & $clangFormat $arguments
            if ($LASTEXITCODE -ne 0)
            {
                $formatFailed = $true
            }
        }
    }

    $utf8WithoutBom = [Text.UTF8Encoding]::new($false)
    foreach ($relativePath in $ownedCppPaths)
    {
        $absolutePath = Join-Path $repositoryRoot ($relativePath -replace "/", [IO.Path]::DirectorySeparatorChar)
        $currentText = [IO.File]::ReadAllText($absolutePath)
        $canonicalText = [CodeStyleParagraphSpacing]::Format($currentText)
        if ($currentText.Replace("`r`n", "`n").Replace("`r", "`n") -ceq $canonicalText)
        {
            continue
        }

        if ($Mode -eq "Format")
        {
            [IO.File]::WriteAllText($absolutePath, $canonicalText, $utf8WithoutBom)
        }
        else
        {
            [Console]::Error.WriteLine("${relativePath}: multiline statements require blank paragraph boundaries")
            $formatFailed = $true
        }
    }

    foreach ($relativePath in $ownedShaderPaths)
    {
        $absolutePath = Join-Path $repositoryRoot ($relativePath -replace "/", [IO.Path]::DirectorySeparatorChar)
        $formattedLines = @(& $clangFormat --Werror --style=file $absolutePath)
        if ($LASTEXITCODE -ne 0)
        {
            $formatFailed = $true
            continue
        }

        $formattedText = ($formattedLines -join "`n") + "`n"
        $canonicalText = ConvertTo-CanonicalShaderText -Text $formattedText
        $canonicalText = [CodeStyleParagraphSpacing]::Format($canonicalText)
        $currentText = ([IO.File]::ReadAllText($absolutePath)).Replace("`r`n", "`n").Replace("`r", "`n")
        if ($currentText -ceq $canonicalText)
        {
            continue
        }

        if ($Mode -eq "Format")
        {
            [IO.File]::WriteAllText($absolutePath, $canonicalText, $utf8WithoutBom)
        }
        else
        {
            [Console]::Error.WriteLine("${relativePath}: code should be formatted with the canonical shader policy")
            $formatFailed = $true
        }
    }
}
finally
{
    Pop-Location
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

foreach ($violation in $policyViolations)
{
    [Console]::Error.WriteLine($violation)
}

if ($formatFailed -or $policyViolations.Count -gt 0)
{
    throw "Code-style $($Mode.ToLowerInvariant()) failed: formatterFailure=$formatFailed; policyViolations=$($policyViolations.Count)."
}

$semanticPolicySummary = if ($null -ne $clangTidy) { "; .clang-tidy configuration verified" } else { "" }
Write-Host "Code-style $($Mode.ToLowerInvariant()) passed for $($ownedSourcePaths.Count) tracked owned $($SourceFamily.ToLowerInvariant()) files with clang-format $requiredClangFormatVersion$semanticPolicySummary."
