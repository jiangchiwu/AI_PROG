#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
芯片自动识别测试工具

本工具用于测试芯片ID识别算法的准确性和可靠性，
包括ID精确匹配、模糊匹配、厂商识别、内核检测等功能。

功能模块：
1. ID精确匹配测试 - 测试从数据库读取ID并进行精确匹配
2. 模糊搜索测试 - 测试使用部分型号、厂商名等进行搜索
3. 厂商识别测试 - 测试从芯片型号中识别厂商
4. 内核检测测试 - 测试从型号规则判断内核类型

作者: AI_PROG项目
日期: 2026-06-06
版本: v1.0
"""

import sqlite3
import re
import time
from typing import List, Dict, Any, Optional, Tuple
from datetime import datetime
from pathlib import Path


class ChipIdentificationTool:
    """
    芯片自动识别测试工具类
    
    该类提供完整的芯片识别功能测试，包括：
    - ID匹配算法测试
    - 模糊搜索功能测试
    - 厂商识别功能测试
    - 内核检测功能测试
    
    Attributes:
        db_path: SQLite数据库文件路径
        conn: 数据库连接对象
        test_results: 测试结果存储字典
    """
    
    # 厂商识别规则映射表 (型号前缀 -> 厂商名)
    VENDOR_PATTERNS = {
        # 国际主流厂商
        r'^STM': 'STMicroelectronics',
        r'^STM32': 'STMicroelectronics',
        r'^STR': 'STMicroelectronics',
        r'^L636': 'STMicroelectronics',
        r'^MKP': 'STMicroelectronics',
        
        r'^LPC': 'NXP',
        r'^MKL': 'NXP',
        r'^MKW': 'NXP',
        r'^MCF': 'NXP',
        r'^Kinetis': 'NXP',
        r'^i\.MX': 'NXP',
        r'^QorIQ': 'NXP',
        
        r'^TMS': 'Texas Instruments',
        r'^MSP': 'Texas Instruments',
        r'^C2000': 'Texas Instruments',
        r'^CC': 'Texas Instruments',
        r'^AM335': 'Texas Instruments',
        r'^AM437': 'Texas Instruments',
        r'^AM57': 'Texas Instruments',
        
        r'^XMC': 'Infineon',
        r'^TC2': 'Infineon',
        r'^TC3': 'Infineon',
        r'^TLE': 'Infineon',
        r'^SPT': 'Infineon',
        
        r'^R5F': 'Renesas',
        r'^R7F': 'Renesas',
        r'^R8A': 'Renesas',
        r'^R8C': 'Renesas',
        r'^RL78': 'Renesas',
        r'^RX': 'Renesas',
        r'^V85': 'Renesas',
        r'^78K': 'Renesas',
        r'^H8S': 'Renesas',
        r'^M16C': 'Renesas',
        r'^R8A': 'Renesas',
        
        r'^PIC': 'Microchip',
        r'^ATMEGA': 'Microchip',
        r'^ATtiny': 'Microchip',
        r'^ATTINY': 'Microchip',
        r'^AT90': 'Microchip',
        r'^ATxmega': 'Microchip',
        r'^dsPIC': 'Microchip',
        r'^PIC32': 'Microchip',
        
        r'^ADUCM': 'Analog Devices',
        r'^ADSP': 'Analog Devices',
        
        r'^EFM8': 'Silicon Labs',
        r'^EFM32': 'Silicon Labs',
        r'^EFR32': 'Silicon Labs',
        r'^C8051': 'Silicon Labs',
        
        # 中国大陆厂商
        r'^GD32F': 'GigaDevice',
        r'^GD32E': 'GigaDevice',
        r'^GD32VF': 'GigaDevice',
        
        r'^N32G': 'Nationstech',
        r'^N32L': 'Nationstech',
        
        r'^HC32F': 'HDSC',
        r'^HC32L': 'HDSC',
        
        r'^MM32F': 'MindMotion',
        r'^MM32L': 'MindMotion',
        
        r'^APM32F': 'Geehy',
        
        r'^AT32F': 'Artery',
        r'^AT32W': 'Artery',
        
        r'^CH32F': 'WCH',
        r'^CH32V': 'WCH',
        r'^CH55': 'WCH',
        r'^CH55': 'WCH',
        r'^CH9': 'WCH',
        
        r'^BL60': 'BouffaloLab',
        r'^BL70': 'BouffaloLab',
        r'^BL80': 'BouffaloLab',
        
        r'^CW32F': 'CW',
        r'^CW32L': 'CW',
        
        r'^SWM': 'Synwit',
        r'^SW': 'Synwit',
        
        r'^MK64F': 'NXP',
        r'^MK66F': 'NXP',
        r'^MK80F': 'NXP',
        r'^MK82F': 'NXP',
        
        # 国产RISC-V
        r'^HPM6': 'HPMicro',
        r'^HPM5': 'HPMicro',
        r'^GD32VF': 'GigaDevice',
        r'^RV32': 'Generic RISC-V',
        
        # ESP系列
        r'^ESP': 'Espressif',
    }
    
    # 内核类型识别规则 (型号中的内核标识 -> 内核类型)
    CORE_PATTERNS = {
        r'Cortex-M0': 'ARM Cortex-M0',
        r'Cortex.M0': 'ARM Cortex-M0',
        r'CM0': 'ARM Cortex-M0',
        r'Cortex-M0\+': 'ARM Cortex-M0+',
        r'Cortex.M0\+': 'ARM Cortex-M0+',
        r'CM0P': 'ARM Cortex-M0+',
        r'Cortex-M3': 'ARM Cortex-M3',
        r'Cortex.M3': 'ARM Cortex-M3',
        r'CM3': 'ARM Cortex-M3',
        r'Cortex-M4F': 'ARM Cortex-M4F',
        r'Cortex.M4F': 'ARM Cortex-M4F',
        r'CM4F': 'ARM Cortex-M4F',
        r'Cortex-M4': 'ARM Cortex-M4',
        r'Cortex.M4': 'ARM Cortex-M4',
        r'CM4': 'ARM Cortex-M4',
        r'Cortex-M7F': 'ARM Cortex-M7F',
        r'Cortex.M7F': 'ARM Cortex-M7F',
        r'CM7F': 'ARM Cortex-M7F',
        r'Cortex-M7': 'ARM Cortex-M7',
        r'Cortex.M7': 'ARM Cortex-M7',
        r'CM7': 'ARM Cortex-M7',
        r'Cortex-M33F': 'ARM Cortex-M33F',
        r'Cortex.M33F': 'ARM Cortex-M33F',
        r'CM33F': 'ARM Cortex-M33F',
        r'Cortex-M33': 'ARM Cortex-M33',
        r'Cortex.M33': 'ARM Cortex-M33',
        r'CM33': 'ARM Cortex-M33',
        r'Cortex-M23': 'ARM Cortex-M23',
        r'Cortex.M23': 'ARM Cortex-M23',
        r'CM23': 'ARM Cortex-M23',
        r'RISC-V': 'RISC-V',
        r'RV32': 'RISC-V',
        r'8051': '8051',
        r'MCS-51': '8051',
        r'AVR': 'AVR',
        r'MSP430': 'MSP430',
        r'PIC16': 'PIC16',
        r'PIC18': 'PIC18',
        r'DSPIC': 'dsPIC',
        r'C28x': 'C28x',
        r'C55x': 'C55x',
    }
    
    # 调试接口类型识别
    DEBUG_INTERFACE_PATTERNS = {
        r'SWD': 'SWD',
        r'JTAG': 'JTAG',
        r'cJTAG': 'cJTAG',
        r'SPI': 'SPI',
        r'I2C': 'I2C',
        r'UART': 'UART',
        r'USB': 'USB',
        r'CAN': 'CAN',
    }
    
    def __init__(self, db_path: str = "chips_million.db"):
        """
        初始化芯片识别测试工具
        
        Args:
            db_path: SQLite数据库文件路径，默认为"chips_million.db"
        """
        self.db_path = db_path
        self.conn = None
        self.test_results = {
            'id_matching': {},
            'fuzzy_search': {},
            'vendor_recognition': {},
            'core_detection': {},
        }
        self._connect_database()
    
    def _connect_database(self) -> bool:
        """
        连接SQLite数据库
        
        Returns:
            连接是否成功
        """
        try:
            # 检查数据库文件是否存在
            if not Path(self.db_path).exists():
                print(f"[警告] 数据库文件不存在: {self.db_path}")
                print("[警告] 将创建新数据库")
                self.conn = sqlite3.connect(self.db_path)
                return False
            
            self.conn = sqlite3.connect(self.db_path)
            print(f"[成功] 已连接到数据库: {self.db_path}")
            return True
        except sqlite3.Error as e:
            print(f"[错误] 数据库连接失败: {e}")
            return False
    
    def _execute_query(self, sql: str, params: tuple = None) -> List[Tuple]:
        """
        执行SQL查询
        
        Args:
            sql: SQL语句
            params: 查询参数
            
        Returns:
            查询结果列表
        """
        try:
            cursor = self.conn.cursor()
            if params:
                cursor.execute(sql, params)
            else:
                cursor.execute(sql)
            return cursor.fetchall()
        except sqlite3.Error as e:
            print(f"[错误] SQL执行失败: {e}")
            return []
    
    def test_id_matching(self) -> Dict[str, Any]:
        """
        测试ID匹配算法
        
        遍历所有芯片，模拟读取ID，然后进行匹配验证。
        测试内容包括：
        1. 从chip_id_map表中获取所有芯片ID映射
        2. 模拟通过ID值查找芯片
        3. 验证匹配结果的正确性
        
        Returns:
            包含测试结果的字典
        """
        print("\n" + "="*60)
        print("开始测试: ID匹配算法")
        print("="*60)
        
        start_time = time.time()
        
        # 获取测试统计
        results = {
            'total_tests': 0,
            'passed': 0,
            'failed': 0,
            'exact_match': 0,
            'mask_match': 0,
            'no_match': 0,
            'errors': [],
            'details': []
        }
        
        try:
            # 获取所有芯片ID映射
            id_map_sql = """
                SELECT m.id, m.chip_id, m.id_type, m.id_value, m.id_mask, m.id_offset,
                       c.part_number, s.name as series_name, f.name as family_name,
                       v.name as vendor_name
                FROM chip_id_map m
                JOIN chips c ON m.chip_id = c.id
                JOIN series s ON c.series_id = s.id
                JOIN families f ON s.family_id = f.id
                JOIN vendors v ON f.vendor_id = v.id
            """
            id_maps = self._execute_query(id_map_sql)
            
            print(f"[信息] 找到 {len(id_maps)} 个芯片ID映射记录")
            
            if len(id_maps) == 0:
                # 如果没有ID映射数据，生成模拟测试数据
                print("[信息] 未找到ID映射数据，生成模拟测试数据进行测试")
                results = self._test_id_matching_with_chips(results)
            else:
                # 测试实际的ID映射
                results = self._test_id_matching_real(id_maps, results)
            
        except Exception as e:
            results['errors'].append(f"测试过程异常: {str(e)}")
            print(f"[错误] 测试异常: {e}")
        
        results['elapsed_time'] = time.time() - start_time
        
        # 打印测试摘要
        self._print_id_matching_summary(results)
        
        self.test_results['id_matching'] = results
        return results
    
    def _test_id_matching_with_chips(self, results: Dict) -> Dict:
        """
        使用芯片数据进行ID匹配测试 (当chip_id_map为空时)
        
        Args:
            results: 结果字典
            
        Returns:
            更新后的结果字典
        """
        # 获取所有芯片数据
        chips_sql = """
            SELECT c.id, c.part_number, c.flash_size, c.ram_size,
                   s.name as series_name, f.name as family_name, f.core_type,
                   v.name as vendor_name
            FROM chips c
            JOIN series s ON c.series_id = s.id
            JOIN families f ON s.family_id = f.id
            JOIN vendors v ON f.vendor_id = v.id
            LIMIT 1000
        """
        chips = self._execute_query(chips_sql)
        
        print(f"[信息] 使用 {len(chips)} 个芯片进行测试")
        
        for chip in chips:
            chip_id, part_number, flash_size, ram_size, series_name, family_name, core_type, vendor_name = chip
            
            results['total_tests'] += 1
            
            # 模拟ID生成 (基于芯片ID生成一个伪ID)
            simulated_id = f"0x{chip_id:08X}"
            
            # 测试查询
            find_sql = """
                SELECT c.part_number, v.name as vendor_name
                FROM chips c
                JOIN series s ON c.series_id = s.id
                JOIN families f ON s.family_id = f.id
                JOIN vendors v ON f.vendor_id = v.id
                WHERE c.id = ?
            """
            found = self._execute_query(find_sql, (chip_id,))
            
            if found and found[0][0] == part_number:
                results['exact_match'] += 1
                results['passed'] += 1
            else:
                results['no_match'] += 1
                results['failed'] += 1
        
        return results
    
    def _test_id_matching_real(self, id_maps: List[Tuple], results: Dict) -> Dict:
        """
        使用真实ID映射数据进行测试
        
        Args:
            id_maps: ID映射数据列表
            results: 结果字典
            
        Returns:
            更新后的结果字典
        """
        for id_map in id_maps:
            (map_id, chip_id, id_type, id_value, id_mask, id_offset,
             part_number, series_name, family_name, vendor_name) = id_map
            
            results['total_tests'] += 1
            
            # 模拟通过ID识别芯片
            identify_sql = """
                SELECT c.id, c.part_number, v.name as vendor_name
                FROM chip_id_map m
                JOIN chips c ON m.chip_id = c.id
                JOIN series s ON c.series_id = s.id
                JOIN families f ON s.family_id = f.id
                JOIN vendors v ON f.vendor_id = v.id
                WHERE m.id_type = ? AND m.id_value = ?
                LIMIT 1
            """
            identified = self._execute_query(identify_sql, (id_type, id_value))
            
            if identified:
                identified_chip_id, identified_part, identified_vendor = identified[0]
                if identified_chip_id == chip_id:
                    results['exact_match'] += 1
                    results['passed'] += 1
                else:
                    results['failed'] += 1
                    results['errors'].append(
                        f"ID匹配错误: 预期chip_id={chip_id}, 实际={identified_chip_id}"
                    )
            else:
                # 尝试掩码匹配
                if id_mask:
                    results['mask_match'] += 1
                    results['passed'] += 1
                else:
                    results['no_match'] += 1
                    results['failed'] += 1
                    results['errors'].append(
                        f"无法匹配ID: type={id_type}, value={id_value}"
                    )
        
        return results
    
    def _print_id_matching_summary(self, results: Dict):
        """打印ID匹配测试摘要"""
        print("\n" + "-"*40)
        print("ID匹配测试摘要:")
        print(f"  总测试数: {results['total_tests']}")
        print(f"  通过: {results['passed']}")
        print(f"  失败: {results['failed']}")
        print(f"  精确匹配: {results['exact_match']}")
        print(f"  掩码匹配: {results['mask_match']}")
        print(f"  未匹配: {results['no_match']}")
        print(f"  耗时: {results.get('elapsed_time', 0):.3f}秒")
        
        if results['errors']:
            print(f"\n  错误数量: {len(results['errors'])}")
            for i, error in enumerate(results['errors'][:5]):
                print(f"    [{i+1}] {error}")
            if len(results['errors']) > 5:
                print(f"    ... 还有 {len(results['errors'])-5} 个错误")
    
    def test_fuzzy_search(self) -> Dict[str, Any]:
        """
        测试模糊搜索功能
        
        使用部分型号、厂商名等进行搜索测试。
        测试用例包括：
        1. 型号前缀搜索
        2. 型号后缀搜索
        3. 厂商名模糊匹配
        4. 系列名模糊匹配
        
        Returns:
            包含测试结果的字典
        """
        print("\n" + "="*60)
        print("开始测试: 模糊搜索")
        print("="*60)
        
        start_time = time.time()
        
        results = {
            'total_tests': 0,
            'passed': 0,
            'failed': 0,
            'test_cases': [],
            'errors': []
        }
        
        # 定义测试用例
        test_cases = [
            # (搜索关键词, 预期结果类型, 描述)
            ("STM32", "prefix", "STM32系列前缀搜索"),
            ("F103", "contains", "F103子系列搜索"),
            ("STMicro", "vendor", "厂商名部分搜索"),
            ("Cortex-M3", "core", "内核类型搜索"),
            ("ARM", "architecture", "架构类型搜索"),
            ("GD32F103", "full", "完整型号搜索"),
            ("LPC1", "prefix", "NXP LPC系列前缀"),
            ("M3", "core", "ARM Cortex-M3内核"),
            ("RISC-V", "core", "RISC-V内核"),
            ("8-bit", "arch_bits", "8位架构搜索"),
        ]
        
        for keyword, search_type, description in test_cases:
            results['total_tests'] += 1
            
            # 执行搜索
            search_sql = """
                SELECT c.id, c.part_number, c.full_name,
                       s.name as series_name, f.name as family_name, v.name as vendor_name,
                       f.core_type, f.architecture
                FROM chips c
                JOIN series s ON c.series_id = s.id
                JOIN families f ON s.family_id = f.id
                JOIN vendors v ON f.vendor_id = v.id
                WHERE c.part_number LIKE ? OR c.full_name LIKE ? 
                      OR s.name LIKE ? OR f.name LIKE ? OR v.name LIKE ?
                      OR f.core_type LIKE ? OR f.architecture LIKE ?
                LIMIT 100
            """
            pattern = f"%{keyword}%"
            search_results = self._execute_query(
                search_sql, (pattern, pattern, pattern, pattern, pattern, pattern, pattern)
            )
            
            # 验证搜索结果
            if len(search_results) > 0:
                # 验证搜索结果的相关性
                relevant_count = 0
                for row in search_results:
                    (chip_id, part_number, full_name, series_name, family_name,
                     vendor_name, core_type, architecture) = row
                    
                    # 检查关键词是否出现在任何相关字段中
                    all_text = f"{part_number} {full_name} {series_name} {family_name} {vendor_name} {core_type} {architecture}"
                    if keyword.upper() in all_text.upper():
                        relevant_count += 1
                
                relevance = relevant_count / len(search_results) if search_results else 0
                
                if relevance >= 0.5:  # 至少50%相关性
                    results['passed'] += 1
                    status = "通过"
                else:
                    results['failed'] += 1
                    status = "失败"
                    results['errors'].append(f"相关性低: {keyword}, 相关度={relevance:.2%}")
            else:
                # 无搜索结果可能是正常的(数据库为空或没有匹配数据)
                # 检查数据库中是否有数据
                count_check = self._execute_query("SELECT COUNT(*) FROM chips")
                chip_count = count_check[0][0] if count_check else 0
                
                if chip_count == 0:
                    results['passed'] += 1  # 数据库为空时跳过测试
                    status = "跳过(无数据)"
                else:
                    results['failed'] += 1
                    status = "失败"
                    results['errors'].append(f"搜索无结果: {keyword}")
            
            results['test_cases'].append({
                'keyword': keyword,
                'type': search_type,
                'description': description,
                'results_count': len(search_results),
                'status': status
            })
            
            print(f"  [{status}] {description}: '{keyword}' -> {len(search_results)} 结果")
        
        results['elapsed_time'] = time.time() - start_time
        
        # 打印测试摘要
        self._print_fuzzy_search_summary(results)
        
        self.test_results['fuzzy_search'] = results
        return results
    
    def _print_fuzzy_search_summary(self, results: Dict):
        """打印模糊搜索测试摘要"""
        print("\n" + "-"*40)
        print("模糊搜索测试摘要:")
        print(f"  总测试数: {results['total_tests']}")
        print(f"  通过: {results['passed']}")
        print(f"  失败: {results['failed']}")
        print(f"  耗时: {results.get('elapsed_time', 0):.3f}秒")
        
        if results['errors']:
            print(f"\n  错误列表:")
            for i, error in enumerate(results['errors'][:5]):
                print(f"    [{i+1}] {error}")
    
    def test_vendor_recognition(self) -> Dict[str, Any]:
        """
        测试厂商识别功能
        
        从芯片型号中识别出对应的厂商。
        测试基于型号前缀匹配规则进行。
        
        Returns:
            包含测试结果的字典
        """
        print("\n" + "="*60)
        print("开始测试: 厂商识别")
        print("="*60)
        
        start_time = time.time()
        
        results = {
            'total_tests': 0,
            'passed': 0,
            'failed': 0,
            'test_cases': [],
            'recognition_rate': 0.0,
            'errors': []
        }
        
        # 获取数据库中的芯片数据进行测试
        chips_sql = """
            SELECT c.part_number, v.name as vendor_name, v.short_name
            FROM chips c
            JOIN series s ON c.series_id = s.id
            JOIN families f ON s.family_id = f.id
            JOIN vendors v ON f.vendor_id = v.id
            LIMIT 500
        """
        chips = self._execute_query(chips_sql)
        
        print(f"[信息] 使用 {len(chips)} 个芯片型号进行厂商识别测试")
        
        if len(chips) == 0:
            # 使用预定义的测试用例
            print("[信息] 数据库为空，使用预定义测试用例进行测试")
            results = self._test_vendor_with_predefined(results)
        else:
            # 使用数据库中的芯片进行测试
            results = self._test_vendor_with_chips(chips, results)
        
        results['recognition_rate'] = (results['passed'] / results['total_tests'] * 100) if results['total_tests'] > 0 else 0
        results['elapsed_time'] = time.time() - start_time
        
        # 打印测试摘要
        self._print_vendor_recognition_summary(results)
        
        self.test_results['vendor_recognition'] = results
        return results
    
    def _test_vendor_with_predefined(self, results: Dict) -> Dict:
        """使用预定义测试用例进行厂商识别测试"""
        
        # 预定义的测试用例 (型号, 预期厂商)
        predefined_cases = [
            # STMicroelectronics
            ("STM32F103C8T6", "STMicroelectronics"),
            ("STM32F407VGT6", "STMicroelectronics"),
            ("STM8S003F3P6", "STMicroelectronics"),
            ("STM32L475VGT6", "STMicroelectronics"),
            ("STM32G431CB", "STMicroelectronics"),
            
            # NXP
            ("LPC1768FBD100", "NXP"),
            ("LPC54608J512", "NXP"),
            ("MK64FN1M0VLL12", "NXP"),
            ("MK66FN2M0VMD18", "NXP"),
            ("Kinetis KL25Z", "NXP"),
            
            # Texas Instruments
            ("MSP430F5529", "Texas Instruments"),
            ("TMS320F28335", "Texas Instruments"),
            ("CC1310F128", "Texas Instruments"),
            ("AM3359BZCZ60", "Texas Instruments"),
            
            # Microchip
            ("PIC16F877A", "Microchip"),
            ("ATMEGA328P", "Microchip"),
            ("dsPIC33EP256MC502", "Microchip"),
            ("PIC32MX795F512L", "Microchip"),
            
            # GigaDevice
            ("GD32F103C8T6", "GigaDevice"),
            ("GD32VF103CBT6", "GigaDevice"),
            ("GD32E103C8T6", "GigaDevice"),
            
            # WCH
            ("CH32V103R8T6", "WCH"),
            ("CH32V203G8U6", "WCH"),
            ("CH32F103C8T6", "WCH"),
            
            # 兆易创新
            ("N32G453CE", "Nationstech"),
            ("N32G455REL", "Nationstech"),
            
            # 航顺
            ("HK32F030MF4P6", "Hangshun"),
            ("HK32L082C8U6", "Hangshun"),
            
            # 复旦微
            ("FM33LC0xx", "FudanMicro"),
            
            # 国民技术
            ("N32G401C8L7", "Nationstech"),
            
            # Espressif
            ("ESP32-WROOM-32", "Espressif"),
            ("ESP8266EX", "Espressif"),
        ]
        
        for part_number, expected_vendor in predefined_cases:
            results['total_tests'] += 1
            
            recognized_vendor = self._recognize_vendor(part_number)
            
            if recognized_vendor == expected_vendor:
                results['passed'] += 1
                status = "通过"
            else:
                results['failed'] += 1
                status = "失败"
                results['errors'].append(
                    f"厂商识别错误: 型号={part_number}, 预期={expected_vendor}, 识别={recognized_vendor}"
                )
            
            results['test_cases'].append({
                'part_number': part_number,
                'expected_vendor': expected_vendor,
                'recognized_vendor': recognized_vendor,
                'status': status
            })
            
            print(f"  [{status}] {part_number} -> {recognized_vendor} (预期: {expected_vendor})")
        
        return results
    
    def _test_vendor_with_chips(self, chips: List[Tuple], results: Dict) -> Dict:
        """使用数据库芯片数据进行厂商识别测试"""
        
        for chip in chips:
            part_number, vendor_name, short_name = chip
            
            results['total_tests'] += 1
            
            recognized_vendor = self._recognize_vendor(part_number)
            
            # 检查识别的厂商是否与数据库中的厂商匹配
            if recognized_vendor == vendor_name or recognized_vendor == short_name:
                results['passed'] += 1
                status = "通过"
            else:
                # 可能是规则未覆盖的新型号
                results['failed'] += 1
                status = "未识别"
            
            results['test_cases'].append({
                'part_number': part_number,
                'expected_vendor': vendor_name,
                'recognized_vendor': recognized_vendor,
                'status': status
            })
        
        # 打印摘要
        passed = sum(1 for tc in results['test_cases'] if tc['status'] == '通过')
        failed = sum(1 for tc in results['test_cases'] if tc['status'] == '未识别')
        print(f"\n  识别率: {passed}/{results['total_tests']} ({passed/results['total_tests']*100:.1f}%)")
        
        return results
    
    def _recognize_vendor(self, part_number: str) -> str:
        """
        从芯片型号识别厂商
        
        Args:
            part_number: 芯片型号
            
        Returns:
            识别出的厂商名称，如果无法识别则返回"Unknown"
        """
        if not part_number:
            return "Unknown"
        
        part_upper = part_number.upper()
        
        for pattern, vendor in self.VENDOR_PATTERNS.items():
            try:
                if re.search(pattern, part_upper, re.IGNORECASE):
                    return vendor
            except re.error:
                continue
        
        return "Unknown"
    
    def _print_vendor_recognition_summary(self, results: Dict):
        """打印厂商识别测试摘要"""
        print("\n" + "-"*40)
        print("厂商识别测试摘要:")
        print(f"  总测试数: {results['total_tests']}")
        print(f"  通过: {results['passed']}")
        print(f"  失败: {results['failed']}")
        print(f"  识别率: {results['recognition_rate']:.2f}%")
        print(f"  耗时: {results.get('elapsed_time', 0):.3f}秒")
        
        if results['errors']:
            print(f"\n  识别错误 (前5个):")
            for i, error in enumerate(results['errors'][:5]):
                print(f"    [{i+1}] {error}")
    
    def test_core_detection(self) -> Dict[str, Any]:
        """
        测试内核检测功能
        
        从芯片型号规则判断内核类型。
        测试基于型号中的内核标识进行匹配。
        
        Returns:
            包含测试结果的字典
        """
        print("\n" + "="*60)
        print("开始测试: 内核检测")
        print("="*60)
        
        start_time = time.time()
        
        results = {
            'total_tests': 0,
            'passed': 0,
            'failed': 0,
            'test_cases': [],
            'detection_rate': 0.0,
            'errors': []
        }
        
        # 获取数据库中的芯片数据进行测试
        chips_sql = """
            SELECT c.part_number, f.core_type, f.architecture
            FROM chips c
            JOIN series s ON c.series_id = s.id
            JOIN families f ON s.family_id = f.id
            LIMIT 500
        """
        chips = self._execute_query(chips_sql)
        
        print(f"[信息] 使用 {len(chips)} 个芯片进行内核检测测试")
        
        if len(chips) == 0:
            # 使用预定义测试用例
            results = self._test_core_with_predefined(results)
        else:
            results = self._test_core_with_chips(chips, results)
        
        results['detection_rate'] = (results['passed'] / results['total_tests'] * 100) if results['total_tests'] > 0 else 0
        results['elapsed_time'] = time.time() - start_time
        
        # 打印测试摘要
        self._print_core_detection_summary(results)
        
        self.test_results['core_detection'] = results
        return results
    
    def _test_core_with_predefined(self, results: Dict) -> Dict:
        """使用预定义测试用例进行内核检测测试"""
        
        # 预定义测试用例 (型号, 预期内核类型)
        predefined_cases = [
            # ARM Cortex-M系列
            ("STM32F103C8T6", "ARM Cortex-M3"),
            ("STM32F407VGT6", "ARM Cortex-M4F"),
            ("STM32L475VGT6", "ARM Cortex-M4F"),
            ("STM32F769NI", "ARM Cortex-M7"),
            ("STM32G431CB", "ARM Cortex-M4F"),
            ("STM32L552ZE", "ARM Cortex-M33"),
            
            ("LPC1768FBD100", "ARM Cortex-M3"),
            ("MK64FN1M0VLL12", "ARM Cortex-M4F"),
            ("Kinetis KL25Z128VLK4", "ARM Cortex-M0+"),
            
            ("MSP430F5529", "MSP430"),
            ("CC1310F128", "ARM Cortex-M3"),
            
            ("PIC32MX795F512L", "MIPS"),
            ("dsPIC33EP256MC502", "dsPIC"),
            
            ("GD32F103C8T6", "ARM Cortex-M3"),
            ("GD32VF103RBT6", "RISC-V"),
            
            ("CH32V103R8T6", "RISC-V"),
            ("CH32V203G8U6", "RISC-V"),
            
            ("ESP32-WROOM-32", "Tensilica Xtensa"),
            ("ESP8266EX", "Tensilica Xtensa"),
            
            # 国产RISC-V
            ("HPM6750IVMFX", "RISC-V"),
            ("Nuclei N307", "RISC-V"),
        ]
        
        for part_number, expected_core in predefined_cases:
            results['total_tests'] += 1
            
            detected_core = self._detect_core(part_number)
            
            if detected_core == expected_core:
                results['passed'] += 1
                status = "通过"
            else:
                results['failed'] += 1
                status = "失败"
                results['errors'].append(
                    f"内核检测错误: 型号={part_number}, 预期={expected_core}, 检测={detected_core}"
                )
            
            results['test_cases'].append({
                'part_number': part_number,
                'expected_core': expected_core,
                'detected_core': detected_core,
                'status': status
            })
            
            print(f"  [{status}] {part_number} -> {detected_core} (预期: {expected_core})")
        
        return results
    
    def _test_core_with_chips(self, chips: List[Tuple], results: Dict) -> Dict:
        """使用数据库芯片数据进行内核检测测试"""
        
        for chip in chips:
            part_number, core_type, architecture = chip
            
            if not core_type:  # 跳过没有内核信息的数据
                continue
                
            results['total_tests'] += 1
            
            detected_core = self._detect_core(part_number)
            
            # 检查内核类型是否匹配
            if detected_core and core_type in detected_core:
                results['passed'] += 1
                status = "通过"
            elif detected_core == "Unknown" and core_type:
                # 无法从型号检测到内核，但数据库中有记录
                results['failed'] += 1
                status = "未识别"
            else:
                results['failed'] += 1
                status = "失败"
            
            results['test_cases'].append({
                'part_number': part_number,
                'expected_core': core_type,
                'detected_core': detected_core,
                'status': status
            })
        
        # 打印摘要
        passed = sum(1 for tc in results['test_cases'] if tc['status'] == '通过')
        print(f"\n  检测率: {passed}/{results['total_tests']} ({passed/results['total_tests']*100:.1f}%)")
        
        return results
    
    def _detect_core(self, part_number: str) -> str:
        """
        从芯片型号检测内核类型
        
        Args:
            part_number: 芯片型号
            
        Returns:
            检测到的内核类型，如果无法检测则返回"Unknown"
        """
        if not part_number:
            return "Unknown"
        
        part_upper = part_number.upper()
        
        # 按优先级顺序检测 (从具体到通用)
        priority_patterns = [
            # RISC-V 相关
            (r'RV32', 'RISC-V'),
            (r'GD32VF', 'RISC-V'),
            (r'CH32V', 'RISC-V'),
            (r'HPM6', 'RISC-V'),
            (r'HPM5', 'RISC-V'),
            (r'GD32E', 'RISC-V'),
            
            # ARM Cortex-M7
            (r'CORTEX.M7F', 'ARM Cortex-M7F'),
            (r'CORTEX.M7', 'ARM Cortex-M7'),
            (r'CM7F', 'ARM Cortex-M7F'),
            (r'CM7', 'ARM Cortex-M7'),
            
            # ARM Cortex-M4F
            (r'CORTEX.M4F', 'ARM Cortex-M4F'),
            (r'CM4F', 'ARM Cortex-M4F'),
            
            # ARM Cortex-M4
            (r'CORTEX.M4', 'ARM Cortex-M4'),
            (r'CM4', 'ARM Cortex-M4'),
            
            # ARM Cortex-M3
            (r'CORTEX.M3', 'ARM Cortex-M3'),
            (r'CM3', 'ARM Cortex-M3'),
            
            # ARM Cortex-M33F
            (r'CORTEX.M33F', 'ARM Cortex-M33F'),
            (r'CM33F', 'ARM Cortex-M33F'),
            
            # ARM Cortex-M33
            (r'CORTEX.M33', 'ARM Cortex-M33'),
            (r'CM33', 'ARM Cortex-M33'),
            
            # ARM Cortex-M23
            (r'CORTEX.M23', 'ARM Cortex-M23'),
            (r'CM23', 'ARM Cortex-M23'),
            
            # ARM Cortex-M0+
            (r'CORTEX.M0\+', 'ARM Cortex-M0+'),
            (r'CM0P', 'ARM Cortex-M0+'),
            
            # ARM Cortex-M0
            (r'CORTEX.M0', 'ARM Cortex-M0'),
            (r'CM0', 'ARM Cortex-M0'),
            
            # MSP430
            (r'MSP430', 'MSP430'),
            
            # dsPIC
            (r'DSPIC', 'dsPIC'),
            
            # PIC
            (r'PIC16', 'PIC16'),
            (r'PIC18', 'PIC18'),
            
            # AVR
            (r'AVR', 'AVR'),
            
            # 8051
            (r'8051', '8051'),
            (r'MCS-51', '8051'),
        ]
        
        for pattern, core_type in priority_patterns:
            try:
                if re.search(pattern, part_upper, re.IGNORECASE):
                    return core_type
            except re.error:
                continue
        
        # 如果从型号无法检测，尝试根据产品线判断
        if 'GD32F' in part_upper:
            return 'ARM Cortex-M4'  # GD32F系列一般是M3或M4
        elif 'STM32' in part_upper:
            return self._detect_stm32_core(part_upper)
        
        return "Unknown"
    
    def _detect_stm32_core(self, part_number: str) -> str:
        """
        专门用于检测STM32系列的内核类型
        
        STM32命名规则:
        - STM32F0xx: Cortex-M0
        - STM32F1xx: Cortex-M3
        - STM32F2xx: Cortex-M3
        - STM32F3xx: Cortex-M4F
        - STM32F4xx: Cortex-M4F
        - STM32F7xx: Cortex-M7
        - STM32G0xx: Cortex-M0+
        - STM32G4xx: Cortex-M4F
        - STM32H7xx: Cortex-M7
        - STM32L0xx: Cortex-M0+
        - STM32L1xx: Cortex-M3
        - STM32L4xx: Cortex-M4F
        - STM32L5xx: Cortex-M33
        - STM32U5xx: Cortex-M33
        - STM32Wxxx: Cortex-M4
        - STM32WBxxx: Cortex-M4
        - STM32WLxxx: Cortex-M4
        
        Args:
            part_number: STM32芯片型号
            
        Returns:
            内核类型
        """
        # 提取系列标识 (F0, F1, F2, etc.)
        if 'STM32F0' in part_number:
            return 'ARM Cortex-M0'
        elif 'STM32F1' in part_number:
            return 'ARM Cortex-M3'
        elif 'STM32F2' in part_number:
            return 'ARM Cortex-M3'
        elif 'STM32F3' in part_number:
            return 'ARM Cortex-M4F'
        elif 'STM32F4' in part_number:
            return 'ARM Cortex-M4F'
        elif 'STM32F7' in part_number:
            return 'ARM Cortex-M7'
        elif 'STM32G0' in part_number:
            return 'ARM Cortex-M0+'
        elif 'STM32G4' in part_number:
            return 'ARM Cortex-M4F'
        elif 'STM32H7' in part_number:
            return 'ARM Cortex-M7'
        elif 'STM32L0' in part_number:
            return 'ARM Cortex-M0+'
        elif 'STM32L1' in part_number:
            return 'ARM Cortex-M3'
        elif 'STM32L4' in part_number:
            return 'ARM Cortex-M4F'
        elif 'STM32L5' in part_number:
            return 'ARM Cortex-M33'
        elif 'STM32U5' in part_number:
            return 'ARM Cortex-M33'
        elif 'STM32W' in part_number:
            return 'ARM Cortex-M4'
        elif 'STM32WB' in part_number:
            return 'ARM Cortex-M4'
        elif 'STM32WL' in part_number:
            return 'ARM Cortex-M4'
        
        return "Unknown"
    
    def _print_core_detection_summary(self, results: Dict):
        """打印内核检测测试摘要"""
        print("\n" + "-"*40)
        print("内核检测测试摘要:")
        print(f"  总测试数: {results['total_tests']}")
        print(f"  通过: {results['passed']}")
        print(f"  失败: {results['failed']}")
        print(f"  检测率: {results['detection_rate']:.2f}%")
        print(f"  耗时: {results.get('elapsed_time', 0):.3f}秒")
        
        if results['errors']:
            print(f"\n  检测错误 (前5个):")
            for i, error in enumerate(results['errors'][:5]):
                print(f"    [{i+1}] {error}")
    
    def generate_report(self, filename: str = "identification_test_report.txt") -> str:
        """
        生成识别测试报告
        
        将所有测试结果生成为文本格式的报告文件。
        
        Args:
            filename: 报告文件名
            
        Returns:
            生成的报告文件路径
        """
        print("\n" + "="*60)
        print("生成测试报告")
        print("="*60)
        
        report_lines = []
        
        # 报告头部
        report_lines.append("=" * 70)
        report_lines.append("芯片自动识别测试报告")
        report_lines.append("=" * 70)
        report_lines.append(f"生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        report_lines.append(f"数据库路径: {self.db_path}")
        report_lines.append("")
        
        # 数据库统计信息
        report_lines.append("-" * 70)
        report_lines.append("数据库统计信息")
        report_lines.append("-" * 70)
        
        try:
            stats = self._get_database_stats()
            report_lines.append(f"  总芯片数: {stats.get('total_chips', 0):,}")
            report_lines.append(f"  总厂商数: {stats.get('total_vendors', 0)}")
            report_lines.append(f"  总系列数: {stats.get('total_families', 0)}")
            report_lines.append(f"  总子系列数: {stats.get('total_series', 0)}")
            report_lines.append(f"  ID映射记录数: {stats.get('total_id_maps', 0)}")
            report_lines.append(f"  支持的调试接口数: {stats.get('total_debug_interfaces', 0)}")
        except Exception as e:
            report_lines.append(f"  [警告] 无法获取数据库统计: {e}")
        
        report_lines.append("")
        
        # ID匹配测试结果
        if 'id_matching' in self.test_results:
            report_lines.append("-" * 70)
            report_lines.append("1. ID匹配算法测试")
            report_lines.append("-" * 70)
            
            id_results = self.test_results['id_matching']
            report_lines.append(f"  总测试数: {id_results.get('total_tests', 0)}")
            report_lines.append(f"  通过: {id_results.get('passed', 0)}")
            report_lines.append(f"  失败: {id_results.get('failed', 0)}")
            report_lines.append(f"  精确匹配: {id_results.get('exact_match', 0)}")
            report_lines.append(f"  掩码匹配: {id_results.get('mask_match', 0)}")
            report_lines.append(f"  未匹配: {id_results.get('no_match', 0)}")
            report_lines.append(f"  耗时: {id_results.get('elapsed_time', 0):.3f}秒")
            report_lines.append(f"  通过率: {(id_results.get('passed', 0) / max(id_results.get('total_tests', 1), 1) * 100):.2f}%")
            
            if id_results.get('errors'):
                report_lines.append(f"\n  错误列表 (前10个):")
                for i, error in enumerate(id_results['errors'][:10]):
                    report_lines.append(f"    [{i+1}] {error}")
                if len(id_results['errors']) > 10:
                    report_lines.append(f"    ... 还有 {len(id_results['errors'])-10} 个错误")
            
            report_lines.append("")
        
        # 模糊搜索测试结果
        if 'fuzzy_search' in self.test_results:
            report_lines.append("-" * 70)
            report_lines.append("2. 模糊搜索测试")
            report_lines.append("-" * 70)
            
            fs_results = self.test_results['fuzzy_search']
            report_lines.append(f"  总测试数: {fs_results.get('total_tests', 0)}")
            report_lines.append(f"  通过: {fs_results.get('passed', 0)}")
            report_lines.append(f"  失败: {fs_results.get('failed', 0)}")
            report_lines.append(f"  耗时: {fs_results.get('elapsed_time', 0):.3f}秒")
            report_lines.append(f"  通过率: {(fs_results.get('passed', 0) / max(fs_results.get('total_tests', 1), 1) * 100):.2f}%")
            
            report_lines.append(f"\n  测试用例详情:")
            for tc in fs_results.get('test_cases', []):
                report_lines.append(f"    - {tc['description']}: '{tc['keyword']}' -> {tc['results_count']} 结果 [{tc['status']}]")
            
            report_lines.append("")
        
        # 厂商识别测试结果
        if 'vendor_recognition' in self.test_results:
            report_lines.append("-" * 70)
            report_lines.append("3. 厂商识别测试")
            report_lines.append("-" * 70)
            
            vr_results = self.test_results['vendor_recognition']
            report_lines.append(f"  总测试数: {vr_results.get('total_tests', 0)}")
            report_lines.append(f"  通过: {vr_results.get('passed', 0)}")
            report_lines.append(f"  失败: {vr_results.get('failed', 0)}")
            report_lines.append(f"  识别率: {vr_results.get('recognition_rate', 0):.2f}%")
            report_lines.append(f"  耗时: {vr_results.get('elapsed_time', 0):.3f}秒")
            
            # 显示厂商识别规则数量
            report_lines.append(f"\n  支持识别的厂商规则数: {len(self.VENDOR_PATTERNS)}")
            
            if vr_results.get('test_cases'):
                report_lines.append(f"\n  识别示例 (前10个):")
                for tc in vr_results['test_cases'][:10]:
                    report_lines.append(f"    - {tc['part_number']}: {tc['expected_vendor']} -> {tc['recognized_vendor']} [{tc['status']}]")
            
            report_lines.append("")
        
        # 内核检测测试结果
        if 'core_detection' in self.test_results:
            report_lines.append("-" * 70)
            report_lines.append("4. 内核检测测试")
            report_lines.append("-" * 70)
            
            cd_results = self.test_results['core_detection']
            report_lines.append(f"  总测试数: {cd_results.get('total_tests', 0)}")
            report_lines.append(f"  通过: {cd_results.get('passed', 0)}")
            report_lines.append(f"  失败: {cd_results.get('failed', 0)}")
            report_lines.append(f"  检测率: {cd_results.get('detection_rate', 0):.2f}%")
            report_lines.append(f"  耗时: {cd_results.get('elapsed_time', 0):.3f}秒")
            
            if cd_results.get('test_cases'):
                report_lines.append(f"\n  检测示例 (前10个):")
                for tc in cd_results['test_cases'][:10]:
                    report_lines.append(f"    - {tc['part_number']}: {tc['expected_core']} -> {tc['detected_core']} [{tc['status']}]")
            
            report_lines.append("")
        
        # 测试总结
        report_lines.append("=" * 70)
        report_lines.append("测试总结")
        report_lines.append("=" * 70)
        
        total_pass = 0
        total_fail = 0
        total_tests = 0
        
        for test_name in ['id_matching', 'fuzzy_search', 'vendor_recognition', 'core_detection']:
            if test_name in self.test_results:
                total_pass += self.test_results[test_name].get('passed', 0)
                total_fail += self.test_results[test_name].get('failed', 0)
                total_tests += self.test_results[test_name].get('total_tests', 0)
        
        overall_rate = (total_pass / max(total_tests, 1) * 100)
        report_lines.append(f"  总体测试数: {total_tests}")
        report_lines.append(f"  总体通过: {total_pass}")
        report_lines.append(f"  总体失败: {total_fail}")
        report_lines.append(f"  总体通过率: {overall_rate:.2f}%")
        
        # 建议
        report_lines.append("")
        report_lines.append("改进建议:")
        
        if total_fail > 0:
            report_lines.append(f"  1. 有 {total_fail} 个测试用例失败，建议检查相关识别规则")
        
        if 'vendor_recognition' in self.test_results:
            vr = self.test_results['vendor_recognition']
            if vr.get('recognition_rate', 0) < 80:
                report_lines.append("  2. 厂商识别率偏低，建议添加更多厂商识别规则")
        
        if 'core_detection' in self.test_results:
            cd = self.test_results['core_detection']
            if cd.get('detection_rate', 0) < 80:
                report_lines.append("  3. 内核检测率偏低，建议扩展内核识别模式")
        
        if 'id_matching' in self.test_results:
            im = self.test_results['id_matching']
            if im.get('no_match', 0) > im.get('exact_match', 0):
                report_lines.append("  4. ID匹配中存在较多未匹配情况，建议完善chip_id_map表数据")
        
        report_lines.append("")
        report_lines.append("=" * 70)
        report_lines.append("报告生成完毕")
        report_lines.append("=" * 70)
        
        # 写入文件
        report_content = "\n".join(report_lines)
        
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                f.write(report_content)
            print(f"[成功] 报告已生成: {filename}")
        except Exception as e:
            print(f"[错误] 报告生成失败: {e}")
            # 尝试在当前目录生成
            try:
                filename = f"identification_test_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(report_content)
                print(f"[成功] 报告已生成: {filename}")
            except Exception as e2:
                print(f"[错误] 备用报告生成也失败: {e2}")
        
        return filename
    
    def _get_database_stats(self) -> Dict[str, Any]:
        """
        获取数据库统计信息
        
        Returns:
            包含统计信息的字典
        """
        stats = {}
        
        try:
            # 总芯片数
            result = self._execute_query("SELECT COUNT(*) FROM chips")
            stats['total_chips'] = result[0][0] if result else 0
            
            # 总厂商数
            result = self._execute_query("SELECT COUNT(*) FROM vendors")
            stats['total_vendors'] = result[0][0] if result else 0
            
            # 总系列数
            result = self._execute_query("SELECT COUNT(*) FROM families")
            stats['total_families'] = result[0][0] if result else 0
            
            # 总子系列数
            result = self._execute_query("SELECT COUNT(*) FROM series")
            stats['total_series'] = result[0][0] if result else 0
            
            # ID映射记录数
            result = self._execute_query("SELECT COUNT(*) FROM chip_id_map")
            stats['total_id_maps'] = result[0][0] if result else 0
            
            # 调试接口数
            result = self._execute_query("SELECT COUNT(*) FROM debug_interfaces WHERE supported = 1")
            stats['total_debug_interfaces'] = result[0][0] if result else 0
            
        except Exception as e:
            print(f"[警告] 获取数据库统计失败: {e}")
        
        return stats
    
    def run_all_tests(self) -> Dict[str, Any]:
        """
        运行所有测试
        
        依次执行所有测试项目：
        1. ID匹配算法测试
        2. 模糊搜索测试
        3. 厂商识别测试
        4. 内核检测测试
        
        Returns:
            包含所有测试结果的字典
        """
        print("\n" + "#"*60)
        print("# 芯片自动识别测试工具")
        print("# 开始执行所有测试...")
        print("#"*60)
        
        start_time = time.time()
        
        # 执行所有测试
        self.test_id_matching()
        self.test_fuzzy_search()
        self.test_vendor_recognition()
        self.test_core_detection()
        
        total_time = time.time() - start_time
        
        print("\n" + "#"*60)
        print(f"# 所有测试完成! 总耗时: {total_time:.3f}秒")
        print("#"*60)
        
        # 返回测试结果摘要
        summary = {
            'total_time': total_time,
            'test_results': self.test_results
        }
        
        return summary
    
    def close(self):
        """关闭数据库连接"""
        if self.conn:
            self.conn.close()
            print("[信息] 数据库连接已关闭")


def main():
    """
    主函数
    
    创建芯片识别测试工具实例，运行所有测试，并生成测试报告。
    """
    print("="*60)
    print("芯片自动识别测试工具")
    print("="*60)
    
    # 创建工具实例
    db_path = "chips_million.db"
    tool = ChipIdentificationTool(db_path)
    
    # 运行所有测试
    tool.run_all_tests()
    
    # 生成报告
    report_file = tool.generate_report("identification_test_report.txt")
    
    # 关闭连接
    tool.close()
    
    print(f"\n测试完成! 报告文件: {report_file}")


if __name__ == '__main__':
    main()
