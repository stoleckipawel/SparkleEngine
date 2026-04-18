param(
    [Parameter(Mandatory = $true)]
    [string]$Caller,

    [string]$RemainingArgs = '',

    [Parameter(Mandatory = $true)]
    [string]$LogFile
)

$env:LOG_CAPTURED = '1'
$env:LOGFILE = $LogFile

$commandLine = '"{0}"{1}' -f $Caller, $RemainingArgs
$output = & cmd.exe /d /s /c $commandLine 2>&1
$exitCode = $LASTEXITCODE

$output | Tee-Object -FilePath $LogFile
exit $exitCode