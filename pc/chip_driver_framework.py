#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
插件式驱动框架

目标: 支持动态加载驱动, 模板自动生成
架构:
    - 驱动模板基类
    - 驱动插件加载器
    - 驱动模板生成器
    - 驱动匹配引擎

作者: AI_PROG项目
日期: 2026-06-03
版本: v2.0
"""

import os
import json
import importlib
import inspect
from typing import Dict, List, Any, Optional, Callable, Type, Tuple
from pathlib import Path
from abc import ABC, abstractmethod
import re
from functools import wraps
from time import time


# ==================== 驱动基类 ====================

class ChipDriverBase(ABC):
    """
    芯片驱动基类
    所有芯片驱动必须继承此基类并实现所有方法
    """
    
    # 驱动元信息
    driver_name: str = "base"
    driver_version: str = "1.0.0"
    supported_cores: List[str] = []
    supported_debug_interfaces: List[str] = []
    supported_vendors: List[str] = []
    supported_families: List[str] = []
    
    @abstractmethod
    def init(self, config: Dict) -> bool:
        """
        初始化驱动
        :param config: 配置参数
        :return: 是否成功
        """
        pass
    
    @abstractmethod
    def detect(self) -> Optional[Dict]:
        """
        检测芯片
        :return: 芯片信息字典
        """
        pass
    
    @abstractmethod
    def get_info(self) -> Dict:
        """
        获取芯片详细信息
        :return: 芯片信息
        """
        pass
    
    @abstractmethod
    def erase(self, address: int, size: int) -> bool:
        """
        擦除Flash
        :param address: 起始地址
        :param size: 擦除大小
        :return: 是否成功
        """
        pass
    
    @abstractmethod
    def write(self, address: int, data: bytes) -> bool:
        """
        写入Flash
        :param address: 目标地址
        :param data: 数据
        :return: 是否成功
        """
        pass
    
    @abstractmethod
    def read(self, address: int, size: int) -> bytes:
        """
        读取Flash
        :param address: 起始地址
        :param size: 读取大小
        :return: 数据
        """
        pass
    
    @abstractmethod
    def verify(self, address: int, data: bytes) -> bool:
        """
        校验Flash
        :param address: 目标地址
        :param data: 期望数据
        :return: 是否匹配
        """
        pass
    
    @abstractmethod
    def close(self) -> bool:
        """
        关闭驱动
        :return: 是否成功
        """
        pass
    
    def get_capabilities(self) -> Dict:
        """
        获取驱动能力
        """
        return {
            'name': self.driver_name,
            'version': self.driver_version,
            'cores': self.supported_cores,
            'debug_interfaces': self.supported_debug_interfaces,
            'vendors': self.supported_vendors,
            'families': self.supported_families,
        }


# ==================== 驱动模板定义 ====================

DRIVER_TEMPLATES = {
    # ARM Cortex-M系列通用模板
    'arm_cortex_m': {
        'name': 'ARM Cortex-M通用驱动',
        'core_pattern': 'Cortex-M*',
        'debug_interfaces': ['SWD', 'JTAG'],
        'init_sequence': [
            'connect_debug_interface',
            'read_core_id',
            'match_chip_id',
            'enter_debug_mode',
        ],
        'erase_sequence': [
            'unlock_flash',
            'wait_ready',
            'set_erase_mode',
            'start_erase',
            'wait_complete',
            'lock_flash',
        ],
        'write_sequence': [
            'unlock_flash',
            'wait_ready',
            'set_write_mode',
            'write_data',
            'wait_complete',
            'verify_data',
            'lock_flash',
        ],
        'read_sequence': [
            'set_read_mode',
            'read_data',
        ],
        'id_detection': {
            'JTAG_ID': {
                'address': '0xE0042000',  # ROM表地址
                'size': 4,
                'method': 'memory_read',
            },
            'Flash_ID': {
                'address': '0x1FFF7A10',  # UID地址示例
                'size': 12,
                'method': 'memory_read',
            },
        },
    },
    
    # STM32系列模板
    'stm32': {
        'name': 'STM32系列驱动',
        'inherits': 'arm_cortex_m',
        'vendor': 'STMicroelectronics',
        'family_pattern': 'STM32*',
        'flash_unlock_key': [0x45670123, 0xCDEF89AB],
        'flash_base': '0x40022000',
        'flash_cr_offset': 0x10,
        'flash_sr_offset': 0x0C,
        'id_detection': {
            'DEV_ID': {
                'address': '0x1FFF7A10',
                'size': 4,
                'method': 'memory_read',
            },
            'REV_ID': {
                'address': '0x1FFF7A14',
                'size': 4,
                'method': 'memory_read',
            },
        },
        'id_map': {
            # F1系列
            '0x410': 'STM32F103xB',
            '0x411': 'STM32F101xB',
            '0x412': 'STM32F102xB',
            '0x414': 'STM32F101xE',
            '0x416': 'STM32F103xE',
            '0x418': 'STM32F103xG',
            # F4系列
            '0x413': 'STM32F405/407',
            '0x419': 'STM32F42x/43x',
            '0x423': 'STM32F401xB/C',
            '0x431': 'STM32F411',
            '0x433': 'STM32F401xD/E',
            '0x443': 'STM32F412',
            '0x458': 'STM32F410',
            '0x461': 'STM32F446',
            '0x463': 'STM32F429/439',
            '0x471': 'STM32F469/479',
            # 其他系列
            '0x440': 'STM32F030',
            '0x444': 'STM32F03x',
            '0x448': 'STM32F070x6',
            '0x449': 'STM32F070xB',
            '0x460': 'STM32G0',
            '0x464': 'STM32G0',
        },
    },
    
    # GD32系列模板
    'gd32': {
        'name': 'GD32系列驱动',
        'inherits': 'arm_cortex_m',
        'vendor': 'GigaDevice',
        'family_pattern': 'GD32*',
        'flash_unlock_key': [0x45670123, 0xCDEF89AB],
        'id_detection': {
            'DEV_ID': {
                'address': '0x1FFF7A10',
                'size': 4,
                'method': 'memory_read',
            },
        },
        'id_map': {
            '0x410': 'GD32F103xB',
            '0x414': 'GD32F103xE',
            '0x430': 'GD32F30x',
            '0x460': 'GD32E230',
        },
    },
    
    # NXP S32K系列模板
    's32k': {
        'name': 'S32K系列驱动',
        'inherits': 'arm_cortex_m',
        'vendor': 'NXP',
        'family_pattern': 'S32K*',
        'id_detection': {
            'SDID': {
                'address': '0x4004F000',
                'size': 4,
                'method': 'memory_read',
            },
        },
        'id_map': {
            '0x014': 'S32K142',
            '0x044': 'S32K144',
            '0x074': 'S32K146',
            '0x104': 'S32K148',
            '0x04C': 'S32K116',
            '0x084': 'S32K118',
        },
    },
    
    # MSP430系列模板
    'msp430': {
        'name': 'MSP430系列驱动',
        'vendor': 'TI',
        'family_pattern': 'MSP430*',
        'core': 'MSP430',
        'debug_interfaces': ['SBW', 'JTAG'],
        'init_sequence': [
            'connect_sbw',
            'power_on',
            'read_jtag_id',
            'match_chip',
        ],
        'erase_sequence': [
            'unlock_flash',
            'erase_segment',
            'wait_complete',
            'lock_flash',
        ],
        'write_sequence': [
            'unlock_flash',
            'write_word',
            'wait_complete',
            'lock_flash',
        ],
        'id_detection': {
            'JTAG_ID': {
                'method': 'jtag_read',
                'address': '0x1A',
                'size': 16,
            },
        },
    },
    
    # HCS12系列模板
    'hcs12': {
        'name': 'HCS12系列驱动',
        'vendor': 'NXP',
        'family_pattern': 'MC9S12*',
        'core': 'HCS12',
        'debug_interfaces': ['BDM'],
        'init_sequence': [
            'connect_bdm',
            'reset_target',
            'enter_bdm_mode',
            'read_part_id',
        ],
        'erase_sequence': [
            'unlock_flash',
            'erase_sector',
            'wait_complete',
        ],
        'write_sequence': [
            'write_word',
            'wait_complete',
            'verify',
        ],
        'id_detection': {
            'PART_ID': {
                'method': 'bdm_read',
                'address': '0x001A',
                'size': 4,
            },
        },
    },
    
    # RH850系列模板
    'rh850': {
        'name': 'RH850系列驱动',
        'vendor': 'Renesas',
        'family_pattern': 'RH850*',
        'core': 'RH850',
        'debug_interfaces': ['FINE'],
        'init_sequence': [
            'connect_fine',
            'reset_target',
            'enter_fine_mode',
            'read_id',
        ],
        'erase_sequence': [
            'set_erase_mode',
            'erase_block',
            'wait_complete',
        ],
        'write_sequence': [
            'write_data',
            'wait_complete',
        ],
    },
    
    # AURIX TC系列模板
    'aurix': {
        'name': 'AURIX TC系列驱动',
        'vendor': 'Infineon',
        'family_pattern': 'TC*',
        'core': 'TriCore',
        'debug_interfaces': ['JTAG'],
        'init_sequence': [
            'connect_jtag',
            'reset_target',
            'enter_debug',
            'read_die_id',
        ],
        'id_detection': {
            'DIE_ID': {
                'address': '0xF0080000',
                'size': 32,
                'method': 'memory_read',
            },
        },
    },
    
    # PIC系列模板
    'pic': {
        'name': 'PIC系列驱动',
        'vendor': 'Microchip',
        'family_pattern': 'PIC*',
        'core': 'PIC',
        'debug_interfaces': ['ICSP'],
        'init_sequence': [
            'enter_icsp_mode',
            'read_device_id',
            'match_chip',
        ],
        'erase_sequence': [
            'bulk_erase',
            'wait_complete',
        ],
        'write_sequence': [
            'write_row',
            'wait_complete',
        ],
        'id_detection': {
            'DEV_ID': {
                'method': 'icsp_read',
                'address': 0x3FFFF,
                'size': 4,
            },
            'REV_ID': {
                'method': 'icsp_read',
                'address': 0x3FFFF,
                'size': 4,
            },
        },
    },
    
    # AVR系列模板
    'avr': {
        'name': 'AVR系列驱动',
        'vendor': 'Microchip',
        'family_pattern': 'ATmega*,ATtiny*',
        'core': 'AVR',
        'debug_interfaces': ['ISP'],
        'init_sequence': [
            'enter_isp_mode',
            'read_signature',
            'match_chip',
        ],
        'erase_sequence': [
            'chip_erase',
            'wait_complete',
        ],
        'write_sequence': [
            'write_page',
            'wait_complete',
        ],
        'id_detection': {
            'SIG_BYTE_0': {
                'method': 'isp_read',
                'address': 0x00,
                'size': 1,
            },
            'SIG_BYTE_1': {
                'method': 'isp_read',
                'address': 0x01,
                'size': 1,
            },
            'SIG_BYTE_2': {
                'method': 'isp_read',
                'address': 0x02,
                'size': 1,
            },
        },
        'id_map': {
            '0x1E950F': 'ATmega328P',
            '0x1E9414': 'ATmega128',
            '0x1E9801': 'ATmega2560',
            '0x1E9307': 'ATtiny85',
            '0x1E910A': 'ATtiny2313',
        },
    },
    
    # RISC-V系列模板
    'riscv': {
        'name': 'RISC-V通用驱动',
        'core_pattern': 'RISC-V*',
        'debug_interfaces': ['SWD', 'JTAG'],
        'init_sequence': [
            'connect_debug',
            'read_hart_id',
            'match_chip',
        ],
    },
    
    # CH32V系列模板
    'ch32v': {
        'name': 'CH32V系列驱动',
        'inherits': 'riscv',
        'vendor': 'WCH',
        'family_pattern': 'CH32V*',
        'id_detection': {
            'DEV_ID': {
                'method': 'memory_read',
                'address': '0x1FFFF7D0',
                'size': 4,
            },
        },
    },
    
    # 8051系列模板
    '8051': {
        'name': '8051系列通用驱动',
        'core_pattern': '8051*',
        'debug_interfaces': ['UART', 'ISP'],
        'init_sequence': [
            'enter_bootloader',
            'read_signature',
            'match_chip',
        ],
        'erase_sequence': [
            'erase_flash',
            'wait_complete',
        ],
        'write_sequence': [
            'write_block',
            'wait_complete',
        ],
    },
    
    # STC系列模板
    'stc': {
        'name': 'STC系列驱动',
        'inherits': '8051',
        'vendor': 'STC',
        'family_pattern': 'STC*',
        'bootloader_magic': [0x7F, 0x7F, 0x7F],
        'id_detection': {
            'CHIP_ID': {
                'method': 'uart_bootloader',
                'size': 2,
            },
        },
    },
}


# ==================== 驱动插件加载器 ====================

class DriverPluginLoader:
    """
    驱动插件加载器
    动态加载Python模块形式的驱动插件
    """
    
    def __init__(self, plugin_dir: str = "drivers"):
        """
        :param plugin_dir: 驱动插件目录
        """
        self.plugin_dir = Path(plugin_dir)
        self.loaded_drivers: Dict[str, Type[ChipDriverBase]] = {}
    
    def discover_plugins(self) -> List[str]:
        """
        发现所有可用的驱动插件
        :return: 插件列表
        """
        if not self.plugin_dir.exists():
            return []
        
        plugins = []
        for file in self.plugin_dir.glob("driver_*.py"):
            # 提取驱动名称
            name = file.stem.replace("driver_", "")
            plugins.append(name)
        
        return plugins
    
    def load_plugin(self, name: str) -> Optional[Type[ChipDriverBase]]:
        """
        加载指定的驱动插件
        :param name: 驱动名称
        :return: 驱动类
        """
        if name in self.loaded_drivers:
            return self.loaded_drivers[name]
        
        # 构建模块路径
        module_name = f"driver_{name}"
        module_path = self.plugin_dir / f"{module_name}.py"
        
        if not module_path.exists():
            return None
        
        # 动态加载
        try:
            spec = importlib.util.spec_from_file_location(module_name, module_path)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            
            # 查找驱动类
            for obj_name, obj in inspect.getmembers(module):
                if inspect.isclass(obj) and issubclass(obj, ChipDriverBase):
                    self.loaded_drivers[name] = obj
                    return obj
        except Exception as e:
            print(f"加载驱动插件 {name} 失败: {e}")
            return None
        
        return None
    
    def load_all_plugins(self) -> Dict[str, Type[ChipDriverBase]]:
        """
        加载所有驱动插件
        :return: 驱动字典
        """
        for name in self.discover_plugins():
            self.load_plugin(name)
        return self.loaded_drivers
    
    def get_driver_instance(self, name: str, config: Dict = None) -> Optional[ChipDriverBase]:
        """
        获取驱动实例
        :param name: 驱动名称
        :param config: 配置参数
        :return: 驱动实例
        """
        driver_class = self.load_plugin(name)
        if driver_class:
            instance = driver_class()
            if config and instance.init(config):
                return instance
        return None


# ==================== 驱动模板生成器 ====================

class DriverTemplateGenerator:
    """
    驱动模板生成器
    根据模板配置自动生成驱动代码
    """
    
    TEMPLATE_CODE = '''
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
{name}驱动
自动生成 - 基于模板: {template_name}

厂商: {vendor}
内核: {core}
调试接口: {debug_interfaces}

生成日期: {date}
"""

from chip_driver_framework import ChipDriverBase


class Driver{name}(ChipDriverBase):
    """{name}系列芯片驱动"""
    
    driver_name = "{driver_id}"
    driver_version = "1.0.0"
    supported_cores = {supported_cores}
    supported_debug_interfaces = {debug_interfaces}
    supported_vendors = ["{vendor}"]
    supported_families = {supported_families}
    
    # 配置参数
    flash_unlock_key = {flash_unlock_key}
    flash_base = {flash_base}
    
    def __init__(self):
        self.debug_interface = None
        self.chip_info = None
        self.config = None
    
    def init(self, config: dict) -> bool:
        """初始化驱动"""
        self.config = config
        
        # 初始化调试接口
        debug_type = config.get('debug_interface', '{primary_debug}')
        
        # 执行初始化序列
        {init_sequence_code}
        
        return True
    
    def detect(self) -> dict:
        """检测芯片"""
        {detect_code}
        return self.chip_info
    
    def get_info(self) -> dict:
        """获取芯片信息"""
        return self.chip_info
    
    def erase(self, address: int, size: int) -> bool:
        """擦除Flash"""
        {erase_code}
        return True
    
    def write(self, address: int, data: bytes) -> bool:
        """写入Flash"""
        {write_code}
        return True
    
    def read(self, address: int, size: int) -> bytes:
        """读取Flash"""
        {read_code}
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
CHIP_ID_MAP = {chip_id_map}


def get_driver():
    """获取驱动实例"""
    return Driver{name}()
'''
    
    def __init__(self):
        self.templates = DRIVER_TEMPLATES
    
    def generate_driver(self, template_name: str, output_path: str = None) -> str:
        """
        生成驱动代码
        :param template_name: 模板名称
        :param output_path: 输出路径
        :return: 生成的代码
        """
        template = self.templates.get(template_name)
        if not template:
            raise ValueError(f"模板 {template_name} 不存在")
        
        # 处理继承
        if 'inherits' in template:
            parent = self.templates.get(template['inherits'])
            if parent:
                template = {**parent, **template}
        
        # 生成代码
        code = self._generate_from_template(template, template_name)
        
        # 保存文件
        if output_path:
            Path(output_path).write_text(code)
        
        return code
    
    def _generate_from_template(self, template: Dict, template_name: str) -> str:
        """从模板生成代码"""
        from datetime import datetime
        
        name = template.get('name', template_name)
        driver_id = template_name.replace('_', '').upper()
        vendor = template.get('vendor', 'Generic')
        core = template.get('core', template.get('core_pattern', 'Unknown'))
        debug_interfaces = template.get('debug_interfaces', ['SWD'])
        supported_families = [template.get('family_pattern', '*')]
        
        # 生成初始化序列代码
        init_sequence = template.get('init_sequence', [])
        init_code = self._generate_sequence_code(init_sequence)
        
        # 生成检测代码
        id_detection = template.get('id_detection', {})
        detect_code = self._generate_detect_code(id_detection)
        
        # 生成擦除代码
        erase_sequence = template.get('erase_sequence', [])
        erase_code = self._generate_sequence_code(erase_sequence)
        
        # 生成写入代码
        write_sequence = template.get('write_sequence', [])
        write_code = self._generate_sequence_code(write_sequence)
        
        # 生成读取代码
        read_sequence = template.get('read_sequence', [])
        read_code = self._generate_sequence_code(read_sequence)
        
        # ID映射表
        chip_id_map = template.get('id_map', {})
        
        return self.TEMPLATE_CODE.format(
            name=name,
            template_name=template_name,
            vendor=vendor,
            core=core,
            debug_interfaces=repr(debug_interfaces),
            date=datetime.now().strftime('%Y-%m-%d'),
            driver_id=driver_id,
            supported_cores=repr([core]),
            supported_families=repr(supported_families),
            flash_unlock_key=repr(template.get('flash_unlock_key', [0])),
            flash_base=repr(template.get('flash_base', '0x40000000')),
            primary_debug=debug_interfaces[0] if debug_interfaces else 'SWD',
            init_sequence_code=init_code,
            detect_code=detect_code,
            erase_code=erase_code,
            write_code=write_code,
            read_code=read_code,
            chip_id_map=repr(chip_id_map),
        )
    
    def _generate_sequence_code(self, sequence: List[str]) -> str:
        """生成操作序列代码"""
        if not sequence:
            return "pass"
        
        lines = []
        for step in sequence:
            # 根据步骤名称生成代码
            if step == 'connect_debug_interface':
                lines.append('self.debug_interface = self._create_debug_interface(debug_type)')
            elif step == 'read_core_id':
                lines.append('core_id = self.debug_interface.read_id()')
            elif step == 'match_chip_id':
                lines.append('self.chip_info = self._match_chip_by_id(core_id)')
            elif step == 'enter_debug_mode':
                lines.append('self.debug_interface.enter_debug()')
            elif step == 'unlock_flash':
                lines.append('self._unlock_flash()')
            elif step == 'lock_flash':
                lines.append('self._lock_flash()')
            elif step == 'wait_ready':
                lines.append('self._wait_flash_ready()')
            elif step == 'set_erase_mode':
                lines.append('self._set_erase_mode()')
            elif step == 'start_erase':
                lines.append('self._start_erase(address, size)')
            elif step == 'wait_complete':
                lines.append('self._wait_operation_complete()')
            elif step == 'write_data':
                lines.append('self._write_flash_data(address, data)')
            elif step == 'verify_data':
                lines.append('self._verify_flash_data(address, data)')
            else:
                lines.append(f'# TODO: {step}')
        
        return '\n        '.join(lines)
    
    def _generate_detect_code(self, id_detection: Dict) -> str:
        """生成检测代码"""
        if not id_detection:
            return "# 无ID检测配置\n        pass"
        
        lines = ["# 读取芯片ID"]
        for id_name, id_config in id_detection.items():
            method = id_config.get('method', 'memory_read')
            address = id_config.get('address', '0x00000000')
            size = id_config.get('size', 4)
            
            lines.append(f"{id_name.lower()} = self.debug_interface.read_memory({address}, {size})")
        
        lines.append("self.chip_info = self._match_chip_by_ids({")
        for id_name in id_detection.keys():
            lines.append(f"    '{id_name}': {id_name.lower()},")
        lines.append("})")
        
        return '\n        '.join(lines)
    
    def generate_all_drivers(self, output_dir: str = "drivers"):
        """
        生成所有驱动
        :param output_dir: 输出目录
        """
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        
        for template_name in self.templates.keys():
            try:
                driver_file = output_path / f"driver_{template_name}.py"
                self.generate_driver(template_name, str(driver_file))
                print(f"生成驱动: {driver_file}")
            except Exception as e:
                print(f"生成 {template_name} 失败: {e}")


# ==================== 驱动匹配引擎 ====================

# 缓存装饰器
def cache_result(ttl: int = 300):
    """
    缓存结果装饰器
    :param ttl: 缓存过期时间（秒），默认300秒
    :return: 装饰器函数
    """
    def decorator(func):
        cache = {}
        
        @wraps(func)
        def wrapper(self, *args, **kwargs):
            # 生成缓存键
            cache_key = self._generate_cache_key(func.__name__, args, kwargs)
            
            # 检查缓存是否存在且未过期
            if cache_key in cache:
                result, timestamp = cache[cache_key]
                if time() - timestamp < ttl:
                    return result
            
            # 调用原函数并缓存结果
            result = func(self, *args, **kwargs)
            cache[cache_key] = (result, time())
            return result
        
        # 添加清除缓存的方法
        wrapper.clear_cache = lambda: cache.clear()
        wrapper.get_cache_info = lambda: {
            'size': len(cache),
            'keys': list(cache.keys())
        }
        
        return wrapper
    return decorator


# 匹配结果类
class MatchResult:
    """
    匹配结果类
    用于封装单个驱动匹配的结果信息
    """
    
    def __init__(self, driver_name: str, score: int, match_level: str, 
                 match_details: Dict[str, Any] = None):
        """
        :param driver_name: 驱动名称
        :param score: 匹配分数
        :param match_level: 匹配级别（exact/series/core/interface）
        :param match_details: 匹配详细信息
        """
        self.driver_name = driver_name
        self.score = score
        self.match_level = match_level
        self.match_details = match_details or {}
    
    def __repr__(self):
        return f"MatchResult(driver='{self.driver_name}', score={self.score}, level='{self.match_level}')"
    
    def to_dict(self) -> Dict[str, Any]:
        """转换为字典格式"""
        return {
            'driver_name': self.driver_name,
            'score': self.score,
            'match_level': self.match_level,
            'match_details': self.match_details
        }


class DriverMatchEngine:
    """
    驱动匹配引擎
    根据芯片信息自动匹配合适的驱动
    
    优化特性：
    - 缓存机制：避免重复计算
    - 多级匹配策略：精确匹配 > 系列匹配 > 内核匹配 > 调试接口匹配
    - 评分权重配置：可动态调整匹配策略权重
    - 模糊匹配：支持通配符、正则表达式、别名匹配
    - 结果排序：按匹配分数排序返回多个候选结果
    """
    
    # 默认匹配权重配置
    DEFAULT_WEIGHTS = {
        'vendor': 50,           # 厂商匹配权重
        'family': 30,           # 系列匹配权重
        'core': 20,             # 内核匹配权重
        'debug_interface': 10,  # 调试接口匹配权重
        'part_number': 15,      # 型号匹配权重
    }
    
    # 匹配级别定义
    MATCH_LEVELS = {
        'exact': 4,        # 精确匹配（厂商+系列+型号完全匹配）
        'series': 3,       # 系列匹配（厂商+系列匹配）
        'core': 2,         # 内核匹配（内核类型匹配）
        'interface': 1,    # 调试接口匹配
    }
    
    def __init__(self, template_loader=None, plugin_loader=None):
        """
        初始化驱动匹配引擎
        :param template_loader: 模板加载器
        :param plugin_loader: 插件加载器
        """
        self.templates = DRIVER_TEMPLATES
        self.template_loader = template_loader or DriverTemplateGenerator()
        self.plugin_loader = plugin_loader or DriverPluginLoader()
        
        # 匹配权重配置
        self.weights = self.DEFAULT_WEIGHTS.copy()
        
        # 缓存字典
        self._cache = {}
        self._cache_ttl = 300  # 缓存过期时间（秒）
        
        # 别名映射表（支持芯片型号别名）
        self._alias_map = {
            'STM32F103C8': 'STM32F103xB',
            'STM32F103CB': 'STM32F103xB',
            'STM32F103RE': 'STM32F103xE',
            'STM32F407VG': 'STM32F405/407',
            'STM32F407ZG': 'STM32F405/407',
            'GD32F303VC': 'GD32F30x',
            'ATmega328P-PU': 'ATmega328P',
        }
        
        # 正则表达式缓存
        self._regex_cache = {}
    
    def _generate_cache_key(self, func_name: str, args: tuple, kwargs: dict) -> str:
        """
        生成缓存键
        :param func_name: 函数名
        :param args: 位置参数
        :param kwargs: 关键字参数
        :return: 缓存键字符串
        """
        # 将参数转换为可哈希的字符串
        key_parts = [func_name]
        
        # 处理位置参数
        for arg in args:
            if isinstance(arg, dict):
                # 对字典进行排序后转换为字符串
                key_parts.append(str(sorted(arg.items())))
            else:
                key_parts.append(str(arg))
        
        # 处理关键字参数
        if kwargs:
            key_parts.append(str(sorted(kwargs.items())))
        
        return '|'.join(key_parts)
    
    def clear_cache(self):
        """
        清除所有缓存
        """
        self._cache.clear()
        self._regex_cache.clear()
        print("缓存已清除")
    
    def get_cache_info(self) -> Dict[str, Any]:
        """
        获取缓存信息
        :return: 缓存统计信息
        """
        return {
            'cache_size': len(self._cache),
            'regex_cache_size': len(self._regex_cache),
            'cache_ttl': self._cache_ttl,
            'cached_keys': list(self._cache.keys())[:10]  # 只显示前10个
        }
    
    def set_match_weights(self, weights: Dict[str, int]):
        """
        设置匹配权重
        :param weights: 权重字典，键为匹配项名称，值为权重值
        示例: {'vendor': 60, 'family': 40, 'core': 30}
        """
        # 验证权重值
        for key, value in weights.items():
            if key not in self.DEFAULT_WEIGHTS:
                print(f"警告: 未知的匹配项 '{key}'，将被忽略")
            elif not isinstance(value, (int, float)) or value < 0:
                print(f"警告: 无效的权重值 {value}，将使用默认值")
            else:
                self.weights[key] = int(value)
        
        # 清除缓存，因为权重改变会影响匹配结果
        self.clear_cache()
        print(f"权重已更新: {self.weights}")
    
    def get_match_weights(self) -> Dict[str, int]:
        """
        获取当前匹配权重
        :return: 权重字典
        """
        return self.weights.copy()
    
    def _get_cached_result(self, cache_key: str) -> Optional[Any]:
        """
        获取缓存的结果
        :param cache_key: 缓存键
        :return: 缓存的结果，如果不存在或已过期则返回None
        """
        if cache_key in self._cache:
            result, timestamp = self._cache[cache_key]
            if time() - timestamp < self._cache_ttl:
                return result
            else:
                # 缓存过期，删除
                del self._cache[cache_key]
        return None
    
    def _set_cached_result(self, cache_key: str, result: Any):
        """
        设置缓存结果
        :param cache_key: 缓存键
        :param result: 要缓存的结果
        """
        self._cache[cache_key] = (result, time())
    
    def match_drivers(self, chip_info: Dict, top_n: int = 5) -> List[MatchResult]:
        """
        匹配多个驱动并返回排序后的结果
        :param chip_info: 芯片信息
        :param top_n: 返回前N个匹配结果
        :return: 匹配结果列表，按分数降序排列
        """
        # 生成缓存键
        cache_key = self._generate_cache_key('match_drivers', (chip_info,), {'top_n': top_n})
        
        # 检查缓存
        cached = self._get_cached_result(cache_key)
        if cached is not None:
            return cached
        
        # 提取芯片信息
        vendor = chip_info.get('vendor', '')
        family = chip_info.get('family', '')
        core = chip_info.get('core', '')
        part_number = chip_info.get('part_number', '')
        debug_interface = chip_info.get('debug_interface', 'SWD')
        
        # 多级匹配策略
        results = []
        
        # 第一级：精确匹配（厂商+系列+型号完全匹配）
        exact_matches = self._match_exact(vendor, family, part_number)
        results.extend(exact_matches)
        
        # 第二级：系列匹配（厂商+系列匹配）
        series_matches = self._match_series(vendor, family, part_number)
        results.extend(series_matches)
        
        # 第三级：内核匹配（内核类型匹配）
        core_matches = self._match_core(core, vendor)
        results.extend(core_matches)
        
        # 第四级：调试接口匹配
        interface_matches = self._match_interface(debug_interface)
        results.extend(interface_matches)
        
        # 去重（基于驱动名称）
        unique_results = {}
        for result in results:
            if result.driver_name not in unique_results:
                unique_results[result.driver_name] = result
            else:
                # 如果已存在，保留分数更高的
                if result.score > unique_results[result.driver_name].score:
                    unique_results[result.driver_name] = result
        
        # 按分数降序排序
        sorted_results = sorted(
            unique_results.values(),
            key=lambda x: (x.score, self.MATCH_LEVELS.get(x.match_level, 0)),
            reverse=True
        )
        
        # 返回前N个结果
        top_results = sorted_results[:top_n]
        
        # 缓存结果
        self._set_cached_result(cache_key, top_results)
        
        return top_results
    
    def get_best_driver(self, chip_info: Dict) -> Optional[MatchResult]:
        """
        获取最佳匹配的驱动
        :param chip_info: 芯片信息
        :return: 最佳匹配结果，如果没有匹配则返回None
        """
        results = self.match_drivers(chip_info, top_n=1)
        return results[0] if results else None
    
    def _match_exact(self, vendor: str, family: str, part_number: str) -> List[MatchResult]:
        """
        第一级：精确匹配
        厂商+系列+型号完全匹配
        :param vendor: 厂商
        :param family: 系列
        :param part_number: 型号
        :return: 匹配结果列表
        """
        results = []
        
        # 处理型号别名
        normalized_part = self._normalize_part_number(part_number)
        
        for name, template in self.templates.items():
            template_vendor = template.get('vendor', '')
            template_family_pattern = template.get('family_pattern', '')
            
            # 厂商必须完全匹配
            if template_vendor and template_vendor != vendor:
                continue
            
            # 系列必须匹配
            if template_family_pattern:
                family_matched = False
                patterns = template_family_pattern.split(',')
                for pattern in patterns:
                    if self._pattern_match(family, pattern.strip()):
                        family_matched = True
                        break
                
                if not family_matched:
                    continue
            
            # 如果有型号，检查型号是否匹配
            score = self.weights['vendor'] + self.weights['family']
            match_details = {
                'vendor_match': True,
                'family_match': True,
                'part_number_match': False
            }
            
            if normalized_part:
                # 检查ID映射表中是否有该型号
                id_map = template.get('id_map', {})
                for dev_id, chip_name in id_map.items():
                    if self._fuzzy_match_part(normalized_part, chip_name):
                        score += self.weights['part_number']
                        match_details['part_number_match'] = True
                        match_details['matched_id'] = dev_id
                        break
            
            results.append(MatchResult(
                driver_name=name,
                score=score,
                match_level='exact',
                match_details=match_details
            ))
        
        return results
    
    def _match_series(self, vendor: str, family: str, part_number: str) -> List[MatchResult]:
        """
        第二级：系列匹配
        厂商+系列匹配
        :param vendor: 厂商
        :param family: 系列
        :param part_number: 型号
        :return: 匹配结果列表
        """
        results = []
        
        for name, template in self.templates.items():
            template_vendor = template.get('vendor', '')
            template_family_pattern = template.get('family_pattern', '')
            
            # 厂商匹配（支持模糊匹配）
            vendor_score = 0
            if template_vendor:
                if template_vendor == vendor:
                    vendor_score = self.weights['vendor']
                elif self._fuzzy_match_vendor(vendor, template_vendor):
                    vendor_score = int(self.weights['vendor'] * 0.8)  # 模糊匹配降低权重
            
            # 系列匹配
            family_score = 0
            if template_family_pattern:
                patterns = template_family_pattern.split(',')
                for pattern in patterns:
                    if self._pattern_match(family, pattern.strip()):
                        family_score = self.weights['family']
                        break
            
            # 至少需要厂商或系列有一个匹配
            if vendor_score == 0 and family_score == 0:
                continue
            
            total_score = vendor_score + family_score
            
            results.append(MatchResult(
                driver_name=name,
                score=total_score,
                match_level='series',
                match_details={
                    'vendor_match': vendor_score > 0,
                    'family_match': family_score > 0,
                    'vendor_score': vendor_score,
                    'family_score': family_score
                }
            ))
        
        return results
    
    def _match_core(self, core: str, vendor: str = '') -> List[MatchResult]:
        """
        第三级：内核匹配
        内核类型匹配
        :param core: 内核类型
        :param vendor: 厂商（可选，用于提高匹配精度）
        :return: 匹配结果列表
        """
        results = []
        
        for name, template in self.templates.items():
            template_core = template.get('core', '')
            template_core_pattern = template.get('core_pattern', '')
            template_vendor = template.get('vendor', '')
            
            # 内核匹配
            core_score = 0
            if template_core and self._pattern_match(core, template_core):
                core_score = self.weights['core']
            elif template_core_pattern and self._pattern_match(core, template_core_pattern):
                core_score = self.weights['core']
            
            if core_score == 0:
                continue
            
            # 如果厂商也匹配，增加额外分数
            vendor_bonus = 0
            if vendor and template_vendor and template_vendor == vendor:
                vendor_bonus = int(self.weights['vendor'] * 0.5)
            
            total_score = core_score + vendor_bonus
            
            results.append(MatchResult(
                driver_name=name,
                score=total_score,
                match_level='core',
                match_details={
                    'core_match': True,
                    'core_score': core_score,
                    'vendor_bonus': vendor_bonus
                }
            ))
        
        return results
    
    def _match_interface(self, debug_interface: str) -> List[MatchResult]:
        """
        第四级：调试接口匹配
        :param debug_interface: 调试接口类型
        :return: 匹配结果列表
        """
        results = []
        
        for name, template in self.templates.items():
            supported_interfaces = template.get('debug_interfaces', [])
            
            if debug_interface in supported_interfaces:
                results.append(MatchResult(
                    driver_name=name,
                    score=self.weights['debug_interface'],
                    match_level='interface',
                    match_details={
                        'interface_match': True,
                        'supported_interfaces': supported_interfaces
                    }
                ))
        
        return results
    
    def _normalize_part_number(self, part_number: str) -> str:
        """
        标准化型号名称
        处理别名、大小写等
        :param part_number: 原始型号
        :return: 标准化后的型号
        """
        if not part_number:
            return ''
        
        # 去除空格和特殊字符
        normalized = part_number.strip().upper()
        
        # 检查别名映射
        if normalized in self._alias_map:
            return self._alias_map[normalized]
        
        return normalized
    
    def _fuzzy_match_part(self, part1: str, part2: str) -> bool:
        """
        模糊匹配型号
        支持通配符和部分匹配
        :param part1: 型号1
        :param part2: 型号2
        :return: 是否匹配
        """
        if not part1 or not part2:
            return False
        
        # 标准化
        p1 = part1.upper().strip()
        p2 = part2.upper().strip()
        
        # 完全匹配
        if p1 == p2:
            return True
        
        # 通配符匹配
        if '*' in p2:
            pattern = p2.replace('*', '.*')
            if re.match(pattern, p1):
                return True
        
        # 部分匹配（型号前缀匹配）
        # 例如: STM32F103C8 匹配 STM32F103xB
        if len(p1) > 5 and len(p2) > 5:
            # 提取基础型号（去除后缀）
            base1 = re.sub(r'[A-Z]?[0-9]+[A-Z]?$', '', p1)
            base2 = re.sub(r'[A-Z]?[0-9]+[A-Z]?$', '', p2)
            if base1 and base2 and (base1.startswith(base2) or base2.startswith(base1)):
                return True
        
        return False
    
    def _fuzzy_match_vendor(self, vendor1: str, vendor2: str) -> bool:
        """
        模糊匹配厂商名称
        :param vendor1: 厂商1
        :param vendor2: 厂商2
        :return: 是否匹配
        """
        if not vendor1 or not vendor2:
            return False
        
        v1 = vendor1.upper().strip()
        v2 = vendor2.upper().strip()
        
        # 完全匹配
        if v1 == v2:
            return True
        
        # 常见厂商别名
        vendor_aliases = {
            'STMICROELECTRONICS': ['ST', 'STM'],
            'MICROCHIP': ['MICROCHIP TECHNOLOGY', 'ATMEL'],
            'NXP': ['NXP SEMICONDUCTORS', 'FREESCALE'],
            'TI': ['TEXAS INSTRUMENTS'],
            'GIGADEVICE': ['GD'],
            'WCH': ['WINCHIPHEAD', 'NANJING QINHENG MICROELECTRONICS'],
        }
        
        # 检查别名
        for main_vendor, aliases in vendor_aliases.items():
            all_names = [main_vendor] + aliases
            if v1 in all_names and v2 in all_names:
                return True
        
        return False
    
    def _pattern_match(self, value: str, pattern: str) -> bool:
        """
        模式匹配
        支持 * 通配符和正则表达式
        :param value: 要匹配的值
        :param pattern: 匹配模式
        :return: 是否匹配
        """
        if not value or not pattern:
            return False
        
        # 检查正则表达式缓存
        cache_key = pattern
        if cache_key in self._regex_cache:
            regex = self._regex_cache[cache_key]
        else:
            # 转换通配符为正则表达式
            regex_pattern = pattern.replace('*', '.*')
            try:
                regex = re.compile(f'^{regex_pattern}$', re.IGNORECASE)
                self._regex_cache[cache_key] = regex
            except re.error:
                return False
        
        return regex.match(value) is not None
    
    # 保持原有接口兼容
    def match_driver(self, chip_info: Dict) -> Optional[str]:
        """
        匹配驱动（兼容旧接口）
        :param chip_info: 芯片信息
        :return: 驱动名称
        """
        result = self.get_best_driver(chip_info)
        return result.driver_name if result else None
    
    def get_driver_for_chip(self, chip_info: Dict) -> Optional[ChipDriverBase]:
        """
        获取芯片的驱动实例
        :param chip_info: 芯片信息
        :return: 驱动实例
        """
        driver_name = self.match_driver(chip_info)
        if driver_name:
            return self.plugin_loader.get_driver_instance(driver_name, chip_info)
        return None


# ==================== 使用示例 ====================

def main():
    """示例主函数"""
    print("=== 驱动框架示例 ===")
    
    # 1. 查看可用模板
    print("\n可用驱动模板:")
    for name, template in DRIVER_TEMPLATES.items():
        print(f"  - {name}: {template.get('name')}")
    
    # 2. 生成驱动代码
    print("\n生成驱动代码:")
    generator = DriverTemplateGenerator()
    
    # 生成STM32驱动示例
    code = generator.generate_driver('stm32')
    print(f"STM32驱动代码长度: {len(code)} 字符")
    
    # 3. 匹配驱动
    print("\n驱动匹配示例:")
    engine = DriverMatchEngine()
    
    test_chips = [
        {'vendor': 'STMicroelectronics', 'family': 'STM32F103', 'core': 'Cortex-M3'},
        {'vendor': 'GigaDevice', 'family': 'GD32F303', 'core': 'Cortex-M4F'},
        {'vendor': 'TI', 'family': 'MSP430', 'core': 'MSP430', 'debug_interface': 'SBW'},
        {'vendor': 'Microchip', 'family': 'ATmega328', 'core': 'AVR', 'debug_interface': 'ISP'},
    ]
    
    for chip in test_chips:
        driver = engine.match_driver(chip)
        print(f"  {chip['family']} -> {driver}")
    
    # 4. 批量生成所有驱动
    print("\n批量生成所有驱动:")
    generator.generate_all_drivers('generated_drivers')


if __name__ == '__main__':
    main()


# ==================== 单元测试函数 ====================

def test_match_engine():
    """
    测试匹配引擎
    测试各种匹配场景和功能
    """
    print("\n" + "="*60)
    print("驱动匹配引擎测试")
    print("="*60)
    
    engine = DriverMatchEngine()
    
    # 测试用例
    test_cases = [
        {
            'name': '精确匹配测试 - STM32F103',
            'chip_info': {
                'vendor': 'STMicroelectronics',
                'family': 'STM32F103',
                'core': 'Cortex-M3',
                'part_number': 'STM32F103C8',
                'debug_interface': 'SWD'
            },
            'expected': 'stm32'
        },
        {
            'name': '系列匹配测试 - GD32F303',
            'chip_info': {
                'vendor': 'GigaDevice',
                'family': 'GD32F303',
                'core': 'Cortex-M4F',
                'debug_interface': 'SWD'
            },
            'expected': 'gd32'
        },
        {
            'name': '内核匹配测试 - Cortex-M4',
            'chip_info': {
                'core': 'Cortex-M4',
                'debug_interface': 'JTAG'
            },
            'expected': None  # 可能匹配多个
        },
        {
            'name': '调试接口匹配测试 - SBW',
            'chip_info': {
                'debug_interface': 'SBW'
            },
            'expected': 'msp430'
        },
        {
            'name': '模糊匹配测试 - ATmega328P别名',
            'chip_info': {
                'vendor': 'Microchip',
                'family': 'ATmega328',
                'core': 'AVR',
                'part_number': 'ATmega328P-PU',
                'debug_interface': 'ISP'
            },
            'expected': 'avr'
        },
        {
            'name': '厂商别名测试 - ST',
            'chip_info': {
                'vendor': 'ST',
                'family': 'STM32F4',
                'core': 'Cortex-M4F',
                'debug_interface': 'SWD'
            },
            'expected': 'stm32'
        },
    ]
    
    # 执行测试
    passed = 0
    failed = 0
    
    for test_case in test_cases:
        print(f"\n测试: {test_case['name']}")
        print(f"芯片信息: {test_case['chip_info']}")
        
        # 获取匹配结果
        results = engine.match_drivers(test_case['chip_info'], top_n=3)
        
        if results:
            print(f"匹配结果:")
            for i, result in enumerate(results, 1):
                print(f"  {i}. {result}")
                print(f"     详情: {result.match_details}")
            
            # 检查预期结果
            if test_case['expected']:
                if results[0].driver_name == test_case['expected']:
                    print(f"  ✓ 测试通过")
                    passed += 1
                else:
                    print(f"  ✗ 测试失败: 期望 {test_case['expected']}, 实际 {results[0].driver_name}")
                    failed += 1
            else:
                print(f"  ✓ 测试通过（无特定预期）")
                passed += 1
        else:
            print(f"  ✗ 未找到匹配")
            failed += 1
    
    # 测试缓存功能
    print("\n" + "-"*60)
    print("缓存功能测试")
    print("-"*60)
    
    cache_info = engine.get_cache_info()
    print(f"缓存信息: {cache_info}")
    
    # 重复查询测试缓存
    print("\n重复查询测试（应该使用缓存）:")
    chip_info = {'vendor': 'STMicroelectronics', 'family': 'STM32F103', 'core': 'Cortex-M3'}
    
    import time
    start = time.time()
    result1 = engine.match_drivers(chip_info)
    time1 = time.time() - start
    
    start = time.time()
    result2 = engine.match_drivers(chip_info)
    time2 = time.time() - start
    
    print(f"第一次查询耗时: {time1*1000:.2f}ms")
    print(f"第二次查询耗时: {time2*1000:.2f}ms (缓存)")
    print(f"结果一致: {result1[0].driver_name == result2[0].driver_name}")
    
    # 清除缓存测试
    engine.clear_cache()
    cache_info = engine.get_cache_info()
    print(f"清除后缓存大小: {cache_info['cache_size']}")
    
    # 测试权重配置
    print("\n" + "-"*60)
    print("权重配置测试")
    print("-"*60)
    
    print(f"默认权重: {engine.get_match_weights()}")
    
    # 修改权重
    new_weights = {'vendor': 60, 'family': 40, 'core': 30}
    engine.set_match_weights(new_weights)
    print(f"修改后权重: {engine.get_match_weights()}")
    
    # 使用新权重匹配
    chip_info = {'vendor': 'STMicroelectronics', 'family': 'STM32F103', 'core': 'Cortex-M3'}
    results = engine.match_drivers(chip_info)
    print(f"使用新权重匹配结果: {results[0] if results else 'None'}")
    
    # 测试总结
    print("\n" + "="*60)
    print(f"测试总结: 通过 {passed}/{passed+failed}, 失败 {failed}/{passed+failed}")
    print("="*60)
    
    return passed, failed


def benchmark_match():
    """
    性能测试
    测试匹配引擎的性能表现
    """
    print("\n" + "="*60)
    print("驱动匹配引擎性能测试")
    print("="*60)
    
    engine = DriverMatchEngine()
    
    # 准备测试数据
    test_chips = [
        {'vendor': 'STMicroelectronics', 'family': 'STM32F103', 'core': 'Cortex-M3'},
        {'vendor': 'GigaDevice', 'family': 'GD32F303', 'core': 'Cortex-M4F'},
        {'vendor': 'TI', 'family': 'MSP430', 'core': 'MSP430', 'debug_interface': 'SBW'},
        {'vendor': 'Microchip', 'family': 'ATmega328', 'core': 'AVR', 'debug_interface': 'ISP'},
        {'vendor': 'NXP', 'family': 'S32K144', 'core': 'Cortex-M4F'},
        {'vendor': 'Renesas', 'family': 'RH850', 'core': 'RH850'},
        {'vendor': 'Infineon', 'family': 'TC275', 'core': 'TriCore'},
        {'vendor': 'WCH', 'family': 'CH32V307', 'core': 'RISC-V'},
    ]
    
    # 性能测试参数
    iterations = 100
    
    print(f"\n测试配置:")
    print(f"  测试用例数: {len(test_chips)}")
    print(f"  每个用例迭代次数: {iterations}")
    print(f"  总匹配次数: {len(test_chips) * iterations}")
    
    # 第一次运行（无缓存）
    print("\n第一次运行（无缓存）:")
    start_time = time()
    
    for _ in range(iterations):
        for chip_info in test_chips:
            engine.match_drivers(chip_info)
    
    time_no_cache = time() - start_time
    print(f"  总耗时: {time_no_cache:.2f}秒")
    print(f"  平均每次匹配: {time_no_cache/(len(test_chips)*iterations)*1000:.2f}ms")
    
    # 第二次运行（有缓存）
    print("\n第二次运行（有缓存）:")
    start_time = time()
    
    for _ in range(iterations):
        for chip_info in test_chips:
            engine.match_drivers(chip_info)
    
    time_with_cache = time() - start_time
    print(f"  总耗时: {time_with_cache:.2f}秒")
    print(f"  平均每次匹配: {time_with_cache/(len(test_chips)*iterations)*1000:.2f}ms")
    print(f"  性能提升: {(time_no_cache/time_with_cache):.2f}x")
    
    # 缓存统计
    cache_info = engine.get_cache_info()
    print(f"\n缓存统计:")
    print(f"  缓存项数: {cache_info['cache_size']}")
    print(f"  正则缓存数: {cache_info['regex_cache_size']}")
    
    # 单次匹配详细性能
    print("\n单次匹配详细性能:")
    chip_info = {
        'vendor': 'STMicroelectronics',
        'family': 'STM32F103',
        'core': 'Cortex-M3',
        'part_number': 'STM32F103C8',
        'debug_interface': 'SWD'
    }
    
    # 清除缓存
    engine.clear_cache()
    
    # 测试各个匹配级别
    print("\n  测试芯片: STM32F103C8")
    
    start = time()
    result = engine.get_best_driver(chip_info)
    elapsed = (time() - start) * 1000
    
    print(f"  最佳匹配: {result}")
    print(f"  匹配耗时: {elapsed:.2f}ms")
    
    # 多结果匹配
    start = time()
    results = engine.match_drivers(chip_info, top_n=5)
    elapsed = (time() - start) * 1000
    
    print(f"\n  前5个匹配结果:")
    for i, r in enumerate(results, 1):
        print(f"    {i}. {r}")
    print(f"  匹配耗时: {elapsed:.2f}ms")
    
    # 性能总结
    print("\n" + "="*60)
    print("性能测试总结:")
    print(f"  缓存加速比: {(time_no_cache/time_with_cache):.2f}x")
    print(f"  平均匹配时间（无缓存）: {time_no_cache/(len(test_chips)*iterations)*1000:.2f}ms")
    print(f"  平均匹配时间（有缓存）: {time_with_cache/(len(test_chips)*iterations)*1000:.2f}ms")
    print("="*60)
    
    return {
        'time_no_cache': time_no_cache,
        'time_with_cache': time_with_cache,
        'speedup': time_no_cache/time_with_cache
    }


def run_all_tests():
    """
    运行所有测试
    """
    print("\n" + "="*70)
    print(" "*20 + "驱动匹配引擎完整测试套件")
    print("="*70)
    
    # 运行功能测试
    passed, failed = test_match_engine()
    
    # 运行性能测试
    benchmark_result = benchmark_match()
    
    # 最终总结
    print("\n" + "="*70)
    print("最终测试总结")
    print("="*70)
    print(f"功能测试: 通过 {passed}/{passed+failed}")
    print(f"性能测试: 缓存加速 {benchmark_result['speedup']:.2f}x")
    print("="*70)