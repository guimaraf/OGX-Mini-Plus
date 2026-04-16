$port = "COM3"
$durationSeconds = 10

$p = New-Object System.IO.Ports.SerialPort $port,115200,'None',8,'One'
$p.ReadTimeout = 200
$p.DtrEnable = $true
$p.RtsEnable = $true

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$file = Join-Path $PSScriptRoot "report_$timestamp.txt"

try {
    $p.Open()

    $start = Get-Date
    $count = 0
    $writer = [System.IO.StreamWriter]::new($file, $false, [System.Text.UTF8Encoding]::new($false))

    try {
        while (((Get-Date) - $start).TotalSeconds -lt $durationSeconds) {
            try {
                $b = $p.ReadByte()
                if ($b -ge 0) {
                    $writer.WriteLine("{0:X2}" -f $b)
                    $count++
                }
            } catch [System.TimeoutException] {
            }
        }

        $writer.WriteLine("TOTAL_BYTES=$count")
    }
    finally {
        $writer.Dispose()
    }
}
finally {
    if ($p.IsOpen) {
        $p.Close()
    }
}

Write-Host "Arquivo criado em:"
Write-Host $file
