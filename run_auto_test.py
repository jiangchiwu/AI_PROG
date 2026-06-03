
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from dslogic_automation_simple import DSLogicAutomation

def run_automated_test():
    """自动运行测试，无需人工参与"""
    print("=" * 60)
    print("    自动运行DSlogic闭环测试验证")
    print("=" * 60)
    
    # 创建自动化实例
    automation = DSLogicAutomation()
    
    # 运行5次循环测试
    print("\n开始自动测试...")
    result = automation.run_cycle_test(cycles=5)
    
    # 输出最终结果
    print("\n" + "=" * 60)
    print("           测试结果摘要")
    print("=" * 60)
    print(f"测试次数: {result['total']}")
    print(f"通过率: {result['passed'] / result['total'] * 100:.1f}%")
    print("=" * 60)
    
    return result['passed'] / result['total'] * 100

if __name__ == "__main__":
    run_automated_test()
