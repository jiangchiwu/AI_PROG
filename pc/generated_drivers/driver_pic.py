
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PIC系列驱动驱动
自动生成 - 基于模板: pic

厂商: Microchip
内核: PIC
调试接口: ['ICSP']

生成日期: 2026-06-06
"""

from chip_driver_framework import ChipDriverBase


class DriverPIC系列驱动(ChipDriverBase):
    """PIC系列驱动系列芯片驱动"""
    
    driver_name = "PIC"
    driver_version = "1.0.0"
    supported_cores = ['PIC']
    supported_debug_interfaces = ['ICSP']
    supported_vendors = ["Microchip"]
    supported_families = ['PIC*']
    
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
        debug_type = config.get('debug_interface', 'ICSP')
        
        # 执行初始化序列
        # TODO: enter_icsp_mode
        # TODO: read_device_id
        # TODO: match_chip
        
        return True
    
    def detect(self) -> dict:
        """检测芯片"""
        # 读取芯片ID
        dev_id = self.debug_interface.read_memory(262143, 4)
        rev_id = self.debug_interface.read_memory(262143, 4)
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
        # TODO: bulk_erase
        self._wait_operation_complete()
        return True
    
    def write(self, address: int, data: bytes) -> bool:
        """写入Flash"""
        # TODO: write_row
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
    return DriverPIC系列驱动()
