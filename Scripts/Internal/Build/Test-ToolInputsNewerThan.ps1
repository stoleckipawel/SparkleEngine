param(
    [Parameter(Mandatory = $true)]
    [string]$ReferencePath,

    [Parameter(Mandatory = $true)]
    [string]$PathList
)

if (-not (Test-Path -LiteralPath $ReferencePath -PathType Leaf)) {
    exit 1
}

$referenceItem = Get-Item -LiteralPath $ReferencePath
$referenceWriteTimeUtc = $referenceItem.LastWriteTimeUtc
$extensions = @('.h', '.hpp', '.cpp', '.cxx', '.ixx', '.inl', '.cmake')
$paths = $PathList.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries)

foreach ($path in $Paths) {
    if (-not (Test-Path -LiteralPath $path)) {
        continue
    }

    $item = Get-Item -LiteralPath $path
    if ($item.PSIsContainer) {
        $hasNewerInput = Get-ChildItem -LiteralPath $path -Recurse -File |
            Where-Object {
                $_.Name -eq 'CMakeLists.txt' -or $extensions -contains $_.Extension.ToLowerInvariant()
            } |
            Where-Object {
                $_.LastWriteTimeUtc -gt $referenceWriteTimeUtc
            } |
            Select-Object -First 1
        if ($null -ne $hasNewerInput) {
            exit 1
        }

        continue
    }

    if ($item.LastWriteTimeUtc -gt $referenceWriteTimeUtc) {
        exit 1
    }
}

exit 0
