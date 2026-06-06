
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
STM32系列驱动驱动
自动生成 - 基于模板: stm32

厂商: STMicroelectronics
内核: Cortex-M*
调试接口: ['SWD', 'JTAG']

生成日期: 2026-06-06
"""

from chip_driver_framework import ChipDriverBase


class DriverSTM32系列驱动(ChipDriverBase):
    """STM32系列驱动系列芯片驱动"""
    
    driver_name = "STM32"
    driver_version = "1.0.0"
    supported_cores = ['Cortex-M*']
    supported_debug_interfaces = ['SWD', 'JTAG']
    supported_vendors = ["STMicroelectronics"]
    supported_families = ['STM32*']
    
    # 配置参数
    flash_unlock_key = [1164378403, 3455027627]
    flash_base = '0x40022000'
    
    def __init__(self):
        self.debug_interface = None
        self.chip_info = None
        self.config = None
    
    def init(self, config: dict) -> bool:
        """初始化驱动"""
        self.config = config
        
        # 初始化调试接口
        debug_type = config.get('debug_interface', 'SWD')
        
        # 执行初始化序列
        self.debug_interface = self._create_debug_interface(debug_type)
        core_id = self.debug_interface.read_id()
        self.chip_info = self._match_chip_by_id(core_id)
        self.debug_interface.enter_debug()
        
        return True
    
    def detect(self) -> dict:
        """检测芯片"""
        # 读取芯片ID
        dev_id = self.debug_interface.read_memory(0x1FFF7A10, 4)
        rev_id = self.debug_interface.read_memory(0x1FFF7A14, 4)
        self.chip_info = self._match_chip_by_ids({
            'DEV_ID': dev_id,
            'REV_ID': rev_id,
        })
        return self.chip_info
    
    def get_info(self) -> dict:
        """获取芯片信息"""
        return self.chip_info
    
    def erase(self, address: int, size: int) -> bool:
        """擦除Flash"""
        self._unlock_flash()
        self._wait_flash_ready()
        self._set_erase_mode()
        self._start_erase(address, size)
        self._wait_operation_complete()
        self._lock_flash()
        return True
    
    def write(self, address: int, data: bytes) -> bool:
        """写入Flash"""
        self._unlock_flash()
        self._wait_flash_ready()
        # TODO: set_write_mode
        self._write_flash_data(address, data)
        self._wait_operation_complete()
        self._verify_flash_data(address, data)
        self._lock_flash()
        return True
    
    def read(self, address: int, size: int) -> bytes:
        """读取Flash"""
        # TODO: set_read_mode
        # TODO: read_data
        return self.debug_interface.read_memory(address, size)
    
    def verify(self, address: int, data: bytes) -> bool:
        """校验Flash"""
        read_data = self.read(address, len(data))
        return read_data == data
    
    def close(self) -> bool:
        """关闭驱动"""
        if self.debug_interface:
            self.debug_interface.close()
        return True


# 芯片ID映射表
CHIP_ID_MAP = {'0x410': 'STM32F103xB', '0x411': 'STM32F101xB', '0x412': 'STM32F102xB', '0x414': 'STM32F101xE', '0x416': 'STM32F103xE', '0x418': 'STM32F103xG', '0x413': 'STM32F405/407', '0x419': 'STM32F42x/43x', '0x423': 'STM32F401xB/C', '0x431': 'STM32F411', '0x433': 'STM32F401xD/E', '0x443': 'STM32F412', '0x458': 'STM32F410', '0x461': 'STM32F446', '0x463': 'STM32F429/439', '0x471': 'STM32F469/479', '0x440': 'STM32F030', '0x444': 'STM32F03x', '0x448': 'STM32F070x6', '0x449': 'STM32F070xB', '0x460': 'STM32G0', '0x464': 'STM32G0'}


def get_driver():
    """获取驱动实例"""
    return DriverSTM32系列驱动()
