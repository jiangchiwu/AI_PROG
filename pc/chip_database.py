"""
芯片数据库和快速搜索功能
"""
import json
import os
from typing import List, Dict, Any, Optional


class ChipDatabase:
    """芯片数据库类"""

    def __init__(self, database_file: str = "chip_database.json"):
        self.database_file = database_file
        self.chips: List[Dict[str, Any]] = []
        self._load_database()

    def _load_database(self):
        """加载芯片数据库"""
        if os.path.exists(self.database_file):
            try:
                with open(self.database_file, 'r', encoding='utf-8') as f:
                    self.chips = json.load(f)
            except Exception as e:
                print(f"加载芯片数据库失败: {e}")
                self._init_default_database()
        else:
            self._init_default_database()

    def _save_database(self):
        """保存芯片数据库"""
        try:
            with open(self.database_file, 'w', encoding='utf-8') as f:
                json.dump(self.chips, f, ensure_ascii=False, indent=2)
        except Exception as e:
            print(f"保存芯片数据库失败: {e}")

    def _init_default_database(self):
        """初始化默认芯片数据库"""
        
        # ==================== STM32全系列 ====================
        stm32_chips = [
            # STM32F0系列 (Cortex-M0)
            {"name": "STM32F030C8", "vendor": "STMicroelectronics", "family": "STM32F0", "core": "Cortex-M0",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32F030K6", "vendor": "STMicroelectronics", "family": "STM32F0", "core": "Cortex-M0",
             "flash_size": 32768, "ram_size": 4096, "package": "LQFP32", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32F051R8", "vendor": "STMicroelectronics", "family": "STM32F0", "core": "Cortex-M0",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32F091VC", "vendor": "STMicroelectronics", "family": "STM32F0", "core": "Cortex-M0",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP100", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # STM32F1系列 (Cortex-M3)
            {"name": "STM32F101C8", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 65536, "ram_size": 16384, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103C8", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 65536, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103CB", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 131072, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103R8", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 65536, "ram_size": 20480, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103RC", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 49152, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103RE", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103VC", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 49152, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103VE", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F103ZG", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 1048576, "ram_size": 98304, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F105RB", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 131072, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F107VC", "vendor": "STMicroelectronics", "family": "STM32F1", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32F2系列 (Cortex-M3)
            {"name": "STM32F205RB", "vendor": "STMicroelectronics", "family": "STM32F2", "core": "Cortex-M3",
             "flash_size": 131072, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F207VC", "vendor": "STMicroelectronics", "family": "STM32F2", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 131072, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F207ZG", "vendor": "STMicroelectronics", "family": "STM32F2", "core": "Cortex-M3",
             "flash_size": 1048576, "ram_size": 131072, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32F3系列 (Cortex-M4F)
            {"name": "STM32F301C8", "vendor": "STMicroelectronics", "family": "STM32F3", "core": "Cortex-M4F",
             "flash_size": 65536, "ram_size": 16384, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F303RC", "vendor": "STMicroelectronics", "family": "STM32F3", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 40960, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F303VC", "vendor": "STMicroelectronics", "family": "STM32F3", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 40960, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F373VC", "vendor": "STMicroelectronics", "family": "STM32F3", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32F4系列 (Cortex-M4F)
            {"name": "STM32F401CB", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 131072, "ram_size": 65536, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F401RE", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 98304, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F405RG", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F407VG", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F407ZG", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F411CE", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 131072, "package": "UFQFPN48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F412RE", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 262144, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F413RG", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 1572864, "ram_size": 327680, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F415RG", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F427VI", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 262144, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F429ZI", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 262144, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F439ZI", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 262144, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F469NI", "vendor": "STMicroelectronics", "family": "STM32F4", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 393216, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32F7系列 (Cortex-M7)
            {"name": "STM32F722RE", "vendor": "STMicroelectronics", "family": "STM32F7", "core": "Cortex-M7",
             "flash_size": 524288, "ram_size": 262144, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F746NG", "vendor": "STMicroelectronics", "family": "STM32F7", "core": "Cortex-M7",
             "flash_size": 1048576, "ram_size": 327680, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F756NG", "vendor": "STMicroelectronics", "family": "STM32F7", "core": "Cortex-M7",
             "flash_size": 1048576, "ram_size": 327680, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F767IG", "vendor": "STMicroelectronics", "family": "STM32F7", "core": "Cortex-M7",
             "flash_size": 2097152, "ram_size": 524288, "package": "LQFP176", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32F769NI", "vendor": "STMicroelectronics", "family": "STM32F7", "core": "Cortex-M7",
             "flash_size": 2097152, "ram_size": 524288, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32G0系列 (Cortex-M0+)
            {"name": "STM32G030C6", "vendor": "STMicroelectronics", "family": "STM32G0", "core": "Cortex-M0+",
             "flash_size": 32768, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32G070RB", "vendor": "STMicroelectronics", "family": "STM32G0", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 36864, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32G071RB", "vendor": "STMicroelectronics", "family": "STM32G0", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32G0B1RE", "vendor": "STMicroelectronics", "family": "STM32G0", "core": "Cortex-M0+",
             "flash_size": 524288, "ram_size": 147456, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # STM32G4系列 (Cortex-M4F)
            {"name": "STM32G431RB", "vendor": "STMicroelectronics", "family": "STM32G4", "core": "Cortex-M4F",
             "flash_size": 131072, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32G473RE", "vendor": "STMicroelectronics", "family": "STM32G4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32G474RE", "vendor": "STMicroelectronics", "family": "STM32G4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 153600, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32G484RE", "vendor": "STMicroelectronics", "family": "STM32G4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 153600, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32H7系列 (Cortex-M7/M4双核)
            {"name": "STM32H743VI", "vendor": "STMicroelectronics", "family": "STM32H7", "core": "Cortex-M7",
             "flash_size": 2097152, "ram_size": 1048576, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32H743ZI", "vendor": "STMicroelectronics", "family": "STM32H7", "core": "Cortex-M7",
             "flash_size": 2097152, "ram_size": 1048576, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32H750VB", "vendor": "STMicroelectronics", "family": "STM32H7", "core": "Cortex-M7",
             "flash_size": 131072, "ram_size": 1048576, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32H753VI", "vendor": "STMicroelectronics", "family": "STM32H7", "core": "Cortex-M7",
             "flash_size": 2097152, "ram_size": 1048576, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32H747XI", "vendor": "STMicroelectronics", "family": "STM32H7", "core": "Cortex-M7+M4",
             "flash_size": 2097152, "ram_size": 1114112, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32H7B3LI", "vendor": "STMicroelectronics", "family": "STM32H7", "core": "Cortex-M7",
             "flash_size": 2097152, "ram_size": 1409024, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32L0系列 (Cortex-M0+ 超低功耗)
            {"name": "STM32L010C6", "vendor": "STMicroelectronics", "family": "STM32L0", "core": "Cortex-M0+",
             "flash_size": 32768, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32L051C8", "vendor": "STMicroelectronics", "family": "STM32L0", "core": "Cortex-M0+",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32L073RB", "vendor": "STMicroelectronics", "family": "STM32L0", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 20480, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "STM32L083RB", "vendor": "STMicroelectronics", "family": "STM32L0", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 20480, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # STM32L4系列 (Cortex-M4F 超低功耗)
            {"name": "STM32L431RC", "vendor": "STMicroelectronics", "family": "STM32L4", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32L452RE", "vendor": "STMicroelectronics", "family": "STM32L4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 163840, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32L476RG", "vendor": "STMicroelectronics", "family": "STM32L4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 131072, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32L496ZG", "vendor": "STMicroelectronics", "family": "STM32L4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 327680, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32L4R9ZI", "vendor": "STMicroelectronics", "family": "STM32L4+", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 655360, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32U5系列 (Cortex-M33 超低功耗)
            {"name": "STM32U575RG", "vendor": "STMicroelectronics", "family": "STM32U5", "core": "Cortex-M33",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32U585RG", "vendor": "STMicroelectronics", "family": "STM32U5", "core": "Cortex-M33",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32U599ZI", "vendor": "STMicroelectronics", "family": "STM32U5", "core": "Cortex-M33",
             "flash_size": 2097152, "ram_size": 786432, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32WB系列 (无线)
            {"name": "STM32WB55CG", "vendor": "STMicroelectronics", "family": "STM32WB", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32WB55RG", "vendor": "STMicroelectronics", "family": "STM32WB", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # STM32WL系列 (LoRa)
            {"name": "STM32WLE5JB", "vendor": "STMicroelectronics", "family": "STM32WL", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "STM32WL55JC", "vendor": "STMicroelectronics", "family": "STM32WL", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
        ]

        # ==================== GD32全系列 ====================
        gd_chips = [
            # GD32F1系列 (Cortex-M3)
            {"name": "GD32F103C8", "vendor": "GigaDevice", "family": "GD32F1", "core": "Cortex-M3",
             "flash_size": 65536, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F103CB", "vendor": "GigaDevice", "family": "GD32F1", "core": "Cortex-M3",
             "flash_size": 131072, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F103RC", "vendor": "GigaDevice", "family": "GD32F1", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 49152, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F103RE", "vendor": "GigaDevice", "family": "GD32F1", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F103VC", "vendor": "GigaDevice", "family": "GD32F1", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 49152, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F103ZE", "vendor": "GigaDevice", "family": "GD32F1", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # GD32F3系列 (Cortex-M4F)
            {"name": "GD32F303RC", "vendor": "GigaDevice", "family": "GD32F3", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 49152, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F303VC", "vendor": "GigaDevice", "family": "GD32F3", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 49152, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F303ZG", "vendor": "GigaDevice", "family": "GD32F3", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 98304, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F350RB", "vendor": "GigaDevice", "family": "GD32F3", "core": "Cortex-M4F",
             "flash_size": 131072, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # GD32F4系列 (Cortex-M4F)
            {"name": "GD32F405RG", "vendor": "GigaDevice", "family": "GD32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F407VG", "vendor": "GigaDevice", "family": "GD32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F407ZG", "vendor": "GigaDevice", "family": "GD32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F450ZI", "vendor": "GigaDevice", "family": "GD32F4", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 262144, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32F470ZG", "vendor": "GigaDevice", "family": "GD32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # GD32E5系列 (Cortex-M33F)
            {"name": "GD32E503RC", "vendor": "GigaDevice", "family": "GD32E5", "core": "Cortex-M33F",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "GD32E507VG", "vendor": "GigaDevice", "family": "GD32E5", "core": "Cortex-M33F",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # GD32E1系列 (Cortex-M23)
            {"name": "GD32E103C8", "vendor": "GigaDevice", "family": "GD32E1", "core": "Cortex-M23",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "GD32E103RB", "vendor": "GigaDevice", "family": "GD32E1", "core": "Cortex-M23",
             "flash_size": 131072, "ram_size": 16384, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # GD32C2系列 (Cortex-M23)
            {"name": "GD32C231RC", "vendor": "GigaDevice", "family": "GD32C2", "core": "Cortex-M23",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
        ]
        
        # ==================== NXP/飞思卡尔系列 ====================
        nxp_chips = [
            # S32K1系列 (Cortex-M4F/M0+)
            {"name": "S32K142", "vendor": "NXP", "family": "S32K1", "core": "Cortex-M4F",
             "flash_size": 131072, "ram_size": 32768, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "S32K144", "vendor": "NXP", "family": "S32K1", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "S32K146", "vendor": "NXP", "family": "S32K1", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "S32K148", "vendor": "NXP", "family": "S32K1", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 180224, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "S32K116", "vendor": "NXP", "family": "S32K1", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 24576, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "S32K118", "vendor": "NXP", "family": "S32K1", "core": "Cortex-M0+",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # S32K3系列 (Cortex-M7)
            {"name": "S32K344", "vendor": "NXP", "family": "S32K3", "core": "Cortex-M7",
             "flash_size": 2097152, "ram_size": 327680, "package": "LQFP176", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "S32K348", "vendor": "NXP", "family": "S32K3", "core": "Cortex-M7",
             "flash_size": 4194304, "ram_size": 524288, "package": "LQFP176", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "S32K388", "vendor": "NXP", "family": "S32K3", "core": "Cortex-M7",
             "flash_size": 8388608, "ram_size": 786432, "package": "LFBGA329", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # MC9S12系列 (16位 HCS12)
            {"name": "MC9S12DG128", "vendor": "NXP", "family": "HCS12", "core": "HCS12",
             "flash_size": 131072, "ram_size": 8192, "package": "LQFP112", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S12DG256", "vendor": "NXP", "family": "HCS12", "core": "HCS12",
             "flash_size": 262144, "ram_size": 12288, "package": "LQFP112", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S12XS128", "vendor": "NXP", "family": "HCS12X", "core": "HCS12X",
             "flash_size": 131072, "ram_size": 8192, "package": "LQFP112", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S12XS256", "vendor": "NXP", "family": "HCS12X", "core": "HCS12X",
             "flash_size": 262144, "ram_size": 16384, "package": "LQFP112", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S12P64", "vendor": "NXP", "family": "S12P", "core": "HCS12",
             "flash_size": 65536, "ram_size": 4096, "package": "LQFP80", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S12P128", "vendor": "NXP", "family": "S12P", "core": "HCS12",
             "flash_size": 131072, "ram_size": 8192, "package": "LQFP80", "debug_interfaces": ["BDM"], "status": "supported"},
            
            # HCS08系列 (8位)
            {"name": "MC9S08QG8", "vendor": "NXP", "family": "HCS08", "core": "HCS08",
             "flash_size": 8192, "ram_size": 512, "package": "DIP8", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S08QG16", "vendor": "NXP", "family": "HCS08", "core": "HCS08",
             "flash_size": 16384, "ram_size": 1024, "package": "DIP8", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S08SL16", "vendor": "NXP", "family": "HCS08", "core": "HCS08",
             "flash_size": 16384, "ram_size": 1024, "package": "LQFP20", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S08SG32", "vendor": "NXP", "family": "HCS08", "core": "HCS08",
             "flash_size": 32768, "ram_size": 2048, "package": "LQFP32", "debug_interfaces": ["BDM"], "status": "supported"},
            {"name": "MC9S08GT60", "vendor": "NXP", "family": "HCS08", "core": "HCS08",
             "flash_size": 61440, "ram_size": 4096, "package": "LQFP64", "debug_interfaces": ["BDM"], "status": "supported"},
            
            # HC08系列 (8位 Legacy)
            {"name": "MC68HC908GP32", "vendor": "NXP", "family": "HC08", "core": "HC08",
             "flash_size": 32768, "ram_size": 1024, "package": "DIP40", "debug_interfaces": ["MON8"], "status": "supported"},
            {"name": "MC68HC908JK3", "vendor": "NXP", "family": "HC08", "core": "HC08",
             "flash_size": 4096, "ram_size": 128, "package": "DIP20", "debug_interfaces": ["MON8"], "status": "supported"},
        ]
        
        # ==================== 瑞萨系列 ====================
        renesas_chips = [
            # 78K系列 (8位)
            {"name": "uPD78F0503", "vendor": "Renesas", "family": "78K0", "core": "78K0",
             "flash_size": 32768, "ram_size": 2048, "package": "LQFP64", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "uPD78F0513", "vendor": "Renesas", "family": "78K0", "core": "78K0",
             "flash_size": 65536, "ram_size": 4096, "package": "LQFP64", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "uPD78F1166", "vendor": "Renesas", "family": "78K0R", "core": "78K0R",
             "flash_size": 262144, "ram_size": 16384, "package": "LQFP100", "debug_interfaces": ["UART"], "status": "supported"},
            
            # RL78系列 (16位)
            {"name": "RL78/G12", "vendor": "Renesas", "family": "RL78", "core": "RL78",
             "flash_size": 16384, "ram_size": 2048, "package": "LQFP48", "debug_interfaces": ["FINE"], "status": "supported"},
            {"name": "RL78/G13", "vendor": "Renesas", "family": "RL78", "core": "RL78",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["FINE"], "status": "supported"},
            {"name": "RL78/G14", "vendor": "Renesas", "family": "RL78", "core": "RL78",
             "flash_size": 131072, "ram_size": 16384, "package": "LQFP64", "debug_interfaces": ["FINE"], "status": "supported"},
            {"name": "RL78/G15", "vendor": "Renesas", "family": "RL78", "core": "RL78",
             "flash_size": 32768, "ram_size": 4096, "package": "LQFP48", "debug_interfaces": ["FINE"], "status": "supported"},
            {"name": "RL78/G16", "vendor": "Renesas", "family": "RL78", "core": "RL78",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["FINE"], "status": "supported"},
            
            # V850系列 (32位)
            {"name": "V850ES/FG3", "vendor": "Renesas", "family": "V850", "core": "V850ES",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP144", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "V850E2/IA2", "vendor": "Renesas", "family": "V850", "core": "V850E2",
             "flash_size": 1048576, "ram_size": 131072, "package": "LQFP176", "debug_interfaces": ["JTAG"], "status": "supported"},
            
            # RH850系列 (32位 汽车级)
            {"name": "RH850/F1L", "vendor": "Renesas", "family": "RH850", "core": "RH850",
             "flash_size": 1048576, "ram_size": 131072, "package": "LQFP176", "debug_interfaces": ["FINE"], "status": "supported"},
            {"name": "RH850/F1K", "vendor": "Renesas", "family": "RH850", "core": "RH850",
             "flash_size": 2097152, "ram_size": 262144, "package": "LQFP176", "debug_interfaces": ["FINE"], "status": "supported"},
            {"name": "RH850/D1M", "vendor": "Renesas", "family": "RH850", "core": "RH850",
             "flash_size": 4194304, "ram_size": 524288, "package": "LFBGA337", "debug_interfaces": ["FINE"], "status": "supported"},
            {"name": "RH850/E2M", "vendor": "Renesas", "family": "RH850", "core": "RH850",
             "flash_size": 8388608, "ram_size": 1048576, "package": "LFBGA624", "debug_interfaces": ["FINE"], "status": "supported"},
            
            # RA系列 (ARM Cortex)
            {"name": "RA2A1", "vendor": "Renesas", "family": "RA2", "core": "Cortex-M23",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "RA4M1", "vendor": "Renesas", "family": "RA4", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "RA4E1", "vendor": "Renesas", "family": "RA4", "core": "Cortex-M4F",
             "flash_size": 131072, "ram_size": 16384, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "RA6M1", "vendor": "Renesas", "family": "RA6", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "RA6M2", "vendor": "Renesas", "family": "RA6", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "RA6M3", "vendor": "Renesas", "family": "RA6", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 655360, "package": "LQFP176", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
        ]
        
        # ==================== TI系列 ====================
        ti_chips = [
            # MSP430系列 (16位超低功耗)
            {"name": "MSP430G2553", "vendor": "TI", "family": "MSP430G2", "core": "MSP430",
             "flash_size": 16384, "ram_size": 512, "package": "DIP20", "debug_interfaces": ["SBW", "JTAG"], "status": "supported"},
            {"name": "MSP430F5529", "vendor": "TI", "family": "MSP430F5", "core": "MSP430X",
             "flash_size": 131072, "ram_size": 8192, "package": "LQFP80", "debug_interfaces": ["SBW", "JTAG"], "status": "supported"},
            {"name": "MSP430FR5969", "vendor": "TI", "family": "MSP430FR", "core": "MSP430X",
             "flash_size": 65536, "ram_size": 2048, "package": "LQFP64", "debug_interfaces": ["SBW", "JTAG"], "status": "supported"},
            {"name": "MSP430FR6989", "vendor": "TI", "family": "MSP430FR", "core": "MSP430X",
             "flash_size": 131072, "ram_size": 2048, "package": "LQFP100", "debug_interfaces": ["SBW", "JTAG"], "status": "supported"},
            {"name": "MSP430FR2433", "vendor": "TI", "family": "MSP430FR", "core": "MSP430",
             "flash_size": 16384, "ram_size": 1024, "package": "LQFP48", "debug_interfaces": ["SBW"], "status": "supported"},
            {"name": "MSP430FR2633", "vendor": "TI", "family": "MSP430FR", "core": "MSP430",
             "flash_size": 32768, "ram_size": 1024, "package": "LQFP48", "debug_interfaces": ["SBW"], "status": "supported"},
            
            # MSP432系列 (Cortex-M4F)
            {"name": "MSP432P401R", "vendor": "TI", "family": "MSP432P4", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "MSP432P4111", "vendor": "TI", "family": "MSP432P4", "core": "Cortex-M4F",
             "flash_size": 2097152, "ram_size": 262144, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # TMS320C2000系列 (DSP)
            {"name": "TMS320F28027", "vendor": "TI", "family": "C2000", "core": "C28x",
             "flash_size": 65536, "ram_size": 10240, "package": "LQFP48", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TMS320F28069", "vendor": "TI", "family": "C2000", "core": "C28x",
             "flash_size": 262144, "ram_size": 49152, "package": "LQFP100", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TMS320F28379D", "vendor": "TI", "family": "C2000", "core": "C28x+CLA",
             "flash_size": 524288, "ram_size": 262144, "package": "LQFP176", "debug_interfaces": ["JTAG"], "status": "supported"},
            
            # Hercules系列 (汽车级)
            {"name": "TMS570LS3137", "vendor": "TI", "family": "Hercules", "core": "ARM Cortex-R4F",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP144", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "RM57L843", "vendor": "TI", "family": "Hercules", "core": "ARM Cortex-R5F",
             "flash_size": 2097152, "ram_size": 524288, "package": "LQFP176", "debug_interfaces": ["JTAG"], "status": "supported"},
            
            # CC2530系列 (ZigBee)
            {"name": "CC2530F256", "vendor": "TI", "family": "CC2530", "core": "8051",
             "flash_size": 262144, "ram_size": 8192, "package": "QFN40", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "CC2538SF53", "vendor": "TI", "family": "CC2538", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 32768, "package": "QFN48", "debug_interfaces": ["JTAG"], "status": "supported"},
        ]
        
        # ==================== 英飞凌AURIX系列 ====================
        infineon_chips = [
            # XMC系列
            {"name": "XMC1404", "vendor": "Infineon", "family": "XMC1", "core": "Cortex-M0+",
             "flash_size": 65536, "ram_size": 16384, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "XMC4400", "vendor": "Infineon", "family": "XMC4", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "XMC4700", "vendor": "Infineon", "family": "XMC4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 81920, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # TLE系列 (汽车级)
            {"name": "TLE9844", "vendor": "Infineon", "family": "TLE98", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "TLE9879", "vendor": "Infineon", "family": "TLE98", "core": "Cortex-M0+",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # AURIX TC2xx系列 (第一代)
            {"name": "TC222D", "vendor": "Infineon", "family": "AURIX-TC2xx", "core": "TriCore",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP144", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TC234", "vendor": "Infineon", "family": "AURIX-TC2xx", "core": "TriCore",
             "flash_size": 1048576, "ram_size": 262144, "package": "LQFP144", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TC264D", "vendor": "Infineon", "family": "AURIX-TC2xx", "core": "TriCore",
             "flash_size": 2097152, "ram_size": 393216, "package": "LQFP176", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TC275D", "vendor": "Infineon", "family": "AURIX-TC2xx", "core": "TriCore",
             "flash_size": 2097152, "ram_size": 524288, "package": "LQFP176", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TC297TP", "vendor": "Infineon", "family": "AURIX-TC2xx", "core": "TriCore",
             "flash_size": 4194304, "ram_size": 1048576, "package": "LFBGA416", "debug_interfaces": ["JTAG"], "status": "supported"},
            
            # AURIX TC3xx系列 (第二代)
            {"name": "TC334LE", "vendor": "Infineon", "family": "AURIX-TC3xx", "core": "TriCore",
             "flash_size": 4194304, "ram_size": 786432, "package": "LQFP176", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TC337TE", "vendor": "Infineon", "family": "AURIX-TC3xx", "core": "TriCore",
             "flash_size": 6291456, "ram_size": 1179648, "package": "LFBGA292", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TC375TP", "vendor": "Infineon", "family": "AURIX-TC3xx", "core": "TriCore",
             "flash_size": 8388608, "ram_size": 1572864, "package": "LFBGA416", "debug_interfaces": ["JTAG"], "status": "supported"},
            {"name": "TC397TE", "vendor": "Infineon", "family": "AURIX-TC3xx", "core": "TriCore",
             "flash_size": 8388608, "ram_size": 2097152, "package": "LFBGA384", "debug_interfaces": ["JTAG"], "status": "supported"},
        ]
        
        # ==================== 国产芯片系列 ====================
        domestic_chips = [
            # 国民技术 N32系列
            {"name": "N32G455VEL", "vendor": "Nationstech", "family": "N32G4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "N32G457VEL", "vendor": "Nationstech", "family": "N32G4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "N32G4FRML", "vendor": "Nationstech", "family": "N32G4", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "N32H78x", "vendor": "Nationstech", "family": "N32H7", "core": "Cortex-M7+M4",
             "flash_size": 1048576, "ram_size": 1048576, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "N32G031C8", "vendor": "Nationstech", "family": "N32G0", "core": "Cortex-M0+",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 华大半导体 HC32系列
            {"name": "HC32F4A0SIT", "vendor": "HDSC", "family": "HC32F4A", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 524288, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "HC32F460JETA", "vendor": "HDSC", "family": "HC32F460", "core": "Cortex-M4F",
             "flash_size": 262144, "ram_size": 196608, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "HC32F005C6PA", "vendor": "HDSC", "family": "HC32F005", "core": "Cortex-M0+",
             "flash_size": 32768, "ram_size": 4096, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "HC32L136K8TA", "vendor": "HDSC", "family": "HC32L136", "core": "Cortex-M0+",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP32", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 航顺芯片 HK32系列
            {"name": "HK32F103C8T6", "vendor": "HKMCU", "family": "HK32F103", "core": "Cortex-M3",
             "flash_size": 65536, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "HK32F103VET6", "vendor": "HKMCU", "family": "HK32F103", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "HK32F39AVET6", "vendor": "HKMCU", "family": "HK32F39A", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 98304, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "HK32F030C8T6", "vendor": "HKMCU", "family": "HK32F030", "core": "Cortex-M0",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 灵动微电子 MM32系列
            {"name": "MM32F103CBT6", "vendor": "MindMotion", "family": "MM32F1", "core": "Cortex-M3",
             "flash_size": 131072, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "MM32F3277G9EP", "vendor": "MindMotion", "family": "MM32F3", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "MM32F5277E9EP", "vendor": "MindMotion", "family": "MM32F5", "core": "Cortex-M33",
             "flash_size": 524288, "ram_size": 131072, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "MM32L373PSY", "vendor": "MindMotion", "family": "MM32L3", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 16384, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 极海半导体 APM32系列
            {"name": "APM32F103CBT6", "vendor": "Geehy", "family": "APM32F1", "core": "Cortex-M3",
             "flash_size": 131072, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "APM32F407VGT6", "vendor": "Geehy", "family": "APM32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 196608, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "APM32E103VET6", "vendor": "Geehy", "family": "APM32E1", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 65536, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "APM32L071RBT6", "vendor": "Geehy", "family": "APM32L0", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 20480, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 雅特力 AT32系列
            {"name": "AT32F403AVGT7", "vendor": "Artery", "family": "AT32F4", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 229376, "package": "LQFP100", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "AT32F415CBT7", "vendor": "Artery", "family": "AT32F4", "core": "Cortex-M4F",
             "flash_size": 131072, "ram_size": 32768, "package": "LQFP48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "AT32F437ZMT7", "vendor": "Artery", "family": "AT32F4", "core": "Cortex-M4F",
             "flash_size": 4194304, "ram_size": 524288, "package": "LQFP144", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # 东软载波 ES32系列
            {"name": "ES32F369xT", "vendor": "Eastsoft", "family": "ES32F3", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "ES32F092xT", "vendor": "Eastsoft", "family": "ES32F0", "core": "Cortex-M0",
             "flash_size": 65536, "ram_size": 8192, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 芯科 Silicon Labs系列
            {"name": "EFM32TG110F", "vendor": "Silicon Labs", "family": "EFM32TG", "core": "Cortex-M3",
             "flash_size": 32768, "ram_size": 8192, "package": "QFN32", "debug_interfaces": ["JTAG", "SWD"], "status": "supported"},
            {"name": "EFM32ZG110F", "vendor": "Silicon Labs", "family": "EFM32ZG", "core": "Cortex-M0+",
             "flash_size": 32768, "ram_size": 4096, "package": "QFN24", "debug_interfaces": ["JTAG", "SWD"], "status": "supported"},
            {"name": "EFM32HG110F", "vendor": "Silicon Labs", "family": "EFM32HG", "core": "Cortex-M0+",
             "flash_size": 16384, "ram_size": 2048, "package": "QFN24", "debug_interfaces": ["JTAG", "SWD"], "status": "supported"},
            {"name": "EFR32MG12P", "vendor": "Silicon Labs", "family": "EFM32MG", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 65536, "package": "QFN48", "debug_interfaces": ["JTAG", "SWD"], "status": "supported"},
            
            # 模拟器件 ADI系列
            {"name": "ADUCM3027", "vendor": "Analog Devices", "family": "ADuCM3xx", "core": "Cortex-M3",
             "flash_size": 262144, "ram_size": 32768, "package": "QFN48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            {"name": "ADUCM3029", "vendor": "Analog Devices", "family": "ADuCM3xx", "core": "Cortex-M3",
             "flash_size": 524288, "ram_size": 65536, "package": "QFN48", "debug_interfaces": ["SWD", "JTAG"], "status": "supported"},
            
            # Microchip PIC系列 (8位)
            {"name": "PIC16F877A", "vendor": "Microchip", "family": "PIC16", "core": "PIC",
             "flash_size": 14336, "ram_size": 368, "package": "DIP40", "debug_interfaces": ["ICSP"], "status": "supported"},
            {"name": "PIC16F887", "vendor": "Microchip", "family": "PIC16", "core": "PIC",
             "flash_size": 14336, "ram_size": 368, "package": "DIP40", "debug_interfaces": ["ICSP"], "status": "supported"},
            {"name": "PIC18F4550", "vendor": "Microchip", "family": "PIC18", "core": "PIC",
             "flash_size": 32768, "ram_size": 2048, "package": "DIP40", "debug_interfaces": ["ICSP"], "status": "supported"},
            {"name": "PIC18F4580", "vendor": "Microchip", "family": "PIC18", "core": "PIC",
             "flash_size": 32768, "ram_size": 1536, "package": "DIP40", "debug_interfaces": ["ICSP"], "status": "supported"},
            {"name": "PIC12F675", "vendor": "Microchip", "family": "PIC12", "core": "PIC",
             "flash_size": 1706, "ram_size": 64, "package": "DIP8", "debug_interfaces": ["ICSP"], "status": "supported"},
            
            # Microchip AVR系列 (8位)
            {"name": "ATmega328P", "vendor": "Microchip", "family": "ATmega", "core": "AVR",
             "flash_size": 32768, "ram_size": 2048, "package": "DIP28", "debug_interfaces": ["ISP"], "status": "supported"},
            {"name": "ATmega2560", "vendor": "Microchip", "family": "ATmega", "core": "AVR",
             "flash_size": 262144, "ram_size": 8192, "package": "TQFP100", "debug_interfaces": ["ISP"], "status": "supported"},
            {"name": "ATmega128", "vendor": "Microchip", "family": "ATmega", "core": "AVR",
             "flash_size": 131072, "ram_size": 4096, "package": "TQFP64", "debug_interfaces": ["ISP"], "status": "supported"},
            {"name": "ATtiny85", "vendor": "Microchip", "family": "ATtiny", "core": "AVR",
             "flash_size": 8192, "ram_size": 512, "package": "DIP8", "debug_interfaces": ["ISP"], "status": "supported"},
            {"name": "ATtiny2313", "vendor": "Microchip", "family": "ATtiny", "core": "AVR",
             "flash_size": 2048, "ram_size": 128, "package": "DIP20", "debug_interfaces": ["ISP"], "status": "supported"},
            
            # Microchip SAM系列 (32位 ARM)
            {"name": "ATSAMD21E18A", "vendor": "Microchip", "family": "SAMD21", "core": "Cortex-M0+",
             "flash_size": 262144, "ram_size": 32768, "package": "QFN32", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "ATSAMD51P20A", "vendor": "Microchip", "family": "SAMD51", "core": "Cortex-M4F",
             "flash_size": 1048576, "ram_size": 262144, "package": "QFN64", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "ATSAME51J18A", "vendor": "Microchip", "family": "SAME51", "core": "Cortex-M4F",
             "flash_size": 524288, "ram_size": 192512, "package": "QFN64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # STC 8051增强型系列
            {"name": "STC89C52RC", "vendor": "STC", "family": "STC89", "core": "8051",
             "flash_size": 8192, "ram_size": 512, "package": "DIP40", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "STC15W408AS", "vendor": "STC", "family": "STC15", "core": "8051",
             "flash_size": 4096, "ram_size": 256, "package": "DIP16", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "STC15F2K60S2", "vendor": "STC", "family": "STC15", "core": "8051",
             "flash_size": 61440, "ram_size": 2048, "package": "DIP40", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "STC12C5A60S2", "vendor": "STC", "family": "STC12", "core": "8051",
             "flash_size": 61440, "ram_size": 1280, "package": "DIP40", "debug_interfaces": ["UART"], "status": "supported"},
            
            # 新唐 Nuvoton 8051系列
            {"name": "N76E003", "vendor": "Nuvoton", "family": "N76E", "core": "8051",
             "flash_size": 18432, "ram_size": 768, "package": "TSSOP20", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "MS51XB9BE", "vendor": "Nuvoton", "family": "MS51", "core": "8051",
             "flash_size": 16384, "ram_size": 1024, "package": "QFN20", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "MG51FB9AE", "vendor": "Nuvoton", "family": "MG51", "core": "8051",
             "flash_size": 65536, "ram_size": 4096, "package": "QFN33", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "NANO100LC2BN", "vendor": "Nuvoton", "family": "NANO100", "core": "Cortex-M0",
             "flash_size": 131072, "ram_size": 32768, "package": "QFN48", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # WCH 沁恒 8051系列
            {"name": "CH549G", "vendor": "WCH", "family": "CH5xx", "core": "E8051",
             "flash_size": 32768, "ram_size": 2304, "package": "QFN28", "debug_interfaces": ["USB"], "status": "supported"},
            {"name": "CH548G", "vendor": "WCH", "family": "CH5xx", "core": "E8051",
             "flash_size": 32768, "ram_size": 2304, "package": "QFN28", "debug_interfaces": ["USB"], "status": "supported"},
            {"name": "CH32V103R8T6", "vendor": "WCH", "family": "CH32V1xx", "core": "RISC-V",
             "flash_size": 65536, "ram_size": 20480, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "CH32V203C8T6", "vendor": "WCH", "family": "CH32V2xx", "core": "RISC-V",
             "flash_size": 65536, "ram_size": 20480, "package": "LQFP48", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "CH32V303RCT6", "vendor": "WCH", "family": "CH32V3xx", "core": "RISC-V",
             "flash_size": 262144, "ram_size": 65536, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 赛元 MCU系列
            {"name": "SC95F7516", "vendor": "SinoMCU", "family": "SC9x", "core": "8051",
             "flash_size": 16384, "ram_size": 768, "package": "SOP16", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "SC95F7520", "vendor": "SinoMCU", "family": "SC9x", "core": "8051",
             "flash_size": 20480, "ram_size": 768, "package": "SOP20", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "MC51F7424", "vendor": "SinoMCU", "family": "MC5x", "core": "8051",
             "flash_size": 18432, "ram_size": 2304, "package": "QFN32", "debug_interfaces": ["UART"], "status": "supported"},
            
            # 中微爱芯 8051系列
            {"name": "CMS89F5x6", "vendor": "Chipone", "family": "CMS89", "core": "8051",
             "flash_size": 32768, "ram_size": 512, "package": "DIP16", "debug_interfaces": ["UART"], "status": "supported"},
            
            # 辉芒微 MCU系列
            {"name": "FT61F132", "vendor": "FMD", "family": "FT61F", "core": "8051",
             "flash_size": 32768, "ram_size": 2048, "package": "SOP16", "debug_interfaces": ["UART"], "status": "supported"},
            {"name": "FT61F012", "vendor": "FMD", "family": "FT61F", "core": "8051",
             "flash_size": 8192, "ram_size": 512, "package": "SOP8", "debug_interfaces": ["UART"], "status": "supported"},
            
            # 复旦微 FM系列
            {"name": "FM33LC041N", "vendor": "FudanMicro", "family": "FM33LC", "core": "Cortex-M0+",
             "flash_size": 131072, "ram_size": 16384, "package": "QFN32", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "FM33LG048", "vendor": "FudanMicro", "family": "FM33LG", "core": "Cortex-M0+",
             "flash_size": 262144, "ram_size": 32768, "package": "LQFP64", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 博流智能 BL系列
            {"name": "BL702", "vendor": "BouffaloLab", "family": "BL70x", "core": "RISC-V",
             "flash_size": 131072, "ram_size": 32768, "package": "QFN32", "debug_interfaces": ["SWD"], "status": "supported"},
            {"name": "BL616", "vendor": "BouffaloLab", "family": "BL61x", "core": "RISC-V",
             "flash_size": 262144, "ram_size": 65536, "package": "QFN40", "debug_interfaces": ["SWD"], "status": "supported"},
            
            # 安森美 onsemi RSL系列
            {"name": "RSL10", "vendor": "onsemi", "family": "RSL10", "core": "ARM Cortex-M3",
             "flash_size": 383488, "ram_size": 65536, "package": "QFN56", "debug_interfaces": ["SWD"], "status": "supported"},
        ]
        
        # 合并所有芯片
        self.chips = (stm32_chips + gd_chips + nxp_chips + renesas_chips + 
                      ti_chips + infineon_chips + domestic_chips)
        self._save_database()

    def search_chips(self, query: str, case_sensitive: bool = False) -> List[Dict[str, Any]]:
        """
        快速搜索芯片
        :param query: 搜索关键词
        :param case_sensitive: 是否区分大小写
        :return: 匹配的芯片列表
        """
        if not query:
            return self.chips

        query = query.strip()
        if not case_sensitive:
            query = query.lower()

        results = []
        for chip in self.chips:
            # 搜索字段
            fields_to_search = [
                chip.get('name', ''),
                chip.get('vendor', ''),
                chip.get('family', ''),
                chip.get('core', ''),
                chip.get('package', ''),
            ]

            # 检查是否匹配
            match = False
            for field in fields_to_search:
                if not case_sensitive:
                    if query in field.lower():
                        match = True
                        break
                else:
                    if query in field:
                        match = True
                        break

            if match:
                results.append(chip)

        return results

    def get_chip_by_name(self, name: str) -> Optional[Dict[str, Any]]:
        """通过芯片名称获取芯片信息"""
        for chip in self.chips:
            if chip.get('name') == name:
                return chip
        return None

    def get_all_families(self) -> List[str]:
        """获取所有芯片系列"""
        families = set()
        for chip in self.chips:
            family = chip.get('family')
            if family:
                families.add(family)
        return sorted(list(families))

    def get_all_vendors(self) -> List[str]:
        """获取所有芯片厂商"""
        vendors = set()
        for chip in self.chips:
            vendor = chip.get('vendor')
            if vendor:
                vendors.add(vendor)
        return sorted(list(vendors))

    def add_chip(self, chip: Dict[str, Any]):
        """添加新芯片到数据库"""
        if chip not in self.chips:
            self.chips.append(chip)
            self._save_database()

    def remove_chip(self, name: str):
        """从数据库移除芯片"""
        self.chips = [chip for chip in self.chips if chip.get('name') != name]
        self._save_database()


def format_size(size_bytes: int) -> str:
    """格式化显示大小"""
    if size_bytes >= 1048576:
        return f"{size_bytes / 1048576:.1f} MB"
    elif size_bytes >= 1024:
        return f"{size_bytes / 1024:.1f} KB"
    else:
        return f"{size_bytes} Bytes"


if __name__ == "__main__":
    # 测试代码
    db = ChipDatabase()
    print(f"数据库中共有 {len(db.chips)} 款芯片")
    print(f"\n芯片系列: {db.get_all_families()}")

    # 搜索测试
    results = db.search_chips("S32K")
    print(f"\n搜索 'S32K' 找到 {len(results)} 款芯片:")
    for chip in results:
        print(f"  - {chip['name']}: {format_size(chip['flash_size'])} Flash")
