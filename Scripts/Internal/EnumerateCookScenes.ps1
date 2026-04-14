param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,

    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [Parameter(Mandatory = $true)]
    [string]$ListFile,

    [Parameter(Mandatory = $true)]
    [string]$SummaryFile
)

$extensions = @('.gltf', '.glb', '.fbx')
$entries = @{}
$engineCount = 0
$projectCount = 0
$overrideCount = 0

function Add-SceneEntries {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$Origin
    )

    if (-not (Test-Path -LiteralPath $Root)) {
        return
    }

    $rootPrefix = [System.IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    Get-ChildItem -LiteralPath $Root -Recurse -File |
        Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() } |
        ForEach-Object {
            $fullPath = [System.IO.Path]::GetFullPath($_.FullName)
            if (-not $fullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                return
            }

            $relativePath = $fullPath.Substring($rootPrefix.Length).Replace('\', '/')
            $key = $relativePath.ToLowerInvariant()

            if ($Origin -eq 'Engine') {
                $script:engineCount++
                if (-not $entries.ContainsKey($key)) {
                    $entries[$key] = [PSCustomObject]@{
                        Origin = $Origin
                        Relative = $relativePath
                        Path = $fullPath
                    }
                }
                return
            }

            $script:projectCount++
            if ($entries.ContainsKey($key) -and $entries[$key].Origin -eq 'Engine') {
                $script:overrideCount++
            }

            $entries[$key] = [PSCustomObject]@{
                Origin = $Origin
                Relative = $relativePath
                Path = $fullPath
            }
        }
}

Add-SceneEntries -Root $EngineRoot -Origin 'Engine'
Add-SceneEntries -Root $ProjectRoot -Origin 'Project'

$finalEntries = $entries.Values | Sort-Object Relative
$finalEntries |
    ForEach-Object { '{0}|{1}|{2}' -f $_.Origin, $_.Relative, $_.Path } |
    Set-Content -Path $ListFile -Encoding Ascii

@(
    'ENGINE_SCENE_COUNT=' + $engineCount
    'PROJECT_SCENE_COUNT=' + $projectCount
    'OVERRIDDEN_ENGINE_COUNT=' + $overrideCount
    'TOTAL_SCENE_COUNT=' + $finalEntries.Count
) | Set-Content -Path $SummaryFile -Encoding Ascii