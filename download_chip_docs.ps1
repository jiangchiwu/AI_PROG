
param(
    [string]$outputDir = "F:\work\AI_PROG\chip_docs"
)

$ErrorActionPreference = "SilentlyContinue"

if (-not (Test-Path -Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$chipInfo = @(
    @{Vendor="ST"; Series="STM32F1"; URL="https://www.st.com/resource/en/datasheet/stm32f103c8.pdf"},
    @{Vendor="ST"; Series="STM32F4"; URL="https://www.st.com/resource/en/datasheet/stm32f407zgt6.pdf"},
    @{Vendor="ST"; Series="STM32H7"; URL="https://www.st.com/resource/en/datasheet/stm32h743vit6.pdf"},
    @{Vendor="ST"; Series="STM32L4"; URL="https://www.st.com/resource/en/datasheet/stm32l431rct6.pdf"},
    @{Vendor="NXP"; Series="S32K144"; URL="https://www.nxp.com/docs/en/data-sheet/S32K144.pdf"},
    @{Vendor="NXP"; Series="S32K344"; URL="https://www.nxp.com/docs/en/data-sheet/S32K344.pdf"},
    @{Vendor="NXP"; Series="LPC55S69"; URL="https://www.nxp.com/docs/en/data-sheet/LPC55S69.pdf"},
    @{Vendor="Infineon"; Series="XMC4700"; URL="https://www.infineon.com/dgdl/Infineon-XMC4700-XMC4800-DataSheet-v01_00-EN.pdf"},
    @{Vendor="Cypress"; Series="PSoC6"; URL="https://www.cypress.com/file/475281/download"},
    @{Vendor="Renesas"; Series="RA4M1"; URL="https://www.renesas.com/us/en/document/dst/ra4m1-group-datasheet"},
    @{Vendor="GD"; Series="GD32F407"; URL="https://www.gd32mcu.com/data/documents/datasheet/GD32F407xx_Datasheet_Rev2.0.pdf"},
    @{Vendor="GD"; Series="GD32F103"; URL="https://www.gd32mcu.com/data/documents/datasheet/GD32F103xx_Datasheet_Rev2.1.pdf"}
)

Write-Host "=== Chip Documentation Download Script ===" -ForegroundColor Cyan
Write-Host "Output Directory: $outputDir" -ForegroundColor Green
Write-Host "Number of Chips: $($chipInfo.Count)" -ForegroundColor Green
Write-Host ""

foreach ($chip in $chipInfo) {
    $vendorDir = Join-Path -Path $outputDir -ChildPath $chip.Vendor
    if (-not (Test-Path -Path $vendorDir)) {
        New-Item -ItemType Directory -Path $vendorDir | Out-Null
    }
    
    $fileName = "$($chip.Series)_Datasheet.pdf"
    $outputPath = Join-Path -Path $vendorDir -ChildPath $fileName
    
    if (Test-Path -Path $outputPath) {
        Write-Host "Already exists: $($chip.Vendor)/$fileName" -ForegroundColor Gray
        continue
    }
    
    Write-Host "Downloading: $($chip.Vendor) $($chip.Series)" -ForegroundColor Yellow
    
    try {
        Invoke-WebRequest -Uri $chip.URL -OutFile $outputPath -UserAgent "Mozilla/5.0" -TimeoutSec 30
        Write-Host "  Download successful" -ForegroundColor Green
    } catch {
        Write-Host "  Download failed: $_" -ForegroundColor Red
    }
    
    Start-Sleep -Milliseconds 500
}

Write-Host ""
Write-Host "=== Download Complete ===" -ForegroundColor Cyan

$downloaded = Get-ChildItem -Path $outputDir -Recurse -Filter "*.pdf" | Measure-Object | Select-Object -ExpandProperty Count
Write-Host "Successfully downloaded: $downloaded documents" -ForegroundColor Green
