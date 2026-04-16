param(
    [string]$InputFile,
    [string]$RawAsciiOutputFile,
    [string]$AsciiOutputFile,
    [string]$PacketOutputFile,
    [string]$TrailingOutputFile
)

$scriptRoot = $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($InputFile)) {
    $preferredInput = Join-Path $scriptRoot "baseBrute.txt"
    $fallbackInput = Join-Path $scriptRoot "base.txt"
    if (Test-Path $preferredInput) {
        $InputFile = $preferredInput
    }
    else {
        $InputFile = $fallbackInput
    }
}

if (-not (Test-Path $InputFile)) {
    throw "Arquivo de entrada nao encontrado: $InputFile"
}

if ([string]::IsNullOrWhiteSpace($AsciiOutputFile)) {
    $AsciiOutputFile = Join-Path $scriptRoot "ascii_output.txt"
}

if ([string]::IsNullOrWhiteSpace($RawAsciiOutputFile)) {
    $RawAsciiOutputFile = Join-Path $scriptRoot "ascii_raw_output.txt"
}

if ([string]::IsNullOrWhiteSpace($PacketOutputFile)) {
    $PacketOutputFile = Join-Path $scriptRoot "packet_output.txt"
}

if ([string]::IsNullOrWhiteSpace($TrailingOutputFile)) {
    $TrailingOutputFile = Join-Path $scriptRoot "trailing_bytes.txt"
}

$frameSize = 64
$headerSize = 9
$maxPayloadSize = $frameSize - $headerSize
$expectedPacketLen = 64
$expectedDriverType = 0x64
$logPacketId = 0x90

$packetNames = @{
    0x00 = "NONE"
    0x50 = "GET_PROFILE_BY_ID"
    0x55 = "GET_PROFILE_BY_IDX"
    0x60 = "SET_PROFILE_START"
    0x61 = "SET_PROFILE"
    0x80 = "SET_GP_IN"
    0x81 = "SET_GP_OUT"
    0x90 = "LOG_STREAM"
    0xFF = "RESP_ERROR"
}

function Get-PacketName {
    param([byte]$PacketId)

    if ($packetNames.ContainsKey([int]$PacketId)) {
        return $packetNames[[int]$PacketId]
    }

    return ("UNKNOWN_0x{0:X2}" -f $PacketId)
}

function Format-HexBytes {
    param([byte[]]$Data)

    if ($null -eq $Data -or $Data.Length -eq 0) {
        return ""
    }

    return (($Data | ForEach-Object { "{0:X2}" -f $_ }) -join " ")
}

$bytes = New-Object System.Collections.Generic.List[byte]

foreach ($line in Get-Content $InputFile) {
    $hex = $line.Trim()
    if ($hex -match '^[0-9A-Fa-f]{2}$') {
        $bytes.Add([Convert]::ToByte($hex, 16))
    }
}

$frameCount = [Math]::Floor($bytes.Count / $frameSize)
$trailingCount = $bytes.Count % $frameSize
$invalidFrames = 0
$logFrames = 0
$dataFrames = 0

$logPayloadBytes = New-Object System.Collections.Generic.List[byte]
$packetLines = New-Object System.Collections.Generic.List[string]

for ($frameIndex = 0; $frameIndex -lt $frameCount; $frameIndex++) {
    $offset = $frameIndex * $frameSize
    $packetLen = $bytes[$offset + 0]
    $packetId = $bytes[$offset + 1]
    $deviceDriver = $bytes[$offset + 2]
    $maxGamepads = $bytes[$offset + 3]
    $playerIdx = $bytes[$offset + 4]
    $profileId = $bytes[$offset + 5]
    $chunksTotal = $bytes[$offset + 6]
    $chunkIdx = $bytes[$offset + 7]
    $chunkLen = $bytes[$offset + 8]

    if ($packetLen -ne $expectedPacketLen -or $deviceDriver -ne $expectedDriverType -or $chunkLen -gt $maxPayloadSize) {
        $invalidFrames++
        continue
    }

    $payload = [byte[]]::new($chunkLen)
    for ($i = 0; $i -lt $chunkLen; $i++) {
        $payload[$i] = $bytes[$offset + $headerSize + $i]
    }

    if ($packetId -eq $logPacketId) {
        $logFrames++
        foreach ($value in $payload) {
            $logPayloadBytes.Add($value)
        }
        continue
    }

    $dataFrames++
    $packetLines.Add((
        "frame={0} packet_id=0x{1:X2} ({2}) player={3} profile={4} chunk={5}/{6} chunk_len={7} data={8}" -f `
        $frameIndex,
        $packetId,
        (Get-PacketName -PacketId $packetId),
        $playerIdx,
        $profileId,
        $chunkIdx,
        $chunksTotal,
        $chunkLen,
        (Format-HexBytes -Data $payload)
    ))
}

$rawAsciiText = [System.Text.Encoding]::ASCII.GetString($logPayloadBytes.ToArray())
$firstLogIndex = $rawAsciiText.IndexOf("OGXM:")
if ($firstLogIndex -ge 0) {
    $rawAsciiText = $rawAsciiText.Substring($firstLogIndex)
}

[System.IO.File]::WriteAllText($RawAsciiOutputFile, $rawAsciiText, [System.Text.UTF8Encoding]::new($false))

$cleanLogLines = New-Object System.Collections.Generic.List[string]
$rawLines = $rawAsciiText -split "\r?\n"

foreach ($rawLine in $rawLines) {
    $line = $rawLine.TrimEnd()
    if ($line -notmatch '^OGXM:') {
        continue
    }

    $body = $line.Substring(5).TrimStart()

    if ([string]::IsNullOrWhiteSpace($body)) {
        continue
    }

    if ($body -match '^(?:[0-9A-Fa-f]{2})(?:\s+[0-9A-Fa-f]{2}){7,}') {
        continue
    }

    $hexTokenCount = ([regex]::Matches($line, '\b[0-9A-Fa-f]{2}\b')).Count
    $hasStructuredHex = $line.Contains('bytes=') -or
        $line.Contains('cmd=0x') -or
        $line.Contains('subcmd=0x') -or
        $line.Contains('id=0x') -or
        $line.Contains('ack=0x') -or
        $line.Contains('report=0x')

    if ($hexTokenCount -ge 8 -and -not $hasStructuredHex) {
        continue
    }

    if ($line -match '^OGXM: Writing gamepad i(?!nput$)') {
        continue
    }

    if ($line -match '^OGXM: SwitchProHo(?!st\[)') {
        continue
    }

    if ($line -match '^OGXM: (?:Switate=|itchProHost)') {
        continue
    }

    $cleanLogLines.Add($line)
}

$asciiText = if ($cleanLogLines.Count -gt 0) {
    ($cleanLogLines -join [Environment]::NewLine) + [Environment]::NewLine
}
else {
    ""
}

[System.IO.File]::WriteAllText($AsciiOutputFile, $asciiText, [System.Text.UTF8Encoding]::new($false))

$packetText = if ($packetLines.Count -gt 0) {
    ($packetLines -join [Environment]::NewLine) + [Environment]::NewLine
}
else {
    ""
}
[System.IO.File]::WriteAllText($PacketOutputFile, $packetText, [System.Text.UTF8Encoding]::new($false))

if ($trailingCount -gt 0) {
    $trailing = [byte[]]::new($trailingCount)
    for ($i = 0; $i -lt $trailingCount; $i++) {
        $trailing[$i] = $bytes[$frameCount * $frameSize + $i]
    }
    $trailingText = "count=$trailingCount bytes=" + (Format-HexBytes -Data $trailing) + [Environment]::NewLine
    [System.IO.File]::WriteAllText($TrailingOutputFile, $trailingText, [System.Text.UTF8Encoding]::new($false))
    Write-Warning "Entrada nao e multipla de 64 bytes. A captura pode estar truncada."
}
elseif (Test-Path $TrailingOutputFile) {
    Remove-Item -LiteralPath $TrailingOutputFile
}

Write-Host "Arquivo de log ASCII criado em:"
Write-Host $AsciiOutputFile
Write-Host "Arquivo de log bruto criado em:"
Write-Host $RawAsciiOutputFile
Write-Host "Arquivo de pacotes nao textuais criado em:"
Write-Host $PacketOutputFile
if ($trailingCount -gt 0) {
    Write-Host "Arquivo com bytes restantes criado em:"
    Write-Host $TrailingOutputFile
}
Write-Host "Frames processados: $frameCount"
Write-Host "Frames de log: $logFrames"
Write-Host "Frames de dados: $dataFrames"
Write-Host "Frames invalidos: $invalidFrames"
