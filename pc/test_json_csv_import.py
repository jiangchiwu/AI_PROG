#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""测试JSON和CSV导入功能"""

from chip_database_sqlite import MillionChipDatabase
from chip_data_importer import ChipDataImporter

# 打开数据库
db = MillionChipDatabase('chips_million.db')

# 创建导入工具
importer = ChipDataImporter(db)

print("=" * 60)
print("测试JSON导入功能")
print("=" * 60)
count = importer.import_from_json('test_chips.json')
print(f"JSON导入完成: {count} 款芯片")

print()
print("=" * 60)
print("测试CSV导入功能")
print("=" * 60)
count = importer.import_from_csv('test_chips.csv')
print(f"CSV导入完成: {count} 款芯片")

print()
print("=" * 60)
print("验证导入结果")
print("=" * 60)

# 搜索测试芯片
results = db.search_chips('TEST', limit=10)
print(f"\n找到 {len(results)} 款测试芯片:")
for r in results:
    print(f"  {r['part_number']:20s} - {r['vendor']:15s} {r['family']:15s} - Flash:{r['flash_size']:4d}KB RAM:{r['ram_size']:3d}KB")

# 生成导入报告
print()
print(importer.generate_import_report())

# 关闭数据库
db.close()
