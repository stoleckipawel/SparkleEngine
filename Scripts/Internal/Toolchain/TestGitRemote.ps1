param(
    [Parameter(Mandatory = $true)]
    [string]$GitCommand,

    [Parameter(Mandatory = $true)]
    [string]$RemoteUrl,

    [int]$TimeoutSeconds = 15
)

$ErrorActionPreference = 'Stop'

function Quote-ProcessArgument {
    param([Parameter(Mandatory = $true)][string]$Value)

    return '"' + $Value.Replace('"', '\"') + '"'
}

$process = [System.Diagnostics.Process]::new()
$process.StartInfo.FileName = $GitCommand
$process.StartInfo.Arguments = 'ls-remote --heads {0} master' -f (Quote-ProcessArgument -Value $RemoteUrl)
$process.StartInfo.UseShellExecute = $false
$process.StartInfo.RedirectStandardOutput = $true
$process.StartInfo.RedirectStandardError = $true
$process.StartInfo.Environment['GIT_TERMINAL_PROMPT'] = '0'

try {
    [void]$process.Start()
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        try {
            $process.Kill($true)
        }
        catch {
            $process.Kill()
        }

        Write-Error "Timed out reaching $RemoteUrl after $TimeoutSeconds second(s)."
        exit 124
    }

    if ($process.ExitCode -ne 0) {
        $stderr = $process.StandardError.ReadToEnd().Trim()
        if ([string]::IsNullOrWhiteSpace($stderr)) {
            $stderr = "git exited with code $($process.ExitCode)."
        }

        Write-Error $stderr
        exit $process.ExitCode
    }

    exit 0
}
finally {
    $process.Dispose()
}