param(
    [string]$Port = "COM3",
    [int]$BaudRate = 115200,
    [int]$CaptureMs = 1200,
    [int]$SettleMs = 150,
    [ValidateSet("Full", "L3R3", "RightStick")]
    [string]$TestProfile = "Full",
    [int]$RepeatCount = 3
)

$scriptRoot = $PSScriptRoot
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportFile = Join-Path $scriptRoot "button_map_report_$timestamp.txt"
$rawFile = Join-Path $scriptRoot "button_map_raw_$timestamp.txt"
$packetFile = Join-Path $scriptRoot "button_map_packets_$timestamp.txt"

$packetLen = 64
$padPacketId = 0x80
$logPacketId = 0x90
$driverTypeWebApp = 0x64
$padInSize = 23

$buttonNames = @{
    0x0001 = "A"
    0x0002 = "B"
    0x0004 = "X"
    0x0008 = "Y"
    0x0010 = "L3"
    0x0020 = "R3"
    0x0040 = "Back"
    0x0080 = "Start"
    0x0100 = "LB"
    0x0200 = "RB"
    0x0400 = "Sys"
    0x0800 = "Misc"
}

$dpadNames = @{
    0x00 = "None"
    0x01 = "Up"
    0x02 = "Down"
    0x04 = "Left"
    0x08 = "Right"
    0x05 = "Up+Left"
    0x09 = "Up+Right"
    0x06 = "Down+Left"
    0x0A = "Down+Right"
}

function Get-CaptureSteps {
    param(
        [string]$Profile,
        [int]$RepeatCount
    )

    switch ($Profile) {
        "L3R3" {
            $steps = New-Object System.Collections.Generic.List[object]
            $steps.Add([pscustomobject]@{ Name = "Neutral"; Instruction = "Release all controls" })

            for ($i = 1; $i -le $RepeatCount; $i++) {
                $steps.Add([pscustomobject]@{
                    Name = ("L3 #{0}" -f $i)
                    Instruction = ("Press and hold L3 for capture #{0}" -f $i)
                })
            }

            for ($i = 1; $i -le $RepeatCount; $i++) {
                $steps.Add([pscustomobject]@{
                    Name = ("R3 #{0}" -f $i)
                    Instruction = ("Press and hold R3 for capture #{0}" -f $i)
                })
            }

            return $steps.ToArray()
        }

        "RightStick" {
            return @(
                [pscustomobject]@{ Name = "Neutral"; Instruction = "Release all controls" },
                [pscustomobject]@{ Name = "Right Stick Neutral"; Instruction = "Release the right stick and keep it centered" },
                [pscustomobject]@{ Name = "Right Stick Left"; Instruction = "Move and hold right stick to the left" },
                [pscustomobject]@{ Name = "Right Stick Right"; Instruction = "Move and hold right stick to the right" },
                [pscustomobject]@{ Name = "Right Stick Up"; Instruction = "Move and hold right stick up" },
                [pscustomobject]@{ Name = "Right Stick Down"; Instruction = "Move and hold right stick down" },
                [pscustomobject]@{ Name = "Right Stick Down Left"; Instruction = "Move and hold right stick down-left" },
                [pscustomobject]@{ Name = "Right Stick Down Right"; Instruction = "Move and hold right stick down-right" },
                [pscustomobject]@{ Name = "Right Stick Recenter"; Instruction = "Release the right stick and keep it centered" }
            )
        }

        default {
            return @(
                [pscustomobject]@{ Name = "Neutral"; Instruction = "Release all controls" },
                [pscustomobject]@{ Name = "A"; Instruction = "Press and hold A" },
                [pscustomobject]@{ Name = "B"; Instruction = "Press and hold B" },
                [pscustomobject]@{ Name = "X"; Instruction = "Press and hold X" },
                [pscustomobject]@{ Name = "Y"; Instruction = "Press and hold Y" },
                [pscustomobject]@{ Name = "LB"; Instruction = "Press and hold LB" },
                [pscustomobject]@{ Name = "RB"; Instruction = "Press and hold RB" },
                [pscustomobject]@{ Name = "ZL"; Instruction = "Press and hold ZL" },
                [pscustomobject]@{ Name = "ZR"; Instruction = "Press and hold ZR" },
                [pscustomobject]@{ Name = "Minus"; Instruction = "Press and hold Minus" },
                [pscustomobject]@{ Name = "Plus"; Instruction = "Press and hold Plus" },
                [pscustomobject]@{ Name = "Home"; Instruction = "Press and hold Home" },
                [pscustomobject]@{ Name = "Capture"; Instruction = "Press and hold Capture" },
                [pscustomobject]@{ Name = "L3"; Instruction = "Press and hold L3" },
                [pscustomobject]@{ Name = "R3"; Instruction = "Press and hold R3" },
                [pscustomobject]@{ Name = "Dpad Up"; Instruction = "Press and hold Dpad Up" },
                [pscustomobject]@{ Name = "Dpad Down"; Instruction = "Press and hold Dpad Down" },
                [pscustomobject]@{ Name = "Dpad Left"; Instruction = "Press and hold Dpad Left" },
                [pscustomobject]@{ Name = "Dpad Right"; Instruction = "Press and hold Dpad Right" },
                [pscustomobject]@{ Name = "Left Stick Left"; Instruction = "Move and hold left stick to the left" },
                [pscustomobject]@{ Name = "Left Stick Right"; Instruction = "Move and hold left stick to the right" },
                [pscustomobject]@{ Name = "Left Stick Up"; Instruction = "Move and hold left stick up" },
                [pscustomobject]@{ Name = "Left Stick Down"; Instruction = "Move and hold left stick down" },
                [pscustomobject]@{ Name = "Right Stick Left"; Instruction = "Move and hold right stick to the left" },
                [pscustomobject]@{ Name = "Right Stick Right"; Instruction = "Move and hold right stick to the right" },
                [pscustomobject]@{ Name = "Right Stick Up"; Instruction = "Move and hold right stick up" },
                [pscustomobject]@{ Name = "Right Stick Down"; Instruction = "Move and hold right stick down" }
            )
        }
    }
}

$steps = @(Get-CaptureSteps -Profile $TestProfile -RepeatCount $RepeatCount)

function Format-HexBytes {
    param([byte[]]$Data)

    if ($null -eq $Data -or $Data.Length -eq 0) {
        return ""
    }

    return (($Data | ForEach-Object { "{0:X2}" -f $_ }) -join " ")
}

function Format-Buttons {
    param([UInt16]$Buttons)

    if ($Buttons -eq 0) {
        return "None"
    }

    $names = New-Object System.Collections.Generic.List[string]
    foreach ($mask in ($buttonNames.Keys | Sort-Object { [int]$_ })) {
        if (($Buttons -band [UInt16]$mask) -ne 0) {
            $names.Add([string]$buttonNames[[int]$mask])
        }
    }

    if ($names.Count -eq 0) {
        return ("Unknown(0x{0:X4})" -f $Buttons)
    }

    return ($names -join ", ")
}

function Format-Dpad {
    param([byte]$Dpad)

    if ($dpadNames.ContainsKey([int]$Dpad)) {
        return [string]$dpadNames[[int]$Dpad]
    }

    return ("Unknown(0x{0:X2})" -f $Dpad)
}

function Read-SerialBytes {
    param(
        [System.IO.Ports.SerialPort]$SerialPort,
        [int]$DurationMs
    )

    $buffer = New-Object System.Collections.Generic.List[byte]
    $sw = [System.Diagnostics.Stopwatch]::StartNew()

    while ($sw.ElapsedMilliseconds -lt $DurationMs) {
        try {
            $value = $SerialPort.ReadByte()
            if ($value -ge 0) {
                $buffer.Add([byte]$value)
            }
        }
        catch [System.TimeoutException] {
        }
    }

    return ,$buffer.ToArray()
}

function Find-WebAppFrames {
    param([byte[]]$Bytes)

    $frames = New-Object System.Collections.Generic.List[object]
    $index = 0

    while ($index -le ($Bytes.Length - $packetLen)) {
        $packetSize = $Bytes[$index]
        $packetId = $Bytes[$index + 1]
        $driverType = $Bytes[$index + 2]
        $chunkLen = $Bytes[$index + 8]

        $looksValid = (
            $packetSize -eq $packetLen -and
            $driverType -eq $driverTypeWebApp -and
            $chunkLen -le ($packetLen - 9)
        )

        if (-not $looksValid) {
            $index++
            continue
        }

        $payload = [byte[]]::new($chunkLen)
        [Array]::Copy($Bytes, $index + 9, $payload, 0, $chunkLen)

        $frames.Add([pscustomobject]@{
            Offset = $index
            PacketId = [byte]$packetId
            Player = [byte]$Bytes[$index + 4]
            Profile = [byte]$Bytes[$index + 5]
            ChunksTotal = [byte]$Bytes[$index + 6]
            ChunkIndex = [byte]$Bytes[$index + 7]
            ChunkLen = [byte]$chunkLen
            Payload = $payload
        })

        $index += $packetLen
    }

    return ,$frames.ToArray()
}

function Decode-PadIn {
    param([byte[]]$Payload)

    if ($Payload.Length -lt $padInSize) {
        return $null
    }

    return [pscustomobject]@{
        Dpad = [byte]$Payload[0]
        Buttons = [BitConverter]::ToUInt16($Payload, 1)
        TriggerL = [byte]$Payload[3]
        TriggerR = [byte]$Payload[4]
        LeftX = [BitConverter]::ToInt16($Payload, 5)
        LeftY = [BitConverter]::ToInt16($Payload, 7)
        RightX = [BitConverter]::ToInt16($Payload, 9)
        RightY = [BitConverter]::ToInt16($Payload, 11)
        Analog = $Payload[13..22]
        Raw = $Payload
    }
}

function Get-Mode {
    param([Object[]]$Values)

    if ($null -eq $Values -or $Values.Count -eq 0) {
        return $null
    }

    return $Values |
        Group-Object |
        Sort-Object -Property @(
            @{ Expression = "Count"; Descending = $true },
            @{ Expression = "Name"; Descending = $false }
        ) |
        Select-Object -First 1 -ExpandProperty Name
}

function Get-PreferredMode {
    param(
        [Object[]]$Values,
        $NeutralValue
    )

    if ($null -eq $Values -or $Values.Count -eq 0) {
        return $null
    }

    $groups = @(
        $Values |
            Group-Object |
            Sort-Object -Property @(
                @{ Expression = "Count"; Descending = $true },
                @{ Expression = "Name"; Descending = $false }
            )
    )

    if ($null -eq $NeutralValue) {
        return $groups[0].Name
    }

    $nonNeutral = @($groups | Where-Object { $_.Name -ne [string]$NeutralValue })
    if ($nonNeutral.Count -gt 0) {
        return $nonNeutral[0].Name
    }

    return $groups[0].Name
}

function Summarize-Step {
    param(
        [string]$StepName,
        [string]$Instruction,
        [byte[]]$RawBytes,
        [object[]]$Frames,
        [object]$NeutralSummary = $null
    )

    $padFrames = @($Frames | Where-Object { $_.PacketId -eq $padPacketId -and $_.ChunkLen -ge $padInSize })
    $decoded = @($padFrames | ForEach-Object { Decode-PadIn -Payload $_.Payload } | Where-Object { $null -ne $_ })
    $logFrames = @($Frames | Where-Object { $_.PacketId -eq $logPacketId })

    $buttonsSeen = @($decoded | ForEach-Object { $_.Buttons } | Sort-Object -Unique)
    $dpadSeen = @($decoded | ForEach-Object { $_.Dpad } | Sort-Object -Unique)
    $triggerLSeen = @($decoded | ForEach-Object { $_.TriggerL } | Sort-Object -Unique)
    $triggerRSeen = @($decoded | ForEach-Object { $_.TriggerR } | Sort-Object -Unique)

    $leftX = @($decoded | ForEach-Object { $_.LeftX })
    $leftY = @($decoded | ForEach-Object { $_.LeftY })
    $rightX = @($decoded | ForEach-Object { $_.RightX })
    $rightY = @($decoded | ForEach-Object { $_.RightY })

    $buttonSamples = @($decoded | ForEach-Object { $_.Buttons })
    $dpadSamples = @($decoded | ForEach-Object { $_.Dpad })
    $triggerLSamples = @($decoded | ForEach-Object { $_.TriggerL })
    $triggerRSamples = @($decoded | ForEach-Object { $_.TriggerR })

    $dominantButtons = if ($buttonSamples.Count -gt 0) {
        [UInt16](Get-PreferredMode -Values $buttonSamples -NeutralValue $(if ($null -ne $NeutralSummary) { $NeutralSummary.DominantButtons } else { [UInt16]0 }))
    } else {
        [UInt16]0
    }
    $dominantDpad = if ($dpadSamples.Count -gt 0) {
        [byte](Get-PreferredMode -Values $dpadSamples -NeutralValue $(if ($null -ne $NeutralSummary) { $NeutralSummary.DominantDpad } else { [byte]0 }))
    } else {
        [byte]0
    }
    $dominantTriggerL = if ($triggerLSamples.Count -gt 0) {
        [byte](Get-PreferredMode -Values $triggerLSamples -NeutralValue $(if ($null -ne $NeutralSummary) { $NeutralSummary.DominantTriggerL } else { [byte]0 }))
    } else {
        [byte]0
    }
    $dominantTriggerR = if ($triggerRSamples.Count -gt 0) {
        [byte](Get-PreferredMode -Values $triggerRSamples -NeutralValue $(if ($null -ne $NeutralSummary) { $NeutralSummary.DominantTriggerR } else { [byte]0 }))
    } else {
        [byte]0
    }

    $samplePayloads = @(
        $padFrames |
            Group-Object { Format-HexBytes -Data $_.Payload } |
            Sort-Object -Property @(
                @{ Expression = "Count"; Descending = $true },
                @{ Expression = "Name"; Descending = $false }
            ) |
            Select-Object -First 3
    )

    return [pscustomobject]@{
        StepName = $StepName
        Instruction = $Instruction
        RawByteCount = $RawBytes.Length
        FrameCount = $Frames.Count
        LogFrameCount = $logFrames.Count
        PadFrameCount = $padFrames.Count
        ButtonsSeen = $buttonsSeen
        DpadSeen = $dpadSeen
        TriggerLSeen = $triggerLSeen
        TriggerRSeen = $triggerRSeen
        DominantButtons = $dominantButtons
        DominantDpad = $dominantDpad
        DominantTriggerL = $dominantTriggerL
        DominantTriggerR = $dominantTriggerR
        LeftXMin = if ($leftX.Count -gt 0) { ($leftX | Measure-Object -Minimum).Minimum } else { $null }
        LeftXMax = if ($leftX.Count -gt 0) { ($leftX | Measure-Object -Maximum).Maximum } else { $null }
        LeftYMin = if ($leftY.Count -gt 0) { ($leftY | Measure-Object -Minimum).Minimum } else { $null }
        LeftYMax = if ($leftY.Count -gt 0) { ($leftY | Measure-Object -Maximum).Maximum } else { $null }
        RightXMin = if ($rightX.Count -gt 0) { ($rightX | Measure-Object -Minimum).Minimum } else { $null }
        RightXMax = if ($rightX.Count -gt 0) { ($rightX | Measure-Object -Maximum).Maximum } else { $null }
        RightYMin = if ($rightY.Count -gt 0) { ($rightY | Measure-Object -Minimum).Minimum } else { $null }
        RightYMax = if ($rightY.Count -gt 0) { ($rightY | Measure-Object -Maximum).Maximum } else { $null }
        SamplePayloads = $samplePayloads
        RawBytes = $RawBytes
    }
}

function Write-StepReport {
    param(
        [System.Text.StringBuilder]$Builder,
        [object]$Summary,
        [object]$Neutral
    )

    [void]$Builder.AppendLine(("Step: {0}" -f $Summary.StepName))
    [void]$Builder.AppendLine(("Instruction: {0}" -f $Summary.Instruction))
    [void]$Builder.AppendLine(("Raw bytes: {0}" -f $Summary.RawByteCount))
    [void]$Builder.AppendLine(("Frames: total={0} log={1} pad={2}" -f $Summary.FrameCount, $Summary.LogFrameCount, $Summary.PadFrameCount))
    [void]$Builder.AppendLine(("Buttons seen: {0}" -f (($Summary.ButtonsSeen | ForEach-Object { ("0x{0:X4} [{1}]" -f $_, (Format-Buttons -Buttons $_)) }) -join ", ")))
    [void]$Builder.AppendLine(("Dpad seen: {0}" -f (($Summary.DpadSeen | ForEach-Object { ("0x{0:X2} [{1}]" -f $_, (Format-Dpad -Dpad $_)) }) -join ", ")))
    [void]$Builder.AppendLine(("Trigger L seen: {0}" -f (($Summary.TriggerLSeen | ForEach-Object { "{0}" -f $_ }) -join ", ")))
    [void]$Builder.AppendLine(("Trigger R seen: {0}" -f (($Summary.TriggerRSeen | ForEach-Object { "{0}" -f $_ }) -join ", ")))
    [void]$Builder.AppendLine(("Dominant buttons: 0x{0:X4} [{1}]" -f $Summary.DominantButtons, (Format-Buttons -Buttons $Summary.DominantButtons)))
    [void]$Builder.AppendLine(("Dominant dpad: 0x{0:X2} [{1}]" -f $Summary.DominantDpad, (Format-Dpad -Dpad $Summary.DominantDpad)))
    [void]$Builder.AppendLine(("Dominant triggers: L={0} R={1}" -f $Summary.DominantTriggerL, $Summary.DominantTriggerR))
    [void]$Builder.AppendLine(("Axis ranges: LX={0}..{1} LY={2}..{3} RX={4}..{5} RY={6}..{7}" -f `
        $Summary.LeftXMin, $Summary.LeftXMax, $Summary.LeftYMin, $Summary.LeftYMax, `
        $Summary.RightXMin, $Summary.RightXMax, $Summary.RightYMin, $Summary.RightYMax))

    if ($null -ne $Neutral) {
        $buttonDelta = [UInt16]($Summary.DominantButtons -bxor $Neutral.DominantButtons)
        $dpadDelta = [byte]($Summary.DominantDpad -bxor $Neutral.DominantDpad)
        [void]$Builder.AppendLine(("Delta vs neutral: buttons=0x{0:X4} [{1}] dpad=0x{2:X2} [{3}] trigL={4} trigR={5}" -f `
            $buttonDelta,
            (Format-Buttons -Buttons $buttonDelta),
            $dpadDelta,
            (Format-Dpad -Dpad $dpadDelta),
            ($Summary.DominantTriggerL - $Neutral.DominantTriggerL),
            ($Summary.DominantTriggerR - $Neutral.DominantTriggerR)))
    }

    if ($Summary.SamplePayloads.Count -gt 0) {
        [void]$Builder.AppendLine("Sample payloads:")
        foreach ($sample in $Summary.SamplePayloads) {
            [void]$Builder.AppendLine(("  count={0} data={1}" -f $sample.Count, $sample.Name))
        }
    }

    [void]$Builder.AppendLine()
}

$serial = New-Object System.IO.Ports.SerialPort $Port, $BaudRate, 'None', 8, 'One'
$serial.ReadTimeout = 100
$serial.DtrEnable = $true
$serial.RtsEnable = $true

$rawWriter = [System.IO.StreamWriter]::new($rawFile, $false, [System.Text.UTF8Encoding]::new($false))
$packetWriter = [System.IO.StreamWriter]::new($packetFile, $false, [System.Text.UTF8Encoding]::new($false))
$results = New-Object System.Collections.Generic.List[object]

try {
    $serial.Open()

    Write-Host ("Port {0} aberto. Capture por etapa: {1} ms." -f $Port, $CaptureMs)
    Write-Host ("Perfil de teste: {0}" -f $TestProfile)
    if ($TestProfile -eq "L3R3") {
        Write-Host ("Repeticoes por botao: {0}" -f $RepeatCount)
    }
    Write-Host "Segure o controle indicado durante cada etapa ate a captura terminar."
    Write-Host ""

    for ($index = 0; $index -lt $steps.Count; $index++) {
        $step = $steps[$index]
        Write-Host ("[{0}/{1}] {2}" -f ($index + 1), $steps.Count, $step.Instruction)
        [void](Read-Host "Pressione Enter para iniciar a captura")

        if ($serial.IsOpen) {
            $serial.DiscardInBuffer()
        }

        Start-Sleep -Milliseconds $SettleMs
        $rawBytes = Read-SerialBytes -SerialPort $serial -DurationMs $CaptureMs
        $frames = Find-WebAppFrames -Bytes $rawBytes
        $neutralSummaryForStep = $results | Where-Object { $_.StepName -eq "Neutral" } | Select-Object -First 1
        $summary = Summarize-Step -StepName $step.Name -Instruction $step.Instruction -RawBytes $rawBytes -Frames $frames -NeutralSummary $neutralSummaryForStep
        $results.Add($summary)

        $rawWriter.WriteLine(("STEP {0} | {1}" -f $step.Name, $step.Instruction))
        foreach ($byteValue in $rawBytes) {
            $rawWriter.WriteLine("{0:X2}" -f $byteValue)
        }
        $rawWriter.WriteLine()
        $rawWriter.Flush()

        foreach ($frame in $frames) {
            $packetLine = "step={0} packet_id=0x{1:X2} player={2} profile={3} chunk={4}/{5} chunk_len={6} data={7}" -f `
                $step.Name,
                $frame.PacketId,
                $frame.Player,
                $frame.Profile,
                $frame.ChunkIndex,
                $frame.ChunksTotal,
                $frame.ChunkLen,
                (Format-HexBytes -Data $frame.Payload)
            $packetWriter.WriteLine($packetLine)
        }
        $packetWriter.WriteLine()
        $packetWriter.Flush()

        $statusLine = "> pad frames: {0} | dominant buttons: 0x{1:X4} [{2}] | dominant dpad: 0x{3:X2} [{4}]" -f `
            $summary.PadFrameCount,
            $summary.DominantButtons,
            (Format-Buttons -Buttons $summary.DominantButtons),
            $summary.DominantDpad,
            (Format-Dpad -Dpad $summary.DominantDpad)
        Write-Host $statusLine
        Write-Host ""
    }
}
finally {
    $rawWriter.Dispose()
    $packetWriter.Dispose()

    if ($serial.IsOpen) {
        $serial.Close()
    }
}

$builder = New-Object System.Text.StringBuilder
[void]$builder.AppendLine("OGX Mini Plus button map capture")
[void]$builder.AppendLine(("Timestamp: {0}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss")))
[void]$builder.AppendLine(("Port: {0}" -f $Port))
[void]$builder.AppendLine(("Baud rate: {0}" -f $BaudRate))
[void]$builder.AppendLine(("Capture per step: {0} ms" -f $CaptureMs))
[void]$builder.AppendLine(("Settle before capture: {0} ms" -f $SettleMs))
[void]$builder.AppendLine(("Test profile: {0}" -f $TestProfile))
if ($TestProfile -eq "L3R3") {
    [void]$builder.AppendLine(("Repeat count: {0}" -f $RepeatCount))
}
[void]$builder.AppendLine()

$neutral = $results | Where-Object { $_.StepName -eq "Neutral" } | Select-Object -First 1

[void]$builder.AppendLine("Quick summary")
foreach ($result in $results | Where-Object { $_.StepName -ne "Neutral" }) {
    [void]$builder.AppendLine((
        "{0}: buttons=0x{1:X4} [{2}] dpad=0x{3:X2} [{4}] trigL={5} trigR={6} LX={7}..{8} LY={9}..{10} RX={11}..{12} RY={13}..{14}" -f `
        $result.StepName,
        $result.DominantButtons,
        (Format-Buttons -Buttons $result.DominantButtons),
        $result.DominantDpad,
        (Format-Dpad -Dpad $result.DominantDpad),
        $result.DominantTriggerL,
        $result.DominantTriggerR,
        $result.LeftXMin,
        $result.LeftXMax,
        $result.LeftYMin,
        $result.LeftYMax,
        $result.RightXMin,
        $result.RightXMax,
        $result.RightYMin,
        $result.RightYMax
    ))
}
[void]$builder.AppendLine()

[void]$builder.AppendLine("Detailed steps")
foreach ($result in $results) {
    Write-StepReport -Builder $builder -Summary $result -Neutral $neutral
}

[void]$builder.AppendLine(("Raw capture file: {0}" -f $rawFile))
[void]$builder.AppendLine(("Packet capture file: {0}" -f $packetFile))

[System.IO.File]::WriteAllText($reportFile, $builder.ToString(), [System.Text.UTF8Encoding]::new($false))

Write-Host "Relatorio final criado em:"
Write-Host $reportFile
Write-Host "Raw bytes por etapa:"
Write-Host $rawFile
Write-Host "Pacotes decodificados por etapa:"
Write-Host $packetFile
