# 多功能编程器PC端

这是多功能编程器的PC端程序，通过USB CDC与编程器通信，支持芯片选择、快速搜索、Flash读写擦等功能。

## 功能特性

- 📦 **芯片数据库** - 内置多种常见芯片信息，支持手动添加
- 🔍 **快速搜索** - 支持按芯片名称、厂商、系列等关键词搜索
- 💾 **Flash操作** - 支持读取、写入、擦除Flash
- 🔌 **USB CDC通信** - 通过虚拟串口与编程器通信
- 📊 **实时日志** - 显示详细操作日志

## 支持的芯片

- NXP S32K1/S32K3系列
- ST STM32系列
- 更多芯片持续添加中...

## 安装说明

### 环境要求

- Python 3.7+
- Windows/Linux/macOS

### 安装步骤

1. 安装依赖：
```bash
pip install -r requirements.txt
```

2. 运行程序：
```bash
python main.py
```

## 使用说明

### 1. 连接编程器

- 用USB线连接编程器
- 在程序中选择对应的串口号
- 点击"连接"按钮

### 2. 选择芯片

- 在搜索框中输入芯片型号或关键词进行搜索
- 在芯片列表中选择对应的芯片
- 查看右侧的芯片信息

### 3. Flash操作

- 选择要操作的文件（.bin或.hex）
- 设置起始地址和大小
- 点击相应的操作按钮（读取/写入/擦除/全片擦除）

## 文件说明

- `main.py` - 主程序入口，包含GUI界面
- `chip_database.py` - 芯片数据库和搜索功能
- `serial_comm.py` - USB CDC串口通信
- `requirements.txt` - Python依赖列表
- `chip_database.json` - 芯片数据库（程序运行时自动生成）

## 开发说明

### 添加新芯片

编辑 `chip_database.py` 中的 `_init_default_database` 方法，添加新的芯片条目：

```python
{
    "name": "芯片型号",
    "vendor": "厂商名",
    "family": "系列名",
    "core": "内核类型",
    "flash_size": Flash大小(字节),
    "ram_size": RAM大小(字节),
    "package": "封装",
    "debug_interfaces": ["SWD", "JTAG"],
    "status": "supported"
}
```

### 通信协议

PC端与编程器通过自定义协议通信，详见 `serial_comm.py`。

## 项目结构

```
pc/
├── main.py           # 主程序
├── chip_database.py  # 芯片数据库
├── serial_comm.py    # 串口通信
├── requirements.txt  # 依赖列表
└── README.md         # 本文档
```

## 许可证

本项目仅供学习和研究使用。
