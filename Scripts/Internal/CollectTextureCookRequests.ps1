param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [Parameter(Mandatory = $true)]
    [string]$SceneListFile,

    [Parameter(Mandatory = $true)]
    [string]$AssetConverterExe,

    [Parameter(Mandatory = $true)]
    [string]$OutputRequestFile,

    [Parameter(Mandatory = $true)]
    [int]$TotalSceneCount
)

function Get-Fnv1a64Hex {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $offset = [System.Numerics.BigInteger]::Parse("14695981039346656037")
    $prime = [System.Numerics.BigInteger]1099511628211
    $modulus = [System.Numerics.BigInteger]::Parse("18446744073709551616")
    $hash = $offset
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
    foreach ($byte in $bytes) {
        $hash = [System.Numerics.BigInteger]([UInt64]$hash -bxor [UInt64]$byte)
        $hash = [System.Numerics.BigInteger]::Remainder(($hash * $prime), $modulus)
    }

    return ([UInt64]$hash).ToString("x16")
}

function Add-DefaultSkyTextureRequest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProjectRoot,

        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.SortedSet[string]]$UniqueRequestLines
    )

    $projectDirectory = Get-Item -LiteralPath $ProjectRoot
    $projectsRoot = $projectDirectory.Parent
    $workspaceRoot = $projectsRoot.Parent.FullName
    $projectName = $projectDirectory.Name

    $sourcePath = Join-Path $workspaceRoot "Engine\Assets\Textures\Sky\evening_road_01_puresky_4k.exr"
    $outputPath = Join-Path $workspaceRoot ("build\Cooked\{0}\Textures\Defaults\default_cubemap.stex" -f $projectName)
    $assetId = Get-Fnv1a64Hex -Text "engine:linear:Assets/Textures/Sky/evening_road_01_puresky_4k.exr"

    if (-not (Test-Path -LiteralPath $sourcePath)) {
        throw "Default sky source texture was not found: $sourcePath"
    }

    $requestLine = "{0}|linear|{1}|{2}" -f $assetId, ($outputPath -replace '\\', '/'), ($sourcePath -replace '\\', '/')
    [void]$UniqueRequestLines.Add($requestLine)

    Write-Host ("[LOG] Added default sky texture request: source='{0}' output='{1}'" -f $sourcePath, $outputPath)
}

$temporaryParent = Split-Path -Parent $OutputRequestFile
if ([string]::IsNullOrWhiteSpace($temporaryParent)) {
    $temporaryParent = $ProjectRoot
}
$temporaryRoot = Join-Path $temporaryParent ("sparkle-texture-requests-" + [System.Guid]::NewGuid().ToString("N"))
$failedScenes = @()
$header = $null
$uniqueRequestLines = New-Object 'System.Collections.Generic.SortedSet[string]' ([System.StringComparer]::Ordinal)
$collectedSceneCount = 0

New-Item -ItemType Directory -Path $temporaryRoot | Out-Null

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
        $sceneRequestFile = Join-Path $temporaryRoot (("{0:D4}-" -f $collectedSceneCount) + [IO.Path]::GetFileNameWithoutExtension($relativePath) + ".tcreq")

        Write-Host ''
        Write-Host ("[LOG] Collecting texture requests [{0}/{1}] {2}: {3}" -f ($collectedSceneCount + 1), $TotalSceneCount, $origin, $relativePath)

        & $AssetConverterExe collect-texture-requests $scenePath $sceneRequestFile
        if ($LASTEXITCODE -ne 0) {
            Write-Host ("[ERROR] Failed to collect texture requests for '{0}'." -f $relativePath)
            $failedScenes += ('{0}:{1}' -f $origin, $relativePath)
            $collectedSceneCount++
            continue
        }

        $requestLines = @(Get-Content -LiteralPath $sceneRequestFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($requestLines.Count -eq 0) {
            throw "Texture request file was empty for scene '$relativePath'."
        }

        if ($null -eq $header) {
            $header = $requestLines[0]
        } elseif ($header -ne $requestLines[0]) {
            throw "Texture request header mismatch between scenes. Expected '$header' but saw '$($requestLines[0])'."
        }

        for ($lineIndex = 1; $lineIndex -lt $requestLines.Count; $lineIndex++) {
            [void]$uniqueRequestLines.Add($requestLines[$lineIndex])
        }

        $collectedSceneCount++
    }

    if ($failedScenes.Count -gt 0) {
        Write-Host ''
        Write-Host ("[ERROR] Texture request collection failed for {0} scene(s)." -f $failedScenes.Count)
        Write-Host ("[ERROR] Failed scenes: {0}" -f ($failedScenes -join ';'))
        exit 1
    }

    if ($null -eq $header) {
        throw "No texture request header was produced during collection."
    }

    Add-DefaultSkyTextureRequest -ProjectRoot $ProjectRoot -UniqueRequestLines $uniqueRequestLines

    $outputLines = @($header)
    $outputLines += $uniqueRequestLines
    Set-Content -LiteralPath $OutputRequestFile -Value $outputLines -Encoding Ascii

    Write-Host ''
    Write-Host ("[LOG] Collected {0} unique texture request(s) into {1}" -f $uniqueRequestLines.Count, $OutputRequestFile)
}
finally {
    Pop-Location
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

exit 0