"""
多功能编程器PC端主程序
支持USB CDC通信、芯片选择、快速搜索、Flash读写擦等功能
"""
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import threading
import os
from typing import Optional, Dict, Any

from chip_database import ChipDatabase, format_size
from serial_comm import SerialCommunicator


class ProgrammerApp:
    """编程器应用主类"""

    def __init__(self, root):
        self.root = root
        self.root.title("多功能编程器")
        self.root.geometry("1200x800")

        # 初始化组件
        self.db = ChipDatabase()
        self.comm = SerialCommunicator()
        self.selected_chip: Optional[Dict[str, Any]] = None
        self.connected_port: Optional[str] = None

        # 创建UI
        self._create_ui()

        # 刷新端口列表
        self._refresh_ports()

    def _create_ui(self):
        """创建用户界面"""
        # 主框架
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 配置行列权重
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        main_frame.rowconfigure(3, weight=1)

        # 1. 连接区域
        conn_frame = ttk.LabelFrame(main_frame, text="连接设置", padding="10")
        conn_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        conn_frame.columnconfigure(1, weight=1)

        ttk.Label(conn_frame, text="串口:").grid(row=0, column=0, sticky=tk.W)
        self.port_combo = ttk.Combobox(conn_frame, width=30)
        self.port_combo.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(10, 5))

        ttk.Button(conn_frame, text="刷新", command=self._refresh_ports).grid(row=0, column=2, padx=(0, 5))
        self.connect_btn = ttk.Button(conn_frame, text="连接", command=self._toggle_connection)
        self.connect_btn.grid(row=0, column=3)

        self.status_label = ttk.Label(conn_frame, text="未连接", foreground="red")
        self.status_label.grid(row=0, column=4, padx=(10, 0))

        # 2. 芯片选择区域
        chip_frame = ttk.LabelFrame(main_frame, text="芯片选择", padding="10")
        chip_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(0, 5))
        chip_frame.columnconfigure(0, weight=1)

        # 搜索框和厂商筛选
        search_frame = ttk.Frame(chip_frame)
        search_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 5))
        search_frame.columnconfigure(1, weight=1)

        ttk.Label(search_frame, text="搜索:").grid(row=0, column=0, sticky=tk.W)
        self.search_entry = ttk.Entry(search_frame)
        self.search_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(5, 5))
        self.search_entry.bind("<KeyRelease>", self._on_search)

        ttk.Label(search_frame, text="厂商:").grid(row=0, column=2, sticky=tk.W, padx=(10, 0))
        self.vendor_combo = ttk.Combobox(search_frame, width=15)
        self.vendor_combo['values'] = ['全部'] + self.db.get_all_vendors()
        self.vendor_combo.current(0)
        self.vendor_combo.grid(row=0, column=3, sticky=tk.W, padx=(5, 5))
        self.vendor_combo.bind("<<ComboboxSelected>>", self._on_vendor_filter)

        ttk.Button(search_frame, text="清除", command=self._clear_search).grid(row=0, column=4)

        # 芯片列表
        ttk.Label(chip_frame, text="芯片列表:").grid(row=1, column=0, sticky=tk.W)
        self.chip_tree = ttk.Treeview(chip_frame, columns=("name", "vendor", "flash", "ram"), show="headings", height=8)
        self.chip_tree.heading("name", text="型号")
        self.chip_tree.heading("vendor", text="厂商")
        self.chip_tree.heading("flash", text="Flash")
        self.chip_tree.heading("ram", text="RAM")
        self.chip_tree.column("name", width=150)
        self.chip_tree.column("vendor", width=100)
        self.chip_tree.column("flash", width=80)
        self.chip_tree.column("ram", width=80)
        self.chip_tree.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        self.chip_tree.bind("<<TreeviewSelect>>", self._on_chip_select)

        # 滚动条
        chip_scroll = ttk.Scrollbar(chip_frame, orient=tk.VERTICAL, command=self.chip_tree.yview)
        chip_scroll.grid(row=2, column=1, sticky=(tk.N, tk.S))
        self.chip_tree.configure(yscrollcommand=chip_scroll.set)

        # 3. 芯片信息区域
        info_frame = ttk.LabelFrame(main_frame, text="芯片信息", padding="10")
        info_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(5, 0))
        info_frame.columnconfigure(1, weight=1)

        self.info_text = scrolledtext.ScrolledText(info_frame, width=50, height=12, wrap=tk.WORD, state=tk.DISABLED)
        self.info_text.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 4. 操作区域
        op_frame = ttk.LabelFrame(main_frame, text="Flash操作", padding="10")
        op_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(10, 0))

        # 文件选择
        file_frame = ttk.Frame(op_frame)
        file_frame.grid(row=0, column=0, columnspan=4, sticky=(tk.W, tk.E), pady=(0, 10))
        file_frame.columnconfigure(0, weight=1)

        ttk.Label(file_frame, text="文件:").grid(row=0, column=0, sticky=tk.W)
        self.file_entry = ttk.Entry(file_frame, width=50)
        self.file_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(5, 5))
        ttk.Button(file_frame, text="浏览...", command=self._browse_file).grid(row=0, column=2)

        # 地址设置
        addr_frame = ttk.Frame(op_frame)
        addr_frame.grid(row=1, column=0, columnspan=4, sticky=tk.W, pady=(0, 10))

        ttk.Label(addr_frame, text="起始地址:").grid(row=0, column=0, sticky=tk.W)
        self.addr_entry = ttk.Entry(addr_frame, width=12)
        self.addr_entry.insert(0, "0x08000000")
        self.addr_entry.grid(row=0, column=1, sticky=tk.W, padx=(5, 15))

        ttk.Label(addr_frame, text="大小:").grid(row=0, column=2, sticky=tk.W)
        self.size_entry = ttk.Entry(addr_frame, width=12)
        self.size_entry.insert(0, "0x10000")
        self.size_entry.grid(row=0, column=3, sticky=tk.W, padx=(5, 0))

        # 操作按钮
        btn_frame = ttk.Frame(op_frame)
        btn_frame.grid(row=2, column=0, columnspan=4, sticky=(tk.W, tk.E))

        self.read_btn = ttk.Button(btn_frame, text="读取", command=self._read_flash, state=tk.DISABLED)
        self.read_btn.pack(side=tk.LEFT, padx=(0, 5))

        self.write_btn = ttk.Button(btn_frame, text="写入", command=self._write_flash, state=tk.DISABLED)
        self.write_btn.pack(side=tk.LEFT, padx=(0, 5))

        self.erase_btn = ttk.Button(btn_frame, text="擦除", command=self._erase_flash, state=tk.DISABLED)
        self.erase_btn.pack(side=tk.LEFT, padx=(0, 5))

        self.erase_all_btn = ttk.Button(btn_frame, text="全片擦除", command=self._erase_chip, state=tk.DISABLED)
        self.erase_all_btn.pack(side=tk.LEFT, padx=(0, 5))

        # 5. 日志区域
        log_frame = ttk.LabelFrame(main_frame, text="日志", padding="10")
        log_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(10, 0))
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)

        self.log_text = scrolledtext.ScrolledText(log_frame, wrap=tk.WORD, height=15)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        log_scroll = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.log_text.yview)
        log_scroll.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.log_text.configure(yscrollcommand=log_scroll.set)

        # 填充芯片列表
        self._populate_chip_list(self.db.chips)

    def _log(self, message: str, level: str = "INFO"):
        """记录日志"""
        import datetime
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{timestamp}] [{level}] {message}\n")
        self.log_text.see(tk.END)

    def _refresh_ports(self):
        """刷新串口列表"""
        ports = self.comm.list_available_ports()
        self.port_combo['values'] = [p[0] for p in ports]
        if ports:
            self.port_combo.current(0)
        self._log(f"找到 {len(ports)} 个串口")

    def _toggle_connection(self):
        """切换连接状态"""
        if self.comm.is_connected:
            # 断开连接
            self._disconnect()
        else:
            # 连接
            self._connect()

    def _connect(self):
        """连接到编程器"""
        port = self.port_combo.get()
        if not port:
            messagebox.showwarning("警告", "请选择串口")
            return

        self._log(f"正在连接到 {port}...")
        if self.comm.connect(port):
            self.connected_port = port
            self.connect_btn.config(text="断开")
            self.status_label.config(text=f"已连接 ({port})", foreground="green")
            self._update_buttons(True)
            self._log("连接成功")

            # 尝试连接芯片
            self._log("正在检测芯片...")
            if self.comm.connect_to_chip():
                chip_id = self.comm.get_chip_id()
                self._log(f"芯片ID: 0x{chip_id:08X}")
            else:
                self._log("连接芯片失败，请检查接线", "WARNING")
        else:
            messagebox.showerror("错误", "连接失败")
            self._log("连接失败", "ERROR")

    def _disconnect(self):
        """断开连接"""
        if self.comm.is_connected:
            self.comm.disconnect_from_chip()
            self.comm.disconnect()
        self.connected_port = None
        self.connect_btn.config(text="连接")
        self.status_label.config(text="未连接", foreground="red")
        self._update_buttons(False)
        self._log("已断开连接")

    def _update_buttons(self, connected: bool):
        """更新按钮状态"""
        state = tk.NORMAL if connected else tk.DISABLED
        self.read_btn.config(state=state)
        self.write_btn.config(state=state)
        self.erase_btn.config(state=state)
        self.erase_all_btn.config(state=state)

    def _populate_chip_list(self, chips):
        """填充芯片列表"""
        # 清空
        for item in self.chip_tree.get_children():
            self.chip_tree.delete(item)

        # 添加
        for chip in chips:
            status = chip.get('status', '')
            tag = 'supported' if status == 'supported' else ''
            self.chip_tree.insert('', tk.END, values=(
                chip.get('name', ''),
                chip.get('vendor', ''),
                format_size(chip.get('flash_size', 0)),
                format_size(chip.get('ram_size', 0))
            ), tags=(tag,))

        self.chip_tree.tag_configure('supported', foreground='black')

    def _on_search(self, event):
        """搜索事件处理"""
        self._apply_filter()

    def _on_vendor_filter(self, event):
        """厂商筛选事件处理"""
        self._apply_filter()

    def _apply_filter(self):
        """应用搜索和筛选条件"""
        query = self.search_entry.get()
        vendor = self.vendor_combo.get()

        results = self.db.search_chips(query)

        if vendor != '全部':
            results = [chip for chip in results if chip.get('vendor') == vendor]

        self._populate_chip_list(results)
        count = len(results)
        if query and vendor != '全部':
            self._log(f"搜索 '{query}' 且厂商 '{vendor}' 找到 {count} 款芯片")
        elif query:
            self._log(f"搜索 '{query}' 找到 {count} 款芯片")
        elif vendor != '全部':
            self._log(f"筛选厂商 '{vendor}' 找到 {count} 款芯片")

    def _clear_search(self):
        """清除搜索"""
        self.search_entry.delete(0, tk.END)
        self.vendor_combo.current(0)
        self._populate_chip_list(self.db.chips)

    def _on_chip_select(self, event):
        """芯片选择事件处理"""
        selection = self.chip_tree.selection()
        if not selection:
            return

        item = self.chip_tree.item(selection[0])
        chip_name = item['values'][0]
        self.selected_chip = self.db.get_chip_by_name(chip_name)

        if self.selected_chip:
            self._update_chip_info(self.selected_chip)

    def _update_chip_info(self, chip: Dict[str, Any]):
        """更新芯片信息显示"""
        self.info_text.config(state=tk.NORMAL)
        self.info_text.delete(1.0, tk.END)

        info_text = f"""型号: {chip.get('name', 'N/A')}
厂商: {chip.get('vendor', 'N/A')}
系列: {chip.get('family', 'N/A')}
内核: {chip.get('core', 'N/A')}
Flash: {format_size(chip.get('flash_size', 0))}
RAM: {format_size(chip.get('ram_size', 0))}
封装: {chip.get('package', 'N/A')}
调试接口: {', '.join(chip.get('debug_interfaces', []))}
状态: {chip.get('status', 'N/A')}
"""
        self.info_text.insert(tk.END, info_text)
        self.info_text.config(state=tk.DISABLED)

    def _browse_file(self):
        """浏览文件"""
        filename = filedialog.asksaveasfilename(
            title="选择文件",
            filetypes=(("Intel Hex", "*.hex"), ("Binary", "*.bin"), ("All files", "*.*"))
        )
        if filename:
            self.file_entry.delete(0, tk.END)
            self.file_entry.insert(0, filename)

    def _parse_hex(self, s: str) -> int:
        """解析十六进制或十进制数"""
        try:
            if s.lower().startswith('0x'):
                return int(s, 16)
            else:
                return int(s)
        except ValueError:
            return 0

    def _read_flash(self):
        """读取Flash"""
        if not self.comm.is_connected:
            messagebox.showwarning("警告", "请先连接编程器")
            return

        filename = self.file_entry.get()
        if not filename:
            messagebox.showwarning("警告", "请选择文件")
            return

        address = self._parse_hex(self.addr_entry.get())
        size = self._parse_hex(self.size_entry.get())

        self._log(f"正在读取 Flash... 地址: 0x{address:08X}, 大小: {format_size(size)}")

        def do_read():
            try:
                data = self.comm.read_flash(address, size)
                if data is not None:
                    with open(filename, 'wb') as f:
                        f.write(data)
                    self._log(f"读取完成，已保存到 {filename}")
                    self.root.after(0, lambda: messagebox.showinfo("成功", "读取完成"))
                else:
                    self._log("读取失败", "ERROR")
                    self.root.after(0, lambda: messagebox.showerror("错误", "读取失败"))
            except Exception as e:
                self._log(f"读取异常: {e}", "ERROR")

        threading.Thread(target=do_read, daemon=True).start()

    def _write_flash(self):
        """写入Flash"""
        if not self.comm.is_connected:
            messagebox.showwarning("警告", "请先连接编程器")
            return

        filename = self.file_entry.get()
        if not filename or not os.path.exists(filename):
            messagebox.showwarning("警告", "请选择有效的文件")
            return

        address = self._parse_hex(self.addr_entry.get())

        self._log(f"正在写入 Flash... 地址: 0x{address:08X}")

        def do_write():
            try:
                with open(filename, 'rb') as f:
                    data = f.read()

                if self.comm.write_flash(address, data):
                    self._log(f"写入完成，共写入 {format_size(len(data))}")
                    self.root.after(0, lambda: messagebox.showinfo("成功", "写入完成"))
                else:
                    self._log("写入失败", "ERROR")
                    self.root.after(0, lambda: messagebox.showerror("错误", "写入失败"))
            except Exception as e:
                self._log(f"写入异常: {e}", "ERROR")

        threading.Thread(target=do_write, daemon=True).start()

    def _erase_flash(self):
        """擦除Flash"""
        if not self.comm.is_connected:
            messagebox.showwarning("警告", "请先连接编程器")
            return

        address = self._parse_hex(self.addr_entry.get())
        size = self._parse_hex(self.size_entry.get())

        if not messagebox.askyesno("确认", f"确定要擦除 Flash 吗？\n地址: 0x{address:08X}\n大小: {format_size(size)}"):
            return

        self._log(f"正在擦除 Flash...")

        def do_erase():
            try:
                if self.comm.erase_flash(address, size):
                    self._log("擦除完成")
                    self.root.after(0, lambda: messagebox.showinfo("成功", "擦除完成"))
                else:
                    self._log("擦除失败", "ERROR")
                    self.root.after(0, lambda: messagebox.showerror("错误", "擦除失败"))
            except Exception as e:
                self._log(f"擦除异常: {e}", "ERROR")

        threading.Thread(target=do_erase, daemon=True).start()

    def _erase_chip(self):
        """全片擦除"""
        if not self.comm.is_connected:
            messagebox.showwarning("警告", "请先连接编程器")
            return

        if not messagebox.askyesno("确认", "确定要全片擦除吗？此操作不可恢复！"):
            return

        self._log("正在全片擦除...")

        def do_erase_all():
            try:
                if self.comm.erase_chip():
                    self._log("全片擦除完成")
                    self.root.after(0, lambda: messagebox.showinfo("成功", "全片擦除完成"))
                else:
                    self._log("全片擦除失败", "ERROR")
                    self.root.after(0, lambda: messagebox.showerror("错误", "全片擦除失败"))
            except Exception as e:
                self._log(f"全片擦除异常: {e}", "ERROR")

        threading.Thread(target=do_erase_all, daemon=True).start()


def main():
    root = tk.Tk()
    app = ProgrammerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
