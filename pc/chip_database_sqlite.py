#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
百万级芯片数据库 - SQLite版本

目标: 支持1,000,000种可编程芯片
架构:
    - SQLite数据库存储 (支持百万级数据)
    - 分层分类体系 (Vendor > Family > Series > Part)
    - 索引优化 (ID索引, 名称索引, 调试接口索引)
    - 插件式驱动框架
    - 自动识别引擎

数据库结构:
    - vendors: 厂商表
    - families: 系列表
    - series: 子系列表
    - chips: 芯片表 (主表)
    - debug_interfaces: 调试接口表
    - chip_debug_map: 芯片-调试接口映射表
    - drivers: 驱动模板表
    - chip_id_map: 芯片ID识别表

作者: AI_PROG项目
日期: 2026-06-03
版本: v2.0
"""

import sqlite3
import json
import os
from typing import List, Dict, Any, Optional, Tuple
from pathlib import Path
import hashlib
import re


class MillionChipDatabase:
    """百万级芯片数据库管理类"""
    
    # 数据库版本
    DB_VERSION = "2.0.0"
    
    # 默认数据库路径
    DEFAULT_DB_PATH = "chips_million.db"
    
    # 统计目标
    TARGET_CHIP_COUNT = 1_000_000
    
    def __init__(self, db_path: str = None):
        """
        初始化数据库
        :param db_path: 数据库文件路径, 默认为DEFAULT_DB_PATH
        """
        self.db_path = db_path or self.DEFAULT_DB_PATH
        self.conn = None
        self._init_database()
    
    def _init_database(self):
        """初始化数据库表结构"""
        self.conn = sqlite3.connect(self.db_path)
        self.conn.execute("PRAGMA journal_mode=WAL")  # 写前日志模式
        self.conn.execute("PRAGMA synchronous=NORMAL")  # 同步模式
        self.conn.execute("PRAGMA cache_size=10000")  # 缓存大小
        self.conn.execute("PRAGMA temp_store=MEMORY")  # 临时存储在内存
        
        # 创建表
        self._create_tables()
        self._create_indexes()
        self._insert_initial_data()
    
    def _create_tables(self):
        """创建数据库表"""
        
        # 厂商表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS vendors (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                short_name TEXT,
                country TEXT,
                website TEXT,
                description TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        """)
        
        # 系列表 (如STM32F1, GD32F4)
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS families (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                vendor_id INTEGER NOT NULL,
                name TEXT NOT NULL,
                core_type TEXT,
                architecture TEXT,
                description TEXT,
                FOREIGN KEY (vendor_id) REFERENCES vendors(id),
                UNIQUE(vendor_id, name)
            )
        """)
        
        # 子系列表 (如STM32F103)
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS series (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                family_id INTEGER NOT NULL,
                name TEXT NOT NULL,
                process_nm TEXT,
                voltage_range TEXT,
                temp_range TEXT,
                features TEXT,
                FOREIGN KEY (family_id) REFERENCES families(id),
                UNIQUE(family_id, name)
            )
        """)
        
        # 芯片主表 (百万级数据核心)
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS chips (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                series_id INTEGER NOT NULL,
                part_number TEXT UNIQUE NOT NULL,
                full_name TEXT,
                flash_size INTEGER DEFAULT 0,
                flash_size_unit TEXT DEFAULT 'KB',
                ram_size INTEGER DEFAULT 0,
                ram_size_unit TEXT DEFAULT 'KB',
                eeprom_size INTEGER DEFAULT 0,
                package_type TEXT,
                pin_count INTEGER DEFAULT 0,
                max_freq_mhz INTEGER DEFAULT 0,
                core_freq_mhz INTEGER DEFAULT 0,
                operating_voltage_min REAL DEFAULT 0,
                operating_voltage_max REAL DEFAULT 0,
                operating_temp_min INTEGER DEFAULT -40,
                operating_temp_max INTEGER DEFAULT 85,
                grade TEXT DEFAULT 'Commercial',
                status TEXT DEFAULT 'Active',
                lifecycle TEXT DEFAULT 'Production',
                features TEXT,
                peripherals TEXT,
                datasheet_url TEXT,
                reference_manual_url TEXT,
                user_manual_url TEXT,
                programming_manual_url TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (series_id) REFERENCES series(id)
            )
        """)
        
        # 调试接口表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS debug_interfaces (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                type TEXT NOT NULL,
                pins TEXT,
                speed_mhz_max INTEGER DEFAULT 1,
                protocol TEXT,
                description TEXT,
                supported INTEGER DEFAULT 1
            )
        """)
        
        # 芯片-调试接口映射表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS chip_debug_map (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                chip_id INTEGER NOT NULL,
                debug_interface_id INTEGER NOT NULL,
                is_primary INTEGER DEFAULT 0,
                notes TEXT,
                FOREIGN KEY (chip_id) REFERENCES chips(id),
                FOREIGN KEY (debug_interface_id) REFERENCES debug_interfaces(id),
                UNIQUE(chip_id, debug_interface_id)
            )
        """)
        
        # 驱动模板表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS driver_templates (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                family_pattern TEXT,
                series_pattern TEXT,
                core_type TEXT,
                debug_interface TEXT,
                template_code TEXT,
                init_function TEXT,
                erase_function TEXT,
                write_function TEXT,
                read_function TEXT,
                verify_function TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        """)
        
        # 芯片ID识别表 (用于自动识别芯片)
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS chip_id_map (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                chip_id INTEGER NOT NULL,
                id_type TEXT NOT NULL,
                id_value TEXT NOT NULL,
                id_mask TEXT,
                id_offset INTEGER DEFAULT 0,
                detection_method TEXT,
                priority INTEGER DEFAULT 0,
                notes TEXT,
                FOREIGN KEY (chip_id) REFERENCES chips(id),
                UNIQUE(id_type, id_value, chip_id)
            )
        """)
        
        # 分类标签表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS categories (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                parent_id INTEGER,
                level INTEGER DEFAULT 0,
                description TEXT,
                FOREIGN KEY (parent_id) REFERENCES categories(id)
            )
        """)
        
        # 芯片-分类映射表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS chip_category_map (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                chip_id INTEGER NOT NULL,
                category_id INTEGER NOT NULL,
                FOREIGN KEY (chip_id) REFERENCES chips(id),
                FOREIGN KEY (category_id) REFERENCES categories(id),
                UNIQUE(chip_id, category_id)
            )
        """)
        
        # 统计表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS statistics (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                stat_type TEXT UNIQUE NOT NULL,
                stat_value INTEGER DEFAULT 0,
                target_value INTEGER DEFAULT 0,
                last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        """)
        
        # 元数据表
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS metadata (
                key TEXT PRIMARY KEY,
                value TEXT,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        """)
    
    def _create_indexes(self):
        """创建索引以提高查询性能"""
        
        # 芯片表索引
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_chips_part ON chips(part_number)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_chips_series ON chips(series_id)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_chips_flash ON chips(flash_size)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_chips_status ON chips(status)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_chips_package ON chips(package_type)")
        
        # 厂商表索引
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_vendors_name ON vendors(name)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_vendors_country ON vendors(country)")
        
        # 系列表索引
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_families_vendor ON families(vendor_id)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_families_core ON families(core_type)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_families_arch ON families(architecture)")
        
        # 子系列表索引
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_series_family ON series(family_id)")
        
        # ID识别表索引
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_idmap_type ON chip_id_map(id_type)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_idmap_value ON chip_id_map(id_value)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_idmap_chip ON chip_id_map(chip_id)")
        
        # 调试接口映射索引
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_debugmap_chip ON chip_debug_map(chip_id)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_debugmap_interface ON chip_debug_map(debug_interface_id)")
    
    def _insert_initial_data(self):
        """插入初始数据"""
        
        # 插入统计目标
        self.conn.execute("""
            INSERT OR IGNORE INTO statistics (stat_type, stat_value, target_value)
            VALUES ('total_chips', 0, ?)
        """, (self.TARGET_CHIP_COUNT,))
        
        # 插入版本信息
        self.conn.execute("""
            INSERT OR IGNORE INTO metadata (key, value)
            VALUES ('version', ?)
        """, (self.DB_VERSION,))
        
        # 插入调试接口
        debug_interfaces = [
            ('SWD', 'ARM', 'SWCLK,SWDIO', 10, 'ARM Serial Wire Debug', 1),
            ('JTAG', 'ARM', 'TCK,TMS,TDI,TDO,TRST', 10, 'Joint Test Action Group', 1),
            ('BDM', 'NXP', 'BKPT,DS,RESET', 10, 'Background Debug Mode', 1),
            ('MON8', 'NXP', 'MON,RESET,VDD', 10, 'Monitor Mode 8-bit', 1),
            ('SBW', 'TI', 'SBWTCK,SBWTDIO', 10, 'Spy-Bi-Wire', 1),
            ('FINE', 'Renesas', 'FINE,RESET', 10, 'FINE Flash Interface', 1),
            ('ICSP', 'Microchip', 'PGC,PGD,MCLR', 1, 'In-Circuit Serial Programming', 1),
            ('ISP', 'AVR', 'MOSI,MISO,SCK,RST', 1, 'In-System Programming', 1),
            ('USB', 'WCH', 'USB_DP,USB_DM', 12, 'USB Programming', 1),
            ('UART', 'Generic', 'TX,RX', 1, 'UART Bootloader', 1),
            ('SPI', 'Generic', 'SCK,MOSI,MISO,CS', 10, 'SPI Flash Programming', 1),
            ('I2C', 'Generic', 'SCL,SDA', 1, 'I2C EEPROM Programming', 1),
            ('CAN', 'Automotive', 'CANH,CANL', 1, 'CAN Bus Programming', 1),
        ]
        
        for di in debug_interfaces:
            self.conn.execute("""
                INSERT OR IGNORE INTO debug_interfaces (name, type, pins, speed_mhz_max, protocol, supported)
                VALUES (?, ?, ?, ?, ?, ?)
            """, di)
        
        # 插入核心架构类型
        core_types = [
            ('ARM Cortex-M0', 'ARM', '32-bit'),
            ('ARM Cortex-M0+', 'ARM', '32-bit'),
            ('ARM Cortex-M3', 'ARM', '32-bit'),
            ('ARM Cortex-M4', 'ARM', '32-bit'),
            ('ARM Cortex-M4F', 'ARM', '32-bit+FPU'),
            ('ARM Cortex-M7', 'ARM', '32-bit'),
            ('ARM Cortex-M7F', 'ARM', '32-bit+FPU'),
            ('ARM Cortex-M23', 'ARM', '32-bit'),
            ('ARM Cortex-M33', 'ARM', '32-bit'),
            ('ARM Cortex-A', 'ARM', '32/64-bit'),
            ('ARM Cortex-R', 'ARM', '32-bit'),
            ('RISC-V', 'RISC-V', '32/64-bit'),
            ('MIPS', 'MIPS', '32/64-bit'),
            ('PIC', 'Microchip', '8-bit'),
            ('PIC24', 'Microchip', '16-bit'),
            ('dsPIC', 'Microchip', '16-bit+DSP'),
            ('AVR', 'Atmel', '8-bit'),
            ('8051', 'Intel', '8-bit'),
            ('MCS-51', 'Intel', '8-bit'),
            ('HCS08', 'Freescale', '8-bit'),
            ('HCS12', 'Freescale', '16-bit'),
            ('HCS12X', 'Freescale', '16-bit'),
            ('TriCore', 'Infineon', '32-bit'),
            ('V850', 'Renesas', '32-bit'),
            ('RL78', 'Renesas', '16-bit'),
            ('78K0', 'Renesas', '8-bit'),
            ('78K0R', 'Renesas', '16-bit'),
            ('RX', 'Renesas', '32-bit'),
            ('RH850', 'Renesas', '32-bit'),
            ('MSP430', 'TI', '16-bit'),
            ('C28x', 'TI', '32-bit'),
            ('C55x', 'TI', '32-bit+DSP'),
            ('C64x', 'TI', '64-bit+DSP'),
            ('PPC', 'IBM', '32/64-bit'),
            ('SH', 'Renesas', '32-bit'),
            ('ColdFire', 'Freescale', '32-bit'),
            ('DSP56K', 'Freescale', '24-bit+DSP'),
            ('X86', 'Intel', '32/64-bit'),
        ]
        
        self.conn.commit()
    
    def add_vendor(self, name: str, short_name: str = None, country: str = None, 
                   website: str = None, description: str = None) -> int:
        """
        添加厂商
        :return: 厂商ID
        """
        cursor = self.conn.execute("""
            INSERT INTO vendors (name, short_name, country, website, description)
            VALUES (?, ?, ?, ?, ?)
        """, (name, short_name or name, country, website, description))
        self.conn.commit()
        return cursor.lastrowid
    
    def add_family(self, vendor_id: int, name: str, core_type: str = None,
                   architecture: str = None, description: str = None) -> int:
        """
        添加芯片系列
        :return: 系列ID
        """
        cursor = self.conn.execute("""
            INSERT INTO families (vendor_id, name, core_type, architecture, description)
            VALUES (?, ?, ?, ?, ?)
        """, (vendor_id, name, core_type, architecture, description))
        self.conn.commit()
        return cursor.lastrowid
    
    def add_series(self, family_id: int, name: str, process_nm: str = None,
                   voltage_range: str = None, temp_range: str = None,
                   features: str = None) -> int:
        """
        添加子系列
        :return: 子系列ID
        """
        cursor = self.conn.execute("""
            INSERT INTO series (family_id, name, process_nm, voltage_range, temp_range, features)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (family_id, name, process_nm, voltage_range, temp_range, 
              json.dumps(features) if features else None))
        self.conn.commit()
        return cursor.lastrowid
    
    def add_chip(self, series_id: int, part_number: str, **kwargs) -> int:
        """
        添加芯片
        :param series_id: 子系列ID
        :param part_number: 芯片型号
        :param kwargs: 其他属性
        :return: 芯片ID
        """
        # 构建插入SQL
        columns = ['series_id', 'part_number']
        values = [series_id, part_number]
        
        optional_fields = {
            'full_name': kwargs.get('full_name'),
            'flash_size': kwargs.get('flash_size', 0),
            'flash_size_unit': kwargs.get('flash_size_unit', 'KB'),
            'ram_size': kwargs.get('ram_size', 0),
            'ram_size_unit': kwargs.get('ram_size_unit', 'KB'),
            'eeprom_size': kwargs.get('eeprom_size', 0),
            'package_type': kwargs.get('package_type'),
            'pin_count': kwargs.get('pin_count', 0),
            'max_freq_mhz': kwargs.get('max_freq_mhz', 0),
            'core_freq_mhz': kwargs.get('core_freq_mhz', 0),
            'operating_voltage_min': kwargs.get('operating_voltage_min', 0),
            'operating_voltage_max': kwargs.get('operating_voltage_max', 0),
            'operating_temp_min': kwargs.get('operating_temp_min', -40),
            'operating_temp_max': kwargs.get('operating_temp_max', 85),
            'grade': kwargs.get('grade', 'Commercial'),
            'status': kwargs.get('status', 'Active'),
            'lifecycle': kwargs.get('lifecycle', 'Production'),
            'features': json.dumps(kwargs.get('features')) if kwargs.get('features') else None,
            'peripherals': json.dumps(kwargs.get('peripherals')) if kwargs.get('peripherals') else None,
            'datasheet_url': kwargs.get('datasheet_url'),
            'reference_manual_url': kwargs.get('reference_manual_url'),
        }
        
        for field, value in optional_fields.items():
            if value is not None:
                columns.append(field)
                values.append(value)
        
        sql = f"INSERT INTO chips ({','.join(columns)}) VALUES ({','.join(['?']*len(values))})"
        cursor = self.conn.execute(sql, values)
        
        # 更新统计
        self._update_statistics('total_chips', increment=1)
        
        self.conn.commit()
        return cursor.lastrowid
    
    def add_chip_id(self, chip_id: int, id_type: str, id_value: str,
                    id_mask: str = None, id_offset: int = 0,
                    detection_method: str = 'JTAG', priority: int = 0,
                    notes: str = None) -> int:
        """
        添加芯片ID识别信息
        :param chip_id: 芯片ID
        :param id_type: ID类型 (JTAG_ID, Flash_ID, OTP_ID, etc.)
        :param id_value: ID值
        :param id_mask: ID掩码
        :param id_offset: ID偏移地址
        :param detection_method: 检测方法
        :param priority: 优先级
        :param notes: 备注
        :return: 映射ID
        """
        cursor = self.conn.execute("""
            INSERT INTO chip_id_map (chip_id, id_type, id_value, id_mask, id_offset,
                                     detection_method, priority, notes)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """, (chip_id, id_type, id_value, id_mask, id_offset,
              detection_method, priority, notes))
        self.conn.commit()
        return cursor.lastrowid
    
    def add_chip_debug(self, chip_id: int, debug_interface_name: str,
                       is_primary: bool = True, notes: str = None) -> int:
        """
        添加芯片调试接口映射
        :return: 映射ID
        """
        # 查找调试接口ID
        cursor = self.conn.execute(
            "SELECT id FROM debug_interfaces WHERE name = ?", (debug_interface_name,))
        result = cursor.fetchone()
        if not result:
            return -1
        
        debug_id = result[0]
        cursor = self.conn.execute("""
            INSERT INTO chip_debug_map (chip_id, debug_interface_id, is_primary, notes)
            VALUES (?, ?, ?, ?)
        """, (chip_id, debug_id, int(is_primary), notes))
        self.conn.commit()
        return cursor.lastrowid
    
    def search_chips(self, query: str, limit: int = 100) -> List[Dict]:
        """
        搜索芯片
        :param query: 搜索关键词
        :param limit: 返回数量限制
        :return: 芯片列表
        """
        # 构建搜索SQL
        search_sql = """
            SELECT c.id, c.part_number, c.full_name, c.flash_size, c.flash_size_unit,
                   c.ram_size, c.ram_size_unit, c.package_type, c.pin_count,
                   c.max_freq_mhz, c.status,
                   s.name as series_name, f.name as family_name, v.name as vendor_name,
                   v.short_name as vendor_short
            FROM chips c
            JOIN series s ON c.series_id = s.id
            JOIN families f ON s.family_id = f.id
            JOIN vendors v ON f.vendor_id = v.id
            WHERE c.part_number LIKE ? OR c.full_name LIKE ? 
                  OR s.name LIKE ? OR f.name LIKE ? OR v.name LIKE ?
            ORDER BY c.part_number
            LIMIT ?
        """
        
        pattern = f"%{query}%"
        cursor = self.conn.execute(search_sql, 
                                   (pattern, pattern, pattern, pattern, pattern, limit))
        
        results = []
        for row in cursor.fetchall():
            results.append({
                'id': row[0],
                'part_number': row[1],
                'full_name': row[2],
                'flash_size': row[3],
                'flash_size_unit': row[4],
                'ram_size': row[5],
                'ram_size_unit': row[6],
                'package': row[7],
                'pin_count': row[8],
                'freq_mhz': row[9],
                'status': row[10],
                'series': row[11],
                'family': row[12],
                'vendor': row[13],
                'vendor_short': row[14],
            })
        
        return results
    
    def identify_chip(self, id_value: str, id_type: str = 'JTAG_ID') -> Optional[Dict]:
        """
        通过ID值识别芯片
        :param id_value: 读取到的ID值
        :param id_type: ID类型
        :return: 芯片信息
        """
        # 精确匹配
        cursor = self.conn.execute("""
            SELECT c.id, c.part_number, c.full_name, c.flash_size, c.flash_size_unit,
                   c.ram_size, c.ram_size_unit, c.package_type,
                   s.name as series_name, f.name as family_name, v.name as vendor_name,
                   m.id_mask, m.detection_method
            FROM chip_id_map m
            JOIN chips c ON m.chip_id = c.id
            JOIN series s ON c.series_id = s.id
            JOIN families f ON s.family_id = f.id
            JOIN vendors v ON f.vendor_id = v.id
            WHERE m.id_type = ? AND m.id_value = ?
            ORDER BY m.priority DESC
            LIMIT 1
        """, (id_type, id_value))
        
        result = cursor.fetchone()
        if result:
            return {
                'id': result[0],
                'part_number': result[1],
                'full_name': result[2],
                'flash_size': result[3],
                'flash_size_unit': result[4],
                'ram_size': result[5],
                'ram_size_unit': result[6],
                'package': result[7],
                'series': result[8],
                'family': result[9],
                'vendor': result[10],
                'id_mask': result[11],
                'detection_method': result[12],
            }
        
        # 模糊匹配 (掩码匹配)
        cursor = self.conn.execute("""
            SELECT c.id, c.part_number, c.full_name, c.flash_size, c.flash_size_unit,
                   c.ram_size, c.ram_size_unit, c.package_type,
                   s.name as series_name, f.name as family_name, v.name as vendor_name,
                   m.id_mask, m.id_value as expected_id, m.detection_method
            FROM chip_id_map m
            JOIN chips c ON m.chip_id = c.id
            JOIN series s ON c.series_id = s.id
            JOIN families f ON s.family_id = f.id
            JOIN vendors v ON f.vendor_id = v.id
            WHERE m.id_type = ? AND m.id_mask IS NOT NULL
            ORDER BY m.priority DESC
        """, (id_type,))
        
        for row in cursor.fetchall():
            expected_id = row[12]
            mask = row[11]
            # 简化的掩码匹配逻辑
            if self._match_id_with_mask(id_value, expected_id, mask):
                return {
                    'id': row[0],
                    'part_number': row[1],
                    'full_name': row[2],
                    'flash_size': row[3],
                    'flash_size_unit': row[4],
                    'ram_size': row[5],
                    'ram_size_unit': row[6],
                    'package': row[7],
                    'series': row[8],
                    'family': row[9],
                    'vendor': row[10],
                    'id_mask': mask,
                    'expected_id': expected_id,
                    'detection_method': row[13],
                    'matched': 'mask',
                }
        
        return None
    
    def _match_id_with_mask(self, actual_id: str, expected_id: str, mask: str) -> bool:
        """
        使用掩码匹配ID
        """
        try:
            actual = int(actual_id, 16)
            expected = int(expected_id, 16)
            mask_val = int(mask, 16)
            return (actual & mask_val) == (expected & mask_val)
        except:
            return False
    
    def get_chip_debug_interfaces(self, chip_id: int) -> List[Dict]:
        """
        获取芯片支持的调试接口
        """
        cursor = self.conn.execute("""
            SELECT d.name, d.type, d.pins, d.speed_mhz_max, d.protocol,
                   m.is_primary, m.notes
            FROM chip_debug_map m
            JOIN debug_interfaces d ON m.debug_interface_id = d.id
            WHERE m.chip_id = ?
            ORDER BY m.is_primary DESC
        """, (chip_id,))
        
        results = []
        for row in cursor.fetchall():
            results.append({
                'name': row[0],
                'type': row[1],
                'pins': row[2],
                'max_speed_mhz': row[3],
                'protocol': row[4],
                'is_primary': bool(row[5]),
                'notes': row[6],
            })
        return results
    
    def get_statistics(self) -> Dict:
        """
        获取统计数据
        """
        stats = {}
        
        # 总芯片数
        cursor = self.conn.execute("""
            SELECT stat_value, target_value FROM statistics WHERE stat_type = 'total_chips'
        """)
        row = cursor.fetchone()
        if row:
            stats['total_chips'] = row[0]
            stats['target_chips'] = row[1]
            stats['progress_percent'] = round(row[0] / row[1] * 100, 2)
        
        # 厂商数
        cursor = self.conn.execute("SELECT COUNT(*) FROM vendors")
        stats['total_vendors'] = cursor.fetchone()[0]
        
        # 系列数
        cursor = self.conn.execute("SELECT COUNT(*) FROM families")
        stats['total_families'] = cursor.fetchone()[0]
        
        # 子系列数
        cursor = self.conn.execute("SELECT COUNT(*) FROM series")
        stats['total_series'] = cursor.fetchone()[0]
        
        # 调试接口数
        cursor = self.conn.execute("SELECT COUNT(*) FROM debug_interfaces WHERE supported = 1")
        stats['supported_debug_interfaces'] = cursor.fetchone()[0]
        
        return stats
    
    def _update_statistics(self, stat_type: str, value: int = None, increment: int = None):
        """
        更新统计数据
        """
        if increment:
            self.conn.execute("""
                UPDATE statistics SET stat_value = stat_value + ?, last_updated = CURRENT_TIMESTAMP
                WHERE stat_type = ?
            """, (increment, stat_type))
        elif value:
            self.conn.execute("""
                UPDATE statistics SET stat_value = ?, last_updated = CURRENT_TIMESTAMP
                WHERE stat_type = ?
            """, (value, stat_type))
    
    def import_from_json(self, json_file: str) -> int:
        """
        从JSON文件批量导入芯片数据
        :return: 导入数量
        """
        with open(json_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        count = 0
        
        # 导入厂商
        for vendor in data.get('vendors', []):
            try:
                self.add_vendor(
                    name=vendor['name'],
                    short_name=vendor.get('short_name'),
                    country=vendor.get('country'),
                    website=vendor.get('website'),
                    description=vendor.get('description')
                )
            except:
                pass
        
        # 导入芯片
        for chip in data.get('chips', []):
            try:
                # 查找或创建厂商/系列/子系列
                vendor_id = self._get_or_create_vendor(chip.get('vendor'))
                family_id = self._get_or_create_family(vendor_id, chip.get('family'), 
                                                        chip.get('core'))
                series_id = self._get_or_create_series(family_id, chip.get('series'))
                
                chip_id = self.add_chip(series_id, chip['name'], 
                                        full_name=chip.get('full_name'),
                                        flash_size=chip.get('flash_size'),
                                        ram_size=chip.get('ram_size'),
                                        package_type=chip.get('package'),
                                        status=chip.get('status', 'Active'))
                
                # 添加调试接口
                for debug in chip.get('debug_interfaces', []):
                    self.add_chip_debug(chip_id, debug)
                
                count += 1
            except Exception as e:
                print(f"导入失败: {chip.get('name')} - {e}")
        
        self.conn.commit()
        return count
    
    def _get_or_create_vendor(self, vendor_name: str) -> int:
        """获取或创建厂商"""
        cursor = self.conn.execute("SELECT id FROM vendors WHERE name = ?", (vendor_name,))
        result = cursor.fetchone()
        if result:
            return result[0]
        return self.add_vendor(vendor_name)
    
    def _get_or_create_family(self, vendor_id: int, family_name: str, core_type: str) -> int:
        """获取或创建系列"""
        cursor = self.conn.execute(
            "SELECT id FROM families WHERE vendor_id = ? AND name = ?", 
            (vendor_id, family_name))
        result = cursor.fetchone()
        if result:
            return result[0]
        return self.add_family(vendor_id, family_name, core_type)
    
    def _get_or_create_series(self, family_id: int, series_name: str) -> int:
        """获取或创建子系列"""
        cursor = self.conn.execute(
            "SELECT id FROM series WHERE family_id = ? AND name = ?", 
            (family_id, series_name or 'Default'))
        result = cursor.fetchone()
        if result:
            return result[0]
        return self.add_series(family_id, series_name or 'Default')
    
    def export_to_json(self, output_file: str, limit: int = None):
        """
        导出数据库到JSON文件
        """
        data = {
            'metadata': {
                'version': self.DB_VERSION,
                'export_date': str(datetime.now()),
            },
            'statistics': self.get_statistics(),
            'vendors': [],
            'families': [],
            'series': [],
            'chips': [],
            'debug_interfaces': [],
        }
        
        # 导出厂商
        cursor = self.conn.execute("SELECT * FROM vendors")
        for row in cursor.fetchall():
            data['vendors'].append({
                'id': row[0],
                'name': row[1],
                'short_name': row[2],
                'country': row[3],
                'website': row[4],
            })
        
        # 导出系列
        cursor = self.conn.execute("SELECT * FROM families")
        for row in cursor.fetchall():
            data['families'].append({
                'id': row[0],
                'vendor_id': row[1],
                'name': row[2],
                'core_type': row[3],
            })
        
        # 导出子系列
        cursor = self.conn.execute("SELECT * FROM series")
        for row in cursor.fetchall():
            data['series'].append({
                'id': row[0],
                'family_id': row[1],
                'name': row[2],
            })
        
        # 导出芯片
        sql = "SELECT * FROM chips"
        if limit:
            sql += f" LIMIT {limit}"
        
        cursor = self.conn.execute(sql)
        for row in cursor.fetchall():
            data['chips'].append({
                'id': row[0],
                'series_id': row[1],
                'part_number': row[2],
                'full_name': row[3],
                'flash_size': row[4],
                'ram_size': row[6],
                'package': row[9],
            })
        
        # 导出调试接口
        cursor = self.conn.execute("SELECT * FROM debug_interfaces")
        for row in cursor.fetchall():
            data['debug_interfaces'].append({
                'id': row[0],
                'name': row[1],
                'type': row[2],
                'pins': row[3],
            })
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
    
    def close(self):
        """关闭数据库连接"""
        if self.conn:
            self.conn.commit()
            self.conn.close()


# ==================== 批量导入工具 ====================

class ChipBatchImporter:
    """芯片批量导入工具"""
    
    # 预定义的厂商数据
    VENDORS_DATA = [
        # 国际主流厂商
        ('STMicroelectronics', 'ST', 'Switzerland', 'https://www.st.com'),
        ('NXP', 'NXP', 'Netherlands', 'https://www.nxp.com'),
        ('Texas Instruments', 'TI', 'USA', 'https://www.ti.com'),
        ('Infineon', 'IFX', 'Germany', 'https://www.infineon.com'),
        ('Renesas', 'RNS', 'Japan', 'https://www.renesas.com'),
        ('Microchip', 'MCHP', 'USA', 'https://www.microchip.com'),
        ('Analog Devices', 'ADI', 'USA', 'https://www.analog.com'),
        ('Silicon Labs', 'SL', 'USA', 'https://www.silabs.com'),
        ('Maxim Integrated', 'MAX', 'USA', 'https://www.maximintegrated.com'),
        ('ON Semiconductor', 'ON', 'USA', 'https://www.onsemi.com'),
        ('Cypress', 'CY', 'USA', 'https://www.cypress.com'),
        ('Samsung', 'SAM', 'Korea', 'https://www.samsung.com'),
        ('Nuvoton', 'NVT', 'Taiwan', 'https://www.nuvoton.com'),
        ('Fujitsu', 'FUJ', 'Japan', 'https://www.fujitsu.com'),
        ('Toshiba', 'TOS', 'Japan', 'https://www.toshiba.com'),
        ('ROHM', 'ROHM', 'Japan', 'https://www.rohm.com'),
        ('Epson', 'EP', 'Japan', 'https://www.epson.com'),
        ('OKI', 'OKI', 'Japan', 'https://www.oki.com'),
        ('NEC', 'NEC', 'Japan', 'https://www.nec.com'),
        ('Hitachi', 'HIT', 'Japan', 'https://www.hitachi.com'),
        ('Mitsubishi', 'MIT', 'Japan', 'https://www.mitsubishi.com'),
        ('Sanyo', 'SANYO', 'Japan', 'https://www.sanyo.com'),
        ('Sharp', 'SHARP', 'Japan', 'https://www.sharp.com'),
        ('Zilog', 'ZLG', 'USA', 'https://www.zilog.com'),
        ('Atmel', 'ATM', 'USA', 'https://www.microchip.com'),  # 已被Microchip收购
        
        # 中国大陆厂商
        ('GigaDevice', 'GD', 'China', 'https://www.gigadevice.com'),
        ('Nationstech', 'N32', 'China', 'https://www.nationstech.com'),
        ('HDSC', 'HC', 'China', 'https://www.hdsc.com.cn'),
        ('HKMCU', 'HK', 'China', 'http://www.hsmcu.com'),
        ('MindMotion', 'MM', 'China', 'https://www.mindmotion.com.cn'),
        ('Geehy', 'APM', 'China', 'https://www.geehy.com'),
        ('Artery', 'AT', 'China', 'https://www.arterytek.com'),
        ('Eastsoft', 'ES', 'China', 'https://www.supconit.com'),
        ('WCH', 'WCH', 'China', 'https://www.wch.cn'),
        ('SinoMCU', 'SC', 'China', 'https://sinomcu.com'),
        ('Chipone', 'CMS', 'China', 'https://www.chipone.net'),
        ('FMD', 'FT', 'China', 'https://www.fremicro.com'),
        ('FudanMicro', 'FM', 'China', 'https://www.fmsh.com'),
        ('BouffaloLab', 'BL', 'China', 'https://www.bouffalolab.com'),
        ('CW', 'CW', 'China', 'https://www.whxy.com'),
        ('Synwit', 'SW', 'China', 'https://www.synwit.cn'),
        ('Megawin', 'MG', 'China', 'https://www.megawin.com'),
        ('Realtek', 'RTL', 'China', 'https://www.realtek.com'),
        ('Allwinner', 'AW', 'China', 'https://www.allwinnertech.com'),
        ('Rockchip', 'RK', 'China', 'https://www.rock-chips.com'),
        ('Espressif', 'ESP', 'China', 'https://www.espressif.com'),
        ('C-SKY', 'CK', 'China', 'https://www.c-sky.com'),
        ('Hangshun', 'HK', 'China', 'https://www.hkmcu.com'),
        ('Aipu', 'AIPU', 'China', 'https://www.aipu.com'),
        ('Gowin', 'GW', 'China', 'https://www.gowinsemi.com'),
        
        # 欧美其他厂商
        ('ST', 'ST', 'Switzerland', 'https://www.st.com'),
        ('AMS', 'AMS', 'Austria', 'https://www.ams.com'),
        ('Melexis', 'MLX', 'Belgium', 'https://www.melexis.com'),
        ('Dialog', 'DLG', 'UK', 'https://www.dialog-semiconductor.com'),
        ('Nordic', 'NRF', 'Norway', 'https://www.nordicsemi.com'),
        ('XMOS', 'XMOS', 'UK', 'https://www.xmos.com'),
        ('Lattice', 'LSC', 'USA', 'https://www.latticesemi.com'),
        ('QuickLogic', 'QLOG', 'USA', 'https://www.quicklogic.com'),
        
        # 其他亚洲厂商
        ('MediaTek', 'MTK', 'Taiwan', 'https://www.mediatek.com'),
        ('Realtek', 'RTL', 'Taiwan', 'https://www.realtek.com'),
        ('ITE', 'ITE', 'Taiwan', 'https://www.ite.com.tw'),
        ('Genesys', 'GEN', 'Taiwan', 'https://www.genesyslogic.com'),
        ('ALi', 'ALi', 'Taiwan', 'https://www.ali.com.tw'),
        ('Winbond', 'WB', 'Taiwan', 'https://www.winbond.com'),
        ('Etron', 'ET', 'Taiwan', 'https://www.etron.com.tw'),
        ('Faraday', 'FT', 'Taiwan', 'https://www.faraday-tech.com'),
        ('GlobalFoundries', 'GF', 'Singapore', 'https://www.globalfoundries.com'),
        ('UMC', 'UMC', 'Taiwan', 'https://www.umc.com'),
        
        # 新兴RISC-V厂商
        ('SiFive', 'SF', 'USA', 'https://www.sifive.com'),
        ('Nuclei', 'NU', 'China', 'https://www.nucleisys.com'),
        ('T-Head', 'TH', 'China', 'https://www.t-head.cn'),
        ('Kendryte', 'K', 'China', 'https://kendryte.com'),
        
        # FPGA厂商
        ('Xilinx', 'XLX', 'USA', 'https://www.xilinx.com'),
        ('Intel FPGA', 'INTF', 'USA', 'https://www.intel.com'),
        ('Microsemi', 'MS', 'USA', 'https://www.microsemi.com'),
        ('Actel', 'ACT', 'USA', 'https://www.microchip.com'),
        
        # 存储器厂商
        ('Micron', 'MU', 'USA', 'https://www.micron.com'),
        ('SK Hynix', 'HYN', 'Korea', 'https://www.skhynix.com'),
        ('Kioxia', 'KIO', 'Japan', 'https://www.kioxia.com'),
        ('Western Digital', 'WD', 'USA', 'https://www.westerndigital.com'),
        ('Seagate', 'SEA', 'USA', 'https://www.seagate.com'),
        
        # 电源管理厂商
        ('Linear', 'LIN', 'USA', 'https://www.analog.com'),
        ('Power Integrations', 'PI', 'USA', 'https://www.powerint.com'),
        ('Monolithic Power', 'MPS', 'USA', 'https://www.monolithicpower.com'),
    ]
    
    def __init__(self, db: MillionChipDatabase):
        self.db = db
    
    def import_all_vendors(self):
        """导入所有预定义厂商"""
        count = 0
        for vendor in self.VENDORS_DATA:
            try:
                self.db.add_vendor(
                    name=vendor[0],
                    short_name=vendor[1],
                    country=vendor[2],
                    website=vendor[3]
                )
                count += 1
            except sqlite3.IntegrityError:
                pass  # 已存在
        return count
    
    def import_stm32_series(self):
        """导入STM32全系列"""
        vendor_id = self.db._get_or_create_vendor('STMicroelectronics')
        
        stm32_families = [
            ('STM32F0', 'ARM Cortex-M0', 'Entry level'),
            ('STM32F1', 'ARM Cortex-M3', 'Mainstream'),
            ('STM32F2', 'ARM Cortex-M3', 'High performance'),
            ('STM32F3', 'ARM Cortex-M4F', 'Mixed signal'),
            ('STM32F4', 'ARM Cortex-M4F', 'High performance'),
            ('STM32F7', 'ARM Cortex-M7', 'High performance'),
            ('STM32G0', 'ARM Cortex-M0+', 'Entry level'),
            ('STM32G4', 'ARM Cortex-M4F', 'Mixed signal'),
            ('STM32H7', 'ARM Cortex-M7', 'High performance'),
            ('STM32L0', 'ARM Cortex-M0+', 'Ultra low power'),
            ('STM32L1', 'ARM Cortex-M3', 'Ultra low power'),
            ('STM32L4', 'ARM Cortex-M4F', 'Ultra low power'),
            ('STM32L5', 'ARM Cortex-M33', 'Ultra low power'),
            ('STM32U5', 'ARM Cortex-M33', 'Ultra low power'),
            ('STM32W', 'ARM Cortex-M4F', 'Wireless'),
            ('STM32WB', 'ARM Cortex-M4F', 'Wireless BLE'),
            ('STM32WL', 'ARM Cortex-M4F', 'Wireless LoRa'),
        ]
        
        count = 0
        for family in stm32_families:
            try:
                family_id = self.db.add_family(vendor_id, family[0], family[1], 
                                               description=family[2])
                count += 1
            except:
                pass
        return count


# ==================== 使用示例 ====================

def main():
    """示例主函数"""
    # 创建数据库
    db = MillionChipDatabase("chips_million.db")
    
    # 批量导入厂商
    importer = ChipBatchImporter(db)
    vendor_count = importer.import_all_vendors()
    print(f"导入厂商: {vendor_count} 家")
    
    # 导入STM32系列
    stm32_count = importer.import_stm32_series()
    print(f"导入STM32系列: {stm32_count} 个")
    
    # 获取统计
    stats = db.get_statistics()
    print(f"\n数据库统计:")
    print(f"  总芯片数: {stats.get('total_chips', 0)}")
    print(f"  目标芯片数: {stats.get('target_chips', 1000000)}")
    print(f"  进度: {stats.get('progress_percent', 0)}%")
    print(f"  厂商数: {stats.get('total_vendors', 0)}")
    print(f"  系列数: {stats.get('total_families', 0)}")
    
    # 搜索示例
    results = db.search_chips("STM32", limit=5)
    print(f"\n搜索 'STM32' 结果:")
    for chip in results:
        print(f"  {chip['part_number']} - {chip['vendor']} {chip['family']}")
    
    # 关闭数据库
    db.close()


if __name__ == '__main__':
    main()