# PowerShell芯片资料批量下载脚本
# 用法: 右键点击此文件 -> "使用PowerShell运行"
# 或者在PowerShell中执行: .\download_chips_docs.ps1

$ErrorActionPreference = "SilentlyContinue"

# 创建下载目录
$baseDir = "f:\work\AI_PROG\芯片资料"
$categories = @(
    "NXP\S32K\S32K1\Datasheet",
    "NXP\S32K\S32K1\Reference_Manual",
    "NXP\S32K\S32K1\Programming_Manual",
    "NXP\S32K\S32K1\Application_Note",
    "NXP\S32K\S32K3\Datasheet",
    "NXP\S32K\S32K3\Reference_Manual",
    "NXP\S32K\S32K3\Programming_Manual",
    "NXP\S32K\S32K3\Application_Note",
    "STMicroelectronics\STM32F1\Datasheet",
    "STMicroelectronics\STM32F1\Reference_Manual",
    "STMicroelectronics\STM32F1\Programming_Manual",
    "STMicroelectronics\STM32F4\Datasheet",
    "STMicroelectronics\STM32F4\Reference_Manual",
    "STMicroelectronics\STM32F4\Programming_Manual",
    "STMicroelectronics\STM32H7\Datasheet",
    "STMicroelectronics\STM32H7\Reference_Manual",
    "STMicroelectronics\STM32H7\Programming_Manual",
    "TI\TivaC\Datasheet",
    "TI\TivaC\Reference_Manual",
    "Microchip\SAM\Datasheet",
    "Microchip\SAM\Reference_Manual",
    "RISC-V\GD32VF103",
    "RISC-V\ESP32-C3"
)

foreach ($category in $categories) {
    $dir = Join-Path $baseDir $category
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host "Created: $dir" -ForegroundColor Green
    }
}

# NXP S32K系列下载链接
$s32k1_docs = @(
    @{
        url = "https://www.nxp.com.cn/docs/en/data-sheet/S32K116.pdf"
        file = "S32K116_Datasheet.pdf"
        path = "NXP\S32K\S32K1\Datasheet"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/data-sheet/S32K118.pdf"
        file = "S32K118_Datasheet.pdf"
        path = "NXP\S32K\S32K1\Datasheet"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/data-sheet/S32K142.pdf"
        file = "S32K142_Datasheet.pdf"
        path = "NXP\S32K\S32K1\Datasheet"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/data-sheet/S32K144.pdf"
        file = "S32K144_Datasheet.pdf"
        path = "NXP\S32K\S32K1\Datasheet"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/reference-manual/S32K1XXRM.pdf"
        file = "S32K1_Reference_Manual.pdf"
        path = "NXP\S32K\S32K1\Reference_Manual"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/application-note/AN12183.pdf"
        file = "AN12183_Getting_Started.pdf"
        path = "NXP\S32K\S32K1\Application_Note"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/application-note/AN12218.pdf"
        file = "AN12218_Flash_Programming.pdf"
        path = "NXP\S32K\S32K1\Application_Note"
    }
)

$s32k3_docs = @(
    @{
        url = "https://www.nxp.com.cn/docs/en/data-sheet/S32K3XX.pdf"
        file = "S32K3xx_Datasheet.pdf"
        path = "NXP\S32K\S32K3\Datasheet"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/reference-manual/S32K3XXRM.pdf"
        file = "S32K3_Reference_Manual.pdf"
        path = "NXP\S32K\S32K3\Reference_Manual"
    },
    @{
        url = "https://www.nxp.com.cn/docs/en/application-note/S32K3_Security.pdf"
        file = "S32K3_Security.pdf"
        path = "NXP\S32K\S32K3\Application_Note"
    }
)

# ST STM32系列下载链接
$stm32f1_docs = @(
    @{
        url = "https://www.st.com/resource/en/datasheet/stm32f103rb.pdf"
        file = "STM32F103RB_Datasheet.pdf"
        path = "STMicroelectronics\STM32F1\Datasheet"
    },
    @{
        url = "https://www.st.com/resource/en/reference-manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf"
        file = "STM32F1xx_Reference_Manual.pdf"
        path = "STMicroelectronics\STM32F1\Reference_Manual"
    },
    @{
        url = "https://www.st.com/resource/en/programming-manual/pm0075-stm32f1xx-flash-memory-microcontrollers-stmicroelectronics.pdf"
        file = "STM32F1xx_Programming_Manual.pdf"
        path = "STMicroelectronics\STM32F1\Programming_Manual"
    },
    @{
        url = "https://www.st.com/resource/en/application-note/an2557-stm32f10xxx-in-application-programming-using-the-uart-stmicroelectronics.pdf"
        file = "AN2557_Flash_Programming.pdf"
        path = "STMicroelectronics\STM32F1\Application_Note"
    }
)

$stm32f4_docs = @(
    @{
        url = "https://www.st.com/resource/en/datasheet/stm32f405rg.pdf"
        file = "STM32F405_Datasheet.pdf"
        path = "STMicroelectronics\STM32F4\Datasheet"
    },
    @{
        url = "https://www.st.com/resource/en/datasheet/stm32f407vg.pdf"
        file = "STM32F407_Datasheet.pdf"
        path = "STMicroelectronics\STM32F4\Datasheet"
    },
    @{
        url = "https://www.st.com/resource/en/reference-manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf"
        file = "STM32F4xx_Reference_Manual.pdf"
        path = "STMicroelectronics\STM32F4\Reference_Manual"
    },
    @{
        url = "https://www.st.com/resource/en/programming-manual/pm0081-stm32f40x-and-stm32f41x-flash-programming-manual-stmicroelectronics.pdf"
        file = "STM32F4xx_Programming_Manual.pdf"
        path = "STMicroelectronics\STM32F4\Programming_Manual"
    }
)

$stm32h7_docs = @(
    @{
        url = "https://www.st.com/resource/en/datasheet/stm32h750vb.pdf"
        file = "STM32H750VB_Datasheet.pdf"
        path = "STMicroelectronics\STM32H7\Datasheet"
    },
    @{
        url = "https://www.st.com/resource/en/datasheet/stm32h743bi.pdf"
        file = "STM32H743_Datasheet.pdf"
        path = "STMicroelectronics\STM32H7\Datasheet"
    },
    @{
        url = "https://www.st.com/resource/en/reference-manual/rm0433-stm32h742-stm32h743-stm32h753-stm32h750-value-line-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf"
        file = "STM32H7xx_Reference_Manual.pdf"
        path = "STMicroelectronics\STM32H7\Reference_Manual"
    },
    @{
        url = "https://www.st.com/resource/en/programming-manual/pm0254-stm32h7-series-flash-programming-manual-stmicroelectronics.pdf"
        file = "STM32H7xx_Programming_Manual.pdf"
        path = "STMicroelectronics\STM32H7\Programming_Manual"
    }
)

# 合并所有下载任务
$all_docs = @(
    $s32k1_docs,
    $s32k3_docs,
    $stm32f1_docs,
    $stm32f4_docs,
    $stm32h7_docs
)

# 下载函数
function Download-File {
    param(
        [string]$url,
        [string]$filePath
    )

    $fullPath = Join-Path $baseDir $filePath

    if (Test-Path $fullPath) {
        Write-Host "SKIP (exists): $filePath" -ForegroundColor Yellow
        return
    }

    try {
        Write-Host "DOWNLOADING: $filePath" -ForegroundColor Cyan
        Invoke-WebRequest -Uri $url -OutFile $fullPath -TimeoutSec 30 -UseBasicParsing
        $size = (Get-Item $fullPath).Length / 1MB
        Write-Host "COMPLETED: $filePath ($([math]::Round($size, 2)) MB)" -ForegroundColor Green
    } catch {
        Write-Host "FAILED: $filePath - $($_.Exception.Message)" -ForegroundColor Red
    }
}

# 主下载循环
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "开始下载芯片资料..." -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
Write-Host ""

$total = 0
$success = 0
$failed = 0

foreach ($category in $all_docs) {
    foreach ($doc in $category) {
        $total++
        $filePath = Join-Path $doc.path $doc.file

        Download-File -url $doc.url -filePath $filePath

        if ($?) {
            $success++
        } else {
            $failed++
        }

        Start-Sleep -Milliseconds 500
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "下载完成!" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "总计: $total" -ForegroundColor Cyan
Write-Host "成功: $success" -ForegroundColor Green
Write-Host "失败: $failed" -ForegroundColor Red
Write-Host ""
Write-Host "资料保存在: $baseDir" -ForegroundColor Cyan
Write-Host ""

# 显示目录结构
Write-Host "下载的目录结构:" -ForegroundColor Yellow
Get-ChildItem -Path $baseDir -Recurse -Directory | Select-Object FullName
