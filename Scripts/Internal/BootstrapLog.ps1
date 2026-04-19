param(
    [Parameter(Mandatory = $true)]
    [string]$Caller,

    [string]$RemainingArgs = '',

    [Parameter(Mandatory = $true)]
    [string]$LogFile
)

function Get-LineColor {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string]$Line
    )

    if ([string]::IsNullOrEmpty($Line)) {
        return 'White'
    }

    if ($Line -match '(?i)\[(error|fatal)\]') {
        return 'Red'
    }

    if ($Line -match '(?i)\[(warn|warning)\]') {
        return 'Yellow'
    }

    return 'White'
}

$env:LOG_CAPTURED = '1'
$env:LOGFILE = $LogFile

$commandLine = '"{0}"{1}' -f $Caller, $RemainingArgs
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$writer = [System.IO.StreamWriter]::new($LogFile, $false, $utf8NoBom)

try {
    & cmd.exe /d /s /c $commandLine 2>&1 | ForEach-Object {
        $line = if ($_ -is [System.Management.Automation.ErrorRecord]) {
            $_.ToString()
        }
        else {
            [string]$_
        }

        $writer.WriteLine($line)
        $writer.Flush()
        Write-Host $line -ForegroundColor (Get-LineColor -Line $line)
    }

    $exitCode = $LASTEXITCODE
}
finally {
    $writer.Dispose()
}

exit $exitCode