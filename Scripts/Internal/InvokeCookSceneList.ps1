param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [Parameter(Mandatory = $true)]
    [string]$SceneListFile,

    [Parameter(Mandatory = $true)]
    [string]$AssetConverterExe,

    [Parameter(Mandatory = $true)]
    [int]$TotalSceneCount
)

$cookedCount = 0
$failedScenes = @()

Push-Location $ProjectRoot
try {
    foreach ($entry in Get-Content -LiteralPath $SceneListFile) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }

        $parts = $entry -split '\|', 3
        if ($parts.Count -ne 3) {
            throw "Malformed scene entry: $entry"
        }

        $origin = $parts[0]
        $relativePath = $parts[1]
        $scenePath = $parts[2]

        Write-Host ''
        Write-Host ("[LOG] Cooking [{0}/{1}] {2}: {3}" -f $cookedCount, $TotalSceneCount, $origin, $relativePath)

        & $AssetConverterExe $scenePath
        if ($LASTEXITCODE -ne 0) {
            Write-Host ("[ERROR] Failed to cook '{0}'." -f $relativePath)
            $failedScenes += ('{0}:{1}' -f $origin, $relativePath)
            continue
        }

        $cookedCount++
    }
}
finally {
    Pop-Location
}

if ($failedScenes.Count -gt 0) {
    Write-Host ''
    Write-Host ("[ERROR] CookAssets completed with {0} failed scene(s)." -f $failedScenes.Count)
    Write-Host ("[ERROR] Failed scenes: {0}" -f ($failedScenes -join ';'))
    exit 1
}

exit 0