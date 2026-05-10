param(
    [Parameter(Mandatory = $true)]
    [string]$VswherePath,

    [Parameter(Mandatory = $true)]
    [string]$CMakeCommand,

    [Parameter(Mandatory = $true)]
    [string]$ComponentId,

    [int]$MinimumMajor = 17,

    [AllowEmptyString()]
    [string]$PreferredGenerator = ''
)

$ErrorActionPreference = 'Stop'

function Write-BatchSet {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [AllowEmptyString()]
        [string]$Value
    )

    $escapedValue = $Value.Replace('"', '')
    Write-Output ('set "{0}={1}"' -f $Name, $escapedValue)
}

function Get-CMakeVisualStudioGenerators {
    param([Parameter(Mandatory = $true)][string]$Command)

    $help = & $Command --help 2>$null
    foreach ($line in $help) {
        if ($line -match '^\s*(?:\*\s*)?(Visual Studio\s+(\d+)\s+(\d+))\s*=') {
            [pscustomobject]@{
                Name = $matches[1]
                Major = [int]$matches[2]
                Year = [int]$matches[3]
            }
        }
    }
}

function Get-VisualStudioInstances {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Component,
        [Parameter(Mandatory = $true)][int]$Minimum
    )

    $versionRange = '[{0}.0,)' -f $Minimum
    $json = & $Path -products * -requires $Component -version $versionRange -format json 2>$null
    if ([string]::IsNullOrWhiteSpace(($json -join ''))) {
        return @()
    }

    $instances = @($json | ConvertFrom-Json)
    return $instances | Where-Object { $_ -and $_.installationVersion -and $_.installationPath }
}

function Get-MajorVersion {
    param([Parameter(Mandatory = $true)][string]$VersionText)

    return [int]($VersionText.Split('.')[0])
}

$generators = @(Get-CMakeVisualStudioGenerators -Command $CMakeCommand)
if ($generators.Count -eq 0) {
    exit 1
}

$instances = @(Get-VisualStudioInstances -Path $VswherePath -Component $ComponentId -Minimum $MinimumMajor)
if ($instances.Count -eq 0) {
    exit 1
}

if (-not [string]::IsNullOrWhiteSpace($PreferredGenerator)) {
    $preferred = $generators | Where-Object { $_.Name -ieq $PreferredGenerator } | Select-Object -First 1
    if ($null -eq $preferred -or $preferred.Major -lt $MinimumMajor) {
        exit 1
    }

    $matchingInstance = $instances |
        Where-Object { (Get-MajorVersion -VersionText $_.installationVersion) -eq $preferred.Major } |
        Sort-Object @{ Expression = { [version]$_.installationVersion }; Descending = $true } |
        Select-Object -First 1

    if ($null -eq $matchingInstance) {
        exit 1
    }

    $selectedGenerator = $preferred
    $selectedInstance = $matchingInstance
}
else {
    $supportedByMajor = @{}
    foreach ($generator in $generators) {
        if ($generator.Major -ge $MinimumMajor -and -not $supportedByMajor.ContainsKey($generator.Major)) {
            $supportedByMajor[$generator.Major] = $generator
        }
    }

    $selectedInstance = $null
    $selectedGenerator = $null
    foreach ($instance in ($instances | Sort-Object @{ Expression = { [version]$_.installationVersion }; Descending = $true })) {
        $major = Get-MajorVersion -VersionText $instance.installationVersion
        if ($supportedByMajor.ContainsKey($major)) {
            $selectedInstance = $instance
            $selectedGenerator = $supportedByMajor[$major]
            break
        }
    }

    if ($null -eq $selectedInstance -or $null -eq $selectedGenerator) {
        exit 1
    }
}

$selectedMajor = Get-MajorVersion -VersionText $selectedInstance.installationVersion
$selectedDisplayName = if ([string]::IsNullOrWhiteSpace($selectedInstance.displayName)) {
    'Visual Studio {0}' -f $selectedGenerator.Year
}
else {
    $selectedInstance.displayName
}

Write-BatchSet -Name 'GENERATOR' -Value $selectedGenerator.Name
Write-BatchSet -Name 'VS_VERSION_RANGE' -Value ('[{0}.0,{1}.0)' -f $selectedMajor, ($selectedMajor + 1))
Write-BatchSet -Name 'VS_INSTALLATION_PATH' -Value $selectedInstance.installationPath
Write-BatchSet -Name 'VS_INSTALLATION_VERSION' -Value $selectedInstance.installationVersion
Write-BatchSet -Name 'VS_DISPLAY_NAME' -Value $selectedDisplayName