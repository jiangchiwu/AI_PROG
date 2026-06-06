
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
STC系列驱动驱动
自动生成 - 基于模板: stc

厂商: STC
内核: 8051*
调试接口: ['UART', 'ISP']

生成日期: 2026-06-06
"""

from chip_driver_framework import ChipDriverBase


class DriverSTC系列驱动(ChipDriverBase):
    """STC系列驱动系列芯片驱动"""
    
    driver_name = "STC"
    driver_version = "1.0.0"
    supported_cores = ['8051*']
    supported_debug_interfaces = ['UART', 'ISP']
    supported_vendors = ["STC"]
    supported_families = ['STC*']
    
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
        debug_type = config.get('debug_interface', 'UART')
        
        # 执行初始化序列
        # TODO: enter_bootloader
        # TODO: read_signature
        # TODO: match_chip
        
        return True
    
    def detect(self) -> dict:
        """检测芯片"""
        # 读取芯片ID
        chip_id = self.debug_interface.read_memory(0x00000000, 2)
        self.chip_info = self._match_chip_by_ids({
            'CHIP_ID': chip_id,
        })
        return self.chip_info
    
    def get_info(self) -> dict:
        """获取芯片信息"""
        return self.chip_info
    
    def erase(self, address: int, size: int) -> bool:
        """擦除Flash"""
        # TODO: erase_flash
        self._wait_operation_complete()
        return True
    
    def write(self, address: int, data: bytes) -> bool:
        """写入Flash"""
        # TODO: write_block
        self._wait_operation_complete()
        return True
    
    def read(self, address: int, size: int) -> bytes:
        """读取Flash"""
        pass
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
    return DriverSTC系列驱动()
