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
        # NXP S32K系列
        s32k_chips = [
            {
                "name": "S32K148",
                "vendor": "NXP",
                "family": "S32K1",
                "core": "Cortex-M4F",
                "flash_size": 1048576,
                "ram_size": 180224,
                "package": "LQFP144",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "S32K146",
                "vendor": "NXP",
                "family": "S32K1",
                "core": "Cortex-M4F",
                "flash_size": 524288,
                "ram_size": 131072,
                "package": "LQFP144",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "S32K144",
                "vendor": "NXP",
                "family": "S32K1",
                "core": "Cortex-M4F",
                "flash_size": 262144,
                "ram_size": 65536,
                "package": "LQFP144",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "S32K142",
                "vendor": "NXP",
                "family": "S32K1",
                "core": "Cortex-M4F",
                "flash_size": 131072,
                "ram_size": 32768,
                "package": "LQFP144",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "S32K118",
                "vendor": "NXP",
                "family": "S32K1",
                "core": "Cortex-M0+",
                "flash_size": 262144,
                "ram_size": 32768,
                "package": "LQFP100",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "S32K116",
                "vendor": "NXP",
                "family": "S32K1",
                "core": "Cortex-M0+",
                "flash_size": 131072,
                "ram_size": 24576,
                "package": "LQFP100",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "S32K388",
                "vendor": "NXP",
                "family": "S32K3",
                "core": "Cortex-M7",
                "flash_size": 8388608,
                "ram_size": 786432,
                "package": "LFBGA329",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "planned"
            },
            {
                "name": "S32K348",
                "vendor": "NXP",
                "family": "S32K3",
                "core": "Cortex-M7",
                "flash_size": 4194304,
                "ram_size": 524288,
                "package": "LQFP176",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "planned"
            },
            {
                "name": "S32K344",
                "vendor": "NXP",
                "family": "S32K3",
                "core": "Cortex-M7",
                "flash_size": 2097152,
                "ram_size": 327680,
                "package": "LQFP176",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "planned"
            },
        ]

        # STM32系列（示例部分）
        stm32_chips = [
            {
                "name": "STM32F103C8",
                "vendor": "STMicroelectronics",
                "family": "STM32F1",
                "core": "Cortex-M3",
                "flash_size": 65536,
                "ram_size": 20480,
                "package": "LQFP48",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "STM32F103RC",
                "vendor": "STMicroelectronics",
                "family": "STM32F1",
                "core": "Cortex-M3",
                "flash_size": 262144,
                "ram_size": 49152,
                "package": "LQFP64",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "STM32F407VG",
                "vendor": "STMicroelectronics",
                "family": "STM32F4",
                "core": "Cortex-M4F",
                "flash_size": 1048576,
                "ram_size": 196608,
                "package": "LQFP100",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "STM32F411CE",
                "vendor": "STMicroelectronics",
                "family": "STM32F4",
                "core": "Cortex-M4F",
                "flash_size": 524288,
                "ram_size": 131072,
                "package": "UFQFPN48",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
            {
                "name": "STM32H750VB",
                "vendor": "STMicroelectronics",
                "family": "STM32H7",
                "core": "Cortex-M7",
                "flash_size": 131072,
                "ram_size": 1048576,
                "package": "LQFP100",
                "debug_interfaces": ["SWD", "JTAG"],
                "status": "supported"
            },
        ]

        self.chips = s32k_chips + stm32_chips
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
