from maix import uart, time

##########################################################
# UART 通讯测试程序（MaixCAM Pro → 电脑）
#
# 功能：
#   每秒发送一条测试数据到电脑，便于验证通讯是否正常
#
# 接线：
#   MaixCAM Pro A18 (UART1_TX) → USB 转串口模块 RXD
#   MaixCAM Pro A19 (UART1_RX) → USB 转串口模块 TXD
#   MaixCAM Pro GND → USB 转串口模块 GND
#
# 电脑端设置：
#   波特率：115200
#   数据位：8
#   停止位：1
#   校验位：无
#
# 通过标准：
#   电脑串口助手每秒收到一条 "HELLO XX" 消息
##########################################################

# # 初始化串口
# serial_dev = uart.UART("/dev/ttyS1", 115200)

# # 循环发送测试数据
# for i in range(10):
#     msg = "HELLO {}".format(i)
#     serial_dev.write_str(msg)
#     print("发送: {}".format(msg))
#     time.sleep(1)

from maix import app, uart, pinmap, time, sys, err

# ports = uart.list_devices()

# get pin and UART number according to device id
device_id = sys.device_id()
if device_id == "maixcam2":
    pin_function = {
        "A21": "UART4_TX",
        "A22": "UART4_RX"
        # "B0": "UART2_TX",
        # "B1": "UART2_RX"
    }
    device = "/dev/ttyS4"
    # device = "/dev/ttyS2"
else:
    pin_function = {
        "A16": "UART0_TX",
        "A17": "UART0_RX"
    }
    device = "/dev/ttyS1"

for pin, func in pin_function.items():
    err.check_raise(pinmap.set_pin_function(pin, func), f"Failed set pin{pin} function to {func}")

# Init UART
serial_dev = uart.UART(device, 115200)


data = "hello 1\r\n".encode()
serial_dev.write(data)
print("sent:", data)

data = "hello 2\r\n"
serial_dev.write_str(data)
print("sent:", data)

data = "object {} at x: {} y: {} w: {} h: {}, prob: {:.2f}\r\n".format("apple", 100, 100, 80, 80, 0.98123)
serial_dev.write_str(data)
print("sent:", data)

print("now wait receive data:")
while not app.need_exit():
    data = serial_dev.read()
    if data:
        print("Received, type: {}, len: {}, data: {}".format(type(data), len(data), data))
        serial_dev.write(data)

    time.sleep_ms(1) # sleep 1ms to make CPU free