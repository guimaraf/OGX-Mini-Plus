$inputFile = Join-Path $PSScriptRoot "base.txt"
$outputFile = Join-Path $PSScriptRoot "ascii_output.txt"

$frameSize = 64
$headerSize = 8
$lengthOffset = 8
$payloadOffset = 9
$maxPayloadSize = $frameSize - $payloadOffset
$expectedHeader = [byte[]](0x40, 0x90, 0x64, 0x01, 0x00, 0x00, 0x01, 0x00)

$lines = Get-Content $inputFile
$bytes = New-Object System.Collections.Generic.List[byte]

foreach ($line in $lines) {
    $hex = $line.Trim()
    if ($hex -match '^[0-9A-Fa-f]{2}$') {
        $bytes.Add([Convert]::ToByte($hex, 16))
    }
}

if (($bytes.Count % $frameSize) -ne 0) {
    Write-Warning "Entrada nao e multipla de 64 bytes. A captura pode estar truncada."
}

$payloadBytes = New-Object System.Collections.Generic.List[byte]
$frameCount = [Math]::Floor($bytes.Count / $frameSize)
$skippedFrames = 0

for ($frameIndex = 0; $frameIndex -lt $frameCount; $frameIndex++) {
    $offset = $frameIndex * $frameSize
    $headerOk = $true

    for ($i = 0; $i -lt $headerSize; $i++) {
        if ($bytes[$offset + $i] -ne $expectedHeader[$i]) {
            $headerOk = $false
            break
        }
    }

    if (-not $headerOk) {
        $skippedFrames++
        continue
    }

    $payloadLength = $bytes[$offset + $lengthOffset]
    if ($payloadLength -gt $maxPayloadSize) {
        Write-Warning "Frame $frameIndex com payload invalido ($payloadLength bytes)."
        $skippedFrames++
        continue
    }

    for ($i = 0; $i -lt $payloadLength; $i++) {
        $payloadBytes.Add($bytes[$offset + $payloadOffset + $i])
    }
}

$text = [System.Text.Encoding]::ASCII.GetString($payloadBytes.ToArray())

$firstLogIndex = $text.IndexOf("OGXM:")
if ($firstLogIndex -ge 0) {
    $text = $text.Substring($firstLogIndex)
}

[System.IO.File]::WriteAllText($outputFile, $text, [System.Text.UTF8Encoding]::new($false))

Write-Host "Arquivo ASCII criado em:"
Write-Host $outputFile
Write-Host "Frames processados: $frameCount"
Write-Host "Frames ignorados: $skippedFrames"
