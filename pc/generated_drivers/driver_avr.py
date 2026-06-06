
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AVR系列驱动驱动
自动生成 - 基于模板: avr

厂商: Microchip
内核: AVR
调试接口: ['ISP']

生成日期: 2026-06-06
"""

from chip_driver_framework import ChipDriverBase


class DriverAVR系列驱动(ChipDriverBase):
    """AVR系列驱动系列芯片驱动"""
    
    driver_name = "AVR"
    driver_version = "1.0.0"
    supported_cores = ['AVR']
    supported_debug_interfaces = ['ISP']
    supported_vendors = ["Microchip"]
    supported_families = ['ATmega*,ATtiny*']
    
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
        debug_type = config.get('debug_interface', 'ISP')
        
        # 执行初始化序列
        # TODO: enter_isp_mode
        # TODO: read_signature
        # TODO: match_chip
        
        return True
    
    def detect(self) -> dict:
        """检测芯片"""
        # 读取芯片ID
        sig_byte_0 = self.debug_interface.read_memory(0, 1)
        sig_byte_1 = self.debug_interface.read_memory(1, 1)
        sig_byte_2 = self.debug_interface.read_memory(2, 1)
        self.chip_info = self._match_chip_by_ids({
            'SIG_BYTE_0': sig_byte_0,
            'SIG_BYTE_1': sig_byte_1,
            'SIG_BYTE_2': sig_byte_2,
        })
        return self.chip_info
    
    def get_info(self) -> dict:
        """获取芯片信息"""
        return self.chip_info
    
    def erase(self, address: int, size: int) -> bool:
        """擦除Flash"""
        # TODO: chip_erase
        self._wait_operation_complete()
        return True
    
    def write(self, address: int, data: bytes) -> bool:
        """写入Flash"""
        # TODO: write_page
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
CHIP_ID_MAP = {'0x1E950F': 'ATmega328P', '0x1E9414': 'ATmega128', '0x1E9801': 'ATmega2560', '0x1E9307': 'ATtiny85', '0x1E910A': 'ATtiny2313'}


def get_driver():
    """获取驱动实例"""
    return DriverAVR系列驱动()
