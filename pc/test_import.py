#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""测试导入的芯片数据"""

from chip_database_sqlite import MillionChipDatabase

# 打开数据库
db = MillionChipDatabase('chips_million.db')

# 搜索STM32F103系列
print("=" * 60)
print("搜索 STM32F103 系列芯片:")
print("=" * 60)
results = db.search_chips('STM32F103', limit=10)
for r in results:
    print(f"{r['part_number']:20s} - {r['vendor']:15s} {r['family']:10s} {r['series']:10s} - Flash:{r['flash_size']:4d}KB RAM:{r['ram_size']:3d}KB")

print()

# 搜索GD32系列
print("=" * 60)
print("搜索 GD32 系列芯片:")
print("=" * 60)
results = db.search_chips('GD32', limit=10)
for r in results:
    print(f"{r['part_number']:20s} - {r['vendor']:15s} {r['family']:10s} {r['series']:10s} - Flash:{r['flash_size']:4d}KB RAM:{r['ram_size']:3d}KB")

print()

# 搜索ESP32系列
print("=" * 60)
print("搜索 ESP32 系列芯片:")
print("=" * 60)
results = db.search_chips('ESP32', limit=10)
for r in results:
    print(f"{r['part_number']:20s} - {r['vendor']:15s} {r['family']:10s} {r['series']:10s} - Flash:{r['flash_size']:4d}KB RAM:{r['ram_size']:3d}KB")

print()

# 获取统计信息
print("=" * 60)
print("数据库统计:")
print("=" * 60)
stats = db.get_statistics()
print(f"总芯片数: {stats['total_chips']}")
print(f"厂商数: {stats['total_vendors']}")
print(f"系列数: {stats['total_families']}")
print(f"子系列数: {stats['total_series']}")

# 关闭数据库
db.close()
