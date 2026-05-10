param(
    [Parameter(Mandatory = $true)]
    [string]$RootDir,

    [Parameter(Mandatory = $true)]
    [string]$BuildDir,

    [Parameter(Mandatory = $true)]
    [string]$SolutionFile,

    [Parameter(Mandatory = $true)]
    [string]$Generator,

    [Parameter(Mandatory = $true)]
    [string]$Platform,

    [AllowEmptyString()]
    [string]$Toolset,

    [switch]$UpdateStamp
)

$ErrorActionPreference = 'Stop'

function Get-RelativePathStable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $baseUri = [System.Uri]::new((Resolve-Path -LiteralPath $BasePath).Path.TrimEnd('\') + '\')
    $pathUri = [System.Uri]::new((Resolve-Path -LiteralPath $Path).Path)
    return [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($pathUri).ToString()).Replace('/', '\').ToLowerInvariant()
}

function Get-CacheValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CachePath,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $prefix = "${Name}:INTERNAL="
    $line = Get-Content -LiteralPath $CachePath | Where-Object { $_.StartsWith($prefix, [System.StringComparison]::Ordinal) } | Select-Object -First 1
    if ($null -eq $line) {
        return ''
    }

    return $line.Substring($prefix.Length)
}

function Get-BuildInputPaths {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $buildInputFiles = New-Object System.Collections.Generic.List[string]
    $explicitFiles = @(
        'CMakeLists.txt',
        'Scripts\Internal\Config.bat',
        'Scripts\Internal\CMakeHelpers.bat'
    )

    foreach ($relativePath in $explicitFiles) {
        $path = Join-Path $Root $relativePath
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $buildInputFiles.Add($path)
        }
    }

    foreach ($directory in @('CMake', 'Engine', 'Tools', 'Projects')) {
        $path = Join-Path $Root $directory
        if (-not (Test-Path -LiteralPath $path -PathType Container)) {
            continue
        }

        Get-ChildItem -LiteralPath $path -Recurse -File -Force |
            Where-Object {
                $_.Name -eq 'CMakeLists.txt' -or
                $_.Extension -eq '.cmake' -or
                $_.Name -eq '.sparkle-project'
            } |
            ForEach-Object { $buildInputFiles.Add($_.FullName) }
    }

    return $buildInputFiles
}

function Get-SourceListSignature {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $sourceExtensions = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($extension in @('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inl', '.ixx')) {
        [void]$sourceExtensions.Add($extension)
    }

    $relativePaths = New-Object System.Collections.Generic.List[string]
    foreach ($directory in @('Engine', 'Tools', 'Projects')) {
        $path = Join-Path $Root $directory
        if (-not (Test-Path -LiteralPath $path -PathType Container)) {
            continue
        }

        Get-ChildItem -LiteralPath $path -Recurse -File -Force |
            Where-Object {
                $sourceExtensions.Contains($_.Extension) -and
                $_.FullName -notlike '*\third_party\*'
            } |
            ForEach-Object { $relativePaths.Add((Get-RelativePathStable -BasePath $Root -Path $_.FullName)) }
    }

    $ordered = $relativePaths | Sort-Object
    return ($ordered -join "`n")
}

function Get-Sha256Hex {
    param([AllowEmptyString()][string]$Text)

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        return ([System.BitConverter]::ToString($sha256.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
}

$root = (Resolve-Path -LiteralPath $RootDir).Path
$build = if (Test-Path -LiteralPath $BuildDir) { (Resolve-Path -LiteralPath $BuildDir).Path } else { $BuildDir }
$cachePath = Join-Path $build 'CMakeCache.txt'
$stampPath = Join-Path $build 'BuildFilesFreshness.json'

if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
    Write-Host '[LOG] Build files stale: CMake cache missing.'
    exit 1
}

if (-not (Test-Path -LiteralPath $SolutionFile -PathType Leaf)) {
    Write-Host '[LOG] Build files stale: solution file missing.'
    exit 1
}

$cacheGenerator = Get-CacheValue -CachePath $cachePath -Name 'CMAKE_GENERATOR'
$cachePlatform = Get-CacheValue -CachePath $cachePath -Name 'CMAKE_GENERATOR_PLATFORM'
$cacheToolset = Get-CacheValue -CachePath $cachePath -Name 'CMAKE_GENERATOR_TOOLSET'

if ($cacheGenerator -ine $Generator -or $cachePlatform -ine $Platform -or $cacheToolset -ine $Toolset) {
    Write-Host "[LOG] Build files stale: generator/platform/toolset changed."
    Write-Host "[LOG] Existing: generator='$cacheGenerator' platform='$cachePlatform' toolset='$cacheToolset'"
    Write-Host "[LOG] Desired:  generator='$Generator' platform='$Platform' toolset='$Toolset'"
    exit 1
}

$buildInputFiles = Get-BuildInputPaths -Root $root
$sourceListSignature = Get-SourceListSignature -Root $root
$sourceListHash = Get-Sha256Hex -Text $sourceListSignature

if ($UpdateStamp) {
    $stamp = [ordered]@{
        version = 1
        generator = $Generator
        platform = $Platform
        toolset = $Toolset
        sourceListHash = $sourceListHash
        updatedUtc = [System.DateTime]::UtcNow.ToString('o')
    }

    $stamp | ConvertTo-Json | Set-Content -LiteralPath $stampPath -Encoding UTF8
    exit 0
}

if (-not (Test-Path -LiteralPath $stampPath -PathType Leaf)) {
    Write-Host '[LOG] Build files stale: freshness stamp missing.'
    exit 1
}

$stamp = Get-Content -LiteralPath $stampPath -Raw | ConvertFrom-Json
$stampItem = Get-Item -LiteralPath $stampPath

if ($stamp.generator -ine $Generator -or $stamp.platform -ine $Platform -or $stamp.toolset -ine $Toolset) {
    Write-Host '[LOG] Build files stale: freshness stamp generator/platform/toolset changed.'
    exit 1
}

if ($stamp.sourceListHash -ne $sourceListHash) {
    Write-Host '[LOG] Build files stale: source file list changed.'
    exit 1
}

$newerInput = $buildInputFiles |
    Where-Object { (Get-Item -LiteralPath $_).LastWriteTimeUtc -gt $stampItem.LastWriteTimeUtc } |
    Select-Object -First 1

if ($null -ne $newerInput) {
    $relativePath = Get-RelativePathStable -BasePath $root -Path $newerInput
    Write-Host "[LOG] Build files stale: build input changed '$relativePath'."
    exit 1
}

Write-Host '[LOG] Build files are current. Skipping GenerateSolution.'
exit 0