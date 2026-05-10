param(
    [Parameter(Mandatory = $true)]
    [string]$TargetPath
)

if (-not (Test-Path -LiteralPath $TargetPath))
{
    exit 0
}

try
{
    Remove-Item -LiteralPath $TargetPath -Recurse -Force -ErrorAction Stop
}
catch
{
    exit 1
}

if (Test-Path -LiteralPath $TargetPath)
{
    exit 1
}

exit 0
