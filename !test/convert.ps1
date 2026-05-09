$inputFile = Join-Path $PSScriptRoot "base.txt"
$outputFile = Join-Path $PSScriptRoot "reportXXX.txt"

$result = ""

Get-Content $inputFile | ForEach-Object {
    $hex = $_.Trim()

    if ($hex -match '^[0-9A-Fa-f]{2}$') {
        $byte = [Convert]::ToByte($hex, 16)

        if ($byte -ge 32 -and $byte -le 126) {
            $result += [char]$byte
        }
        elseif ($byte -eq 10) {
            $result += "`r`n"
        }
        elseif ($byte -eq 13) {
            # ignora, pois já tratamos o LF
        }
        else {
            $result += "[{0:X2}]" -f $byte
        }
    }
}

$result | Set-Content $outputFile -Encoding UTF8

Write-Host "Arquivo legível criado em:"
Write-Host $outputFile