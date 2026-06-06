
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
8051系列通用驱动驱动
自动生成 - 基于模板: 8051

厂商: Generic
内核: 8051*
调试接口: ['UART', 'ISP']

生成日期: 2026-06-06
"""

from chip_driver_framework import ChipDriverBase


class Driver8051系列通用驱动(ChipDriverBase):
    """8051系列通用驱动系列芯片驱动"""
    
    driver_name = "8051"
    driver_version = "1.0.0"
    supported_cores = ['8051*']
    supported_debug_interfaces = ['UART', 'ISP']
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
        debug_type = config.get('debug_interface', 'UART')
        
        # 执行初始化序列
        # TODO: enter_bootloader
        # TODO: read_signature
        # TODO: match_chip
        
        return True
    
    def detect(self) -> dict:
        """检测芯片"""
        # 无ID检测配置
        pass
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
    return Driver8051系列通用驱动()
