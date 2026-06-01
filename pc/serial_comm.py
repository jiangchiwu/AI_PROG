"""
USB CDC串口通信模块
"""
import serial
import serial.tools.list_ports
import time
from typing import List, Tuple, Optional, Any, Dict
from queue import Queue
import threading


class SerialCommunicator:
    """串口通信类"""

    # 协议命令定义
    CMD_CONNECT = 0x01
    CMD_DISCONNECT = 0x02
    CMD_READ_FLASH = 0x10
    CMD_WRITE_FLASH = 0x11
    CMD_ERASE_FLASH = 0x12
    CMD_ERASE_CHIP = 0x13
    CMD_READ_MEM = 0x20
    CMD_WRITE_MEM = 0x21
    CMD_RESET = 0x30
    CMD_HALT = 0x31
    CMD_RESUME = 0x32
    CMD_GET_CHIP_ID = 0x40
    CMD_GET_CHIP_INFO = 0x41

    RESP_OK = 0x00
    RESP_ERROR = 0xFF
    RESP_DATA = 0x01
    RESP_INFO = 0x02

    def __init__(self, timeout: float = 1.0):
        self.serial_port: Optional[serial.Serial] = None
        self.timeout = timeout
        self.is_connected = False
        self.receive_queue = Queue()
        self.receive_thread: Optional[threading.Thread] = None
        self.running = False
        self._lock = threading.Lock()

    @staticmethod
    def list_available_ports() -> List[Tuple[str, str, str]]:
        """
        列出所有可用的串口
        :return: 串口列表 (端口, 描述, 硬件ID)
        """
        ports = serial.tools.list_ports.comports()
        port_list = []
        for port in ports:
            port_list.append((
                port.device,
                port.description,
                port.hwid
            ))
        return port_list

    def connect(self, port: str, baudrate: int = 115200) -> bool:
        """
        连接到串口
        :param port: 串口号
        :param baudrate: 波特率
        :return: 是否连接成功
        """
        try:
            with self._lock:
                self.serial_port = serial.Serial(
                    port=port,
                    baudrate=baudrate,
                    bytesize=serial.EIGHTBITS,
                    parity=serial.PARITY_NONE,
                    stopbits=serial.STOPBITS_ONE,
                    timeout=self.timeout
                )
                self.is_connected = True
                self.running = True

                # 启动接收线程
                self.receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
                self.receive_thread.start()

            return True
        except Exception as e:
            print(f"连接串口失败: {e}")
            return False

    def disconnect(self):
        """断开串口连接"""
        with self._lock:
            self.running = False
            self.is_connected = False

            if self.serial_port:
                try:
                    self.serial_port.close()
                except:
                    pass
                self.serial_port = None

    def _receive_loop(self):
        """接收数据的循环线程"""
        buffer = bytearray()
        while self.running and self.is_connected:
            try:
                if self.serial_port and self.serial_port.in_waiting > 0:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    buffer.extend(data)

                    # 尝试解析数据包
                    while len(buffer) >= 5:  # 最小包长
                        # 检查帧头
                        if buffer[0] != 0xAA:
                            buffer.pop(0)
                            continue

                        # 获取数据包长度
                        length = buffer[1]
                        if len(buffer) < length + 4:  # 完整包需要
                            break

                        # 检查校验和
                        checksum = sum(buffer[2:length+2]) & 0xFF
                        if checksum != buffer[length+2]:
                            buffer.pop(0)
                            continue

                        # 检查帧尾
                        if buffer[length+3] != 0x55:
                            buffer.pop(0)
                            continue

                        # 提取有效数据
                        packet_data = buffer[2:length+2]
                        self.receive_queue.put(bytes(packet_data))

                        # 移除已处理数据
                        buffer = buffer[length+4:]
                else:
                    time.sleep(0.01)
            except Exception as e:
                print(f"接收数据异常: {e}")
                break

    def _send_packet(self, command: int, data: bytes = b'') -> bool:
        """
        发送数据包
        :param command: 命令字节
        :param data: 数据
        :return: 是否发送成功
        """
        if not self.is_connected or not self.serial_port:
            return False

        try:
            with self._lock:
                # 构建数据包
                packet = bytearray()
                packet.append(0xAA)  # 帧头
                packet.append(len(data) + 1)  # 长度 (命令 + 数据)
                packet.append(command)  # 命令
                packet.extend(data)  # 数据

                # 计算校验和
                checksum = sum(packet[2:]) & 0xFF
                packet.append(checksum)
                packet.append(0x55)  # 帧尾

                # 发送
                self.serial_port.write(packet)
                return True
        except Exception as e:
            print(f"发送数据包失败: {e}")
            return False

    def _receive_response(self, timeout: float = 1.0) -> Optional[Tuple[int, bytes]]:
        """
        接收响应包
        :param timeout: 超时时间
        :return: (响应类型, 数据)
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                if not self.receive_queue.empty():
                    packet = self.receive_queue.get_nowait()
                    if packet:
                        return (packet[0], packet[1:])
                time.sleep(0.01)
            except:
                pass
        return None

    def connect_to_chip(self) -> bool:
        """连接到芯片"""
        if not self._send_packet(self.CMD_CONNECT):
            return False

        resp = self._receive_response()
        return resp is not None and resp[0] == self.RESP_OK

    def disconnect_from_chip(self) -> bool:
        """断开芯片连接"""
        if not self._send_packet(self.CMD_DISCONNECT):
            return False

        resp = self._receive_response()
        return resp is not None and resp[0] == self.RESP_OK

    def get_chip_id(self) -> Optional[int]:
        """获取芯片ID"""
        if not self._send_packet(self.CMD_GET_CHIP_ID):
            return None

        resp = self._receive_response()
        if resp is not None and resp[0] == self.RESP_DATA and len(resp[1]) >= 4:
            return int.from_bytes(resp[1][:4], byteorder='little')
        return None

    def get_chip_info(self) -> Optional[Dict[str, Any]]:
        """获取芯片信息"""
        if not self._send_packet(self.CMD_GET_CHIP_INFO):
            return None

        resp = self._receive_response()
        if resp is not None and resp[0] == self.RESP_INFO:
            # 解析芯片信息 (简化)
            return {}
        return None

    def read_flash(self, address: int, size: int) -> Optional[bytes]:
        """
        读取Flash
        :param address: 起始地址
        :param size: 读取大小
        :return: 读取的数据
        """
        data = address.to_bytes(4, byteorder='little') + size.to_bytes(4, byteorder='little')
        if not self._send_packet(self.CMD_READ_FLASH, data):
            return None

        # 接收数据 (可能分多个包)
        all_data = bytearray()
        remaining = size
        while remaining > 0:
            resp = self._receive_response(timeout=5.0)
            if resp is None:
                return None
            if resp[0] == self.RESP_ERROR:
                return None
            if resp[0] == self.RESP_DATA:
                chunk = resp[1]
                all_data.extend(chunk)
                remaining -= len(chunk)
            if remaining <= 0:
                break

        return bytes(all_data)

    def write_flash(self, address: int, data: bytes) -> bool:
        """
        写入Flash
        :param address: 起始地址
        :param data: 要写入的数据
        :return: 是否写入成功
        """
        chunk_size = 256
        offset = 0
        total_size = len(data)

        while offset < total_size:
            chunk = data[offset:offset + chunk_size]
            packet_data = (address + offset).to_bytes(4, byteorder='little') + len(chunk).to_bytes(2, byteorder='little') + chunk

            if not self._send_packet(self.CMD_WRITE_FLASH, packet_data):
                return False

            resp = self._receive_response(timeout=3.0)
            if resp is None or resp[0] == self.RESP_ERROR:
                return False

            offset += len(chunk)

        return True

    def erase_flash(self, address: int, size: int) -> bool:
        """
        擦除Flash
        :param address: 起始地址
        :param size: 擦除大小
        :return: 是否擦除成功
        """
        data = address.to_bytes(4, byteorder='little') + size.to_bytes(4, byteorder='little')
        if not self._send_packet(self.CMD_ERASE_FLASH, data):
            return False

        resp = self._receive_response(timeout=30.0)
        return resp is not None and resp[0] == self.RESP_OK

    def erase_chip(self) -> bool:
        """
        全片擦除
        :return: 是否擦除成功
        """
        if not self._send_packet(self.CMD_ERASE_CHIP):
            return False

        resp = self._receive_response(timeout=60.0)
        return resp is not None and resp[0] == self.RESP_OK

    def read_memory(self, address: int, size: int) -> Optional[bytes]:
        """
        读取内存
        :param address: 起始地址
        :param size: 读取大小
        :return: 读取的数据
        """
        data = address.to_bytes(4, byteorder='little') + size.to_bytes(4, byteorder='little')
        if not self._send_packet(self.CMD_READ_MEM, data):
            return None

        resp = self._receive_response()
        if resp is not None and resp[0] == self.RESP_DATA:
            return resp[1]
        return None

    def write_memory(self, address: int, data: bytes) -> bool:
        """
        写入内存
        :param address: 起始地址
        :param data: 要写入的数据
        :return: 是否写入成功
        """
        packet_data = address.to_bytes(4, byteorder='little') + data
        if not self._send_packet(self.CMD_WRITE_MEM, packet_data):
            return False

        resp = self._receive_response()
        return resp is not None and resp[0] == self.RESP_OK

    def reset_chip(self) -> bool:
        """复位芯片"""
        if not self._send_packet(self.CMD_RESET):
            return False

        resp = self._receive_response()
        return resp is not None and resp[0] == self.RESP_OK

    def halt_chip(self) -> bool:
        """暂停芯片"""
        if not self._send_packet(self.CMD_HALT):
            return False

        resp = self._receive_response()
        return resp is not None and resp[0] == self.RESP_OK

    def resume_chip(self) -> bool:
        """恢复芯片运行"""
        if not self._send_packet(self.CMD_RESUME):
            return False

        resp = self._receive_response()
        return resp is not None and resp[0] == self.RESP_OK


if __name__ == "__main__":
    # 测试代码
    comm = SerialCommunicator()

    print("可用串口:")
    ports = comm.list_available_ports()
    for i, (port, desc, hwid) in enumerate(ports):
        print(f"  {i+1}. {port} - {desc}")

    if ports:
        print(f"\n尝试连接到 {ports[0][0]}...")
        if comm.connect(ports[0][0]):
            print("连接成功!")

            print("\n尝试连接到芯片...")
            if comm.connect_to_chip():
                chip_id = comm.get_chip_id()
                print(f"芯片ID: 0x{chip_id:08X}")

            comm.disconnect()
        else:
            print("连接失败!")
