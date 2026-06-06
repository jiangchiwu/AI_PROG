
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARM Cortex-M通用驱动驱动
自动生成 - 基于模板: arm_cortex_m

厂商: Generic
内核: Cortex-M*
调试接口: ['SWD', 'JTAG']

生成日期: 2026-06-06
"""

from chip_driver_framework import ChipDriverBase


class DriverARM Cortex-M通用驱动(ChipDriverBase):
    """ARM Cortex-M通用驱动系列芯片驱动"""
    
    driver_name = "ARMCORTEXM"
    driver_version = "1.0.0"
    supported_cores = ['Cortex-M*']
    supported_debug_interfaces = ['SWD', 'JTAG']
    supported_vendors = ["Generic"]
    supported_families = ['*']
    
    # 配置参数
    flash_unlock_key = [0]
    flash_base = '0x40000000'
    
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
        jtag_id = self.debug_interface.read_memory(0xE0042000, 4)
        flash_id = self.debug_interface.read_memory(0x1FFF7A10, 12)
        self.chip_info = self._match_chip_by_ids({
            'JTAG_ID': jtag_id,
            'Flash_ID': flash_id,
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
CHIP_ID_MAP = {}


def get_driver():
    """获取驱动实例"""
    return DriverARM Cortex-M通用驱动()
