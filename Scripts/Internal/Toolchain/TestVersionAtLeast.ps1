param(
    [Parameter(Mandatory = $true)]
    [string]$Actual,

    [Parameter(Mandatory = $true)]
    [string]$Minimum
)

$ErrorActionPreference = 'Stop'

function Convert-ToComparableVersion {
    param([Parameter(Mandatory = $true)][string]$Text)

    $match = [regex]::Match($Text, '\d+(?:\.\d+){0,3}')
    if (-not $match.Success) {
        throw "Could not parse version from '$Text'."
    }

    $parts = @($match.Value.Split('.') | ForEach-Object { [int]$_ })
    while ($parts.Count -lt 3) {
        $parts += 0
    }

    return [version]::new($parts[0], $parts[1], $parts[2], $(if ($parts.Count -gt 3) { $parts[3] } else { 0 }))
}

$actualVersion = Convert-ToComparableVersion -Text $Actual
$minimumVersion = Convert-ToComparableVersion -Text $Minimum

if ($actualVersion -lt $minimumVersion) {
    exit 1
}

exit 0