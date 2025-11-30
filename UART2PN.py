import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports
import binascii
import threading

def crc16_modbus(data: bytes) -> bytes:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc = crc >> 1
    # 将高位和低位颠倒
    crc = crc.to_bytes(2, byteorder='little')[::-1]
    return crc

class SerialApp:
    def __init__(self, root):
        self.root = root
        self.root.title("串口数据处理器")

        self.font = ("Microsoft YaHei", 10)
        self.port_var = tk.StringVar()
        self.baudrate_var = tk.IntVar()
        self.baudrate_var.set(115200)
        self.serial = None
        self.send_flag = False

        tk.Label(root, text="串口号:", font=self.font).grid(row=0, column=0, padx=5, pady=5)
        self.port_combo = ttk.Combobox(root, textvariable=self.port_var, font=self.font)
        self.port_combo.grid(row=0, column=1, padx=5, pady=5)
        self.update_port_list()

        tk.Label(root, text="波特率:", font=self.font).grid(row=1, column=0, padx=5, pady=5)
        tk.Entry(root, textvariable=self.baudrate_var, font=self.font).grid(row=1, column=1, padx=5, pady=5)

        self.connect_button = tk.Button(root, text="连接", command=self.connect_serial, font=self.font)
        self.connect_button.grid(row=2, column=0, columnspan=2, pady=5)

        self.receive_labels = []
        self.receive_entries = []
        tk.Label(root, text="接收数据:", font=self.font).grid(row=3, column=0, padx=5, pady=5)
        for i in range(20):
            row = i // 2 + 4
            col = i % 2
            label = tk.Label(root, text=f"字节{i+1}:", font=self.font)
            label.grid(row=row, column=col*2, padx=5, pady=2)
            entry = tk.Entry(root, width=10, font=self.font)
            entry.grid(row=row, column=col*2+1, padx=5, pady=2)
            self.receive_labels.append(label)
            self.receive_entries.append(entry)

        self.send_labels = []
        self.send_entries = []
        tk.Label(root, text="发送数据:", font=self.font).grid(row=14, column=0, padx=5, pady=5)
        for i in range(20):
            row = i // 2 + 15
            col = i % 2
            label = tk.Label(root, text=f"字节{i+1}:", font=self.font)
            label.grid(row=row, column=col*2, padx=5, pady=2)
            entry = tk.Entry(root, width=10, font=self.font)
            entry.grid(row=row, column=col*2+1, padx=5, pady=2)
            self.send_labels.append(label)
            self.send_entries.append(entry)

        self.send_button = tk.Button(root, text="发送", command=self.toggle_send, font=self.font)
        self.send_button.grid(row=26, column=0, columnspan=2, pady=5)

    def update_port_list(self):
        ports = serial.tools.list_ports.comports()
        port_list = [port.device for port in ports]
        self.port_combo['values'] = port_list
        if port_list:
            self.port_var.set(port_list[0])

    def connect_serial(self):
        if self.serial and self.serial.is_open:
            self.serial.close()
            self.connect_button.config(text="连接")
        else:
            try:
                self.serial = serial.Serial(self.port_var.get(), self.baudrate_var.get(), timeout=0.1)
                self.connect_button.config(text="断开")
                self.read_serial_data()
            except serial.SerialException as e:
                tk.messagebox.showerror("错误", f"无法打开串口: {e}")

    def read_serial_data(self):
        if self.serial and self.serial.is_open:
            try:
                data = self.serial.read(27)  # 读取27字节数据
                if len(data) == 27:
                    self.process_data(data)
            except serial.SerialException as e:
                tk.messagebox.showerror("错误", f"读取串口数据失败: {e}")
        self.root.after(50, self.read_serial_data)  # 50ms 定时器

    def process_data(self, data):
        if data[:2] == b'\x8B\xA2' and data[2] == 0x05 and data[3:5] == b'\x14\x00':  # 接收命令为 0x05
            for i in range(20):
                self.receive_entries[i].delete(0, tk.END)
                self.receive_entries[i].insert(0, format(data[5 + i], '02X'))

    def send_data(self):
        if self.serial and self.serial.is_open:
            try:
                data = bytearray([0x8B, 0xA2, 0x04, 0x14, 0x00])  # 发送命令为 0x04
                for entry in self.send_entries:
                    byte = int(entry.get(), 16)
                    data.append(byte)
                crc = crc16_modbus(data)  # 计算 CRC 校验码
                data.extend(crc)
                self.serial.write(data)
            except ValueError as e:
                tk.messagebox.showerror("错误", f"输入数据无效: {e}")
            except serial.SerialException as e:
                tk.messagebox.showerror("错误", f"发送数据失败: {e}")

    def toggle_send(self):
        if self.send_flag:
            self.send_flag = False
            self.send_button.config(text="发送")
        else:
            self.send_flag = True
            self.send_button.config(text="停止")
            self.send_data()  # 立即发送一次
            self.timer_send()

    def timer_send(self):
        if self.send_flag:
            self.send_data()
            self.root.after(100, self.timer_send)  # 100ms 定时器

if __name__ == "__main__":
    root = tk.Tk()
    app = SerialApp(root)
    root.mainloop()