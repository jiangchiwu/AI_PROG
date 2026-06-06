#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
驱动文件生成脚本
直接使用内联模板数据，确保UTF-8编码
"""

from datetime import datetime
from pathlib import Path

# 内联模板数据
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
                'address': '0xE0042000',
                'size': 4,
                'method': 'memory_read',
            },
            'Flash_ID': {
                'address': '0x1FFF7A10',
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
            '0x410': 'STM32F103xB',
            '0x411': 'STM32F101xB',
            '0x412': 'STM32F102xB',
            '0x414': 'STM32F101xE',
            '0x416': 'STM32F103xE',
            '0x418': 'STM32F103xG',
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

# 驱动代码模板
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


def generate_sequence_code(sequence):
    """生成操作序列代码"""
    if not sequence:
        return "pass"
    
    lines = []
    for step in sequence:
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


def generate_detect_code(id_detection):
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


def generate_driver(template_name, output_path=None):
    """生成单个驱动代码"""
    template = DRIVER_TEMPLATES.get(template_name)
    if not template:
        raise ValueError(f"模板 {template_name} 不存在")
    
    # 处理继承
    if 'inherits' in template:
        parent = DRIVER_TEMPLATES.get(template['inherits'])
        if parent:
            template = {**parent, **template}
    
    # 生成代码
    name = template.get('name', template_name)
    driver_id = template_name.replace('_', '').upper()
    vendor = template.get('vendor', 'Generic')
    core = template.get('core', template.get('core_pattern', 'Unknown'))
    debug_interfaces = template.get('debug_interfaces', ['SWD'])
    supported_families = [template.get('family_pattern', '*')]
    
    init_sequence = template.get('init_sequence', [])
    init_code = generate_sequence_code(init_sequence)
    
    id_detection = template.get('id_detection', {})
    detect_code = generate_detect_code(id_detection)
    
    erase_sequence = template.get('erase_sequence', [])
    erase_code = generate_sequence_code(erase_sequence)
    
    write_sequence = template.get('write_sequence', [])
    write_code = generate_sequence_code(write_sequence)
    
    read_sequence = template.get('read_sequence', [])
    read_code = generate_sequence_code(read_sequence)
    
    chip_id_map = template.get('id_map', {})
    
    code = TEMPLATE_CODE.format(
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
    
    if output_path:
        Path(output_path).write_text(code, encoding='utf-8')
    
    return code


def generate_all_drivers(output_dir):
    """生成所有驱动"""
    output_path = Path(output_dir)
    output_path.mkdir(exist_ok=True)
    
    for template_name in DRIVER_TEMPLATES.keys():
        try:
            driver_file = output_path / f"driver_{template_name}.py"
            generate_driver(template_name, str(driver_file))
            print(f"生成驱动: {driver_file}")
        except Exception as e:
            print(f"生成 {template_name} 失败: {e}")


if __name__ == '__main__':
    output_dir = "F:/work/AI_PROG/pc/generated_drivers"
    print("开始生成驱动文件...")
    print(f"输出目录: {output_dir}")
    print()
    generate_all_drivers(output_dir)
    print()
    print(f"驱动文件已生成到: {output_dir}")