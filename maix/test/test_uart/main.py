# from maix import uart

# ##########################################################
# # 测试 2：串口收发验证
# # 验证目标：UART1 能否正常发送数据到 STM32
# # 运行方式：MaixVision 直接加载运行
# # 前置条件：STM32 端串口助手已打开，监听对应端口
# #
# # 通过标准：
# #   1. 不报错
# #   2. STM32 端串口助手收到以下数据：
# #      - "TEST_START"
# #      - "[012010201]"    （棋子状态格式）
# #      - "[25]"           （角度格式）
# #      - "TEST_END"
# #
# # 如果失败：
# #   - 确认 /dev/ttyS1 未被其他进程占用
# #   - 检查 UART 接线（TX→RX, RX→TX 交叉连接）
# #   - 确认 STM32 端波特率也为 115200
# #   - 可尝试换用其他串口：print(uart.list_devices())
# ##########################################################

# print("=== 串口收发测试 ===")
# print("初始化 UART1...")

# serial_dev = uart.UART("/dev/ttyS1", 115200)

# print("串口初始化成功")
# print("可用串口设备:", uart.list_devices())

# # 测试 1：发送固定字符串
# print("发送: TEST_START")
# serial_dev.write_str("TEST_START")

# # 测试 2：发送棋子状态格式（模拟 9 个格子全是空位）
# test_chess = "[000000000]"
# print("发送: {}".format(test_chess))
# serial_dev.write_str(test_chess)

# # 测试 3：发送棋子状态格式（模拟有黑子白子）
# test_chess2 = "[012010201]"
# print("发送: {}".format(test_chess2))
# serial_dev.write_str(test_chess2)

# # 测试 4：发送角度格式
# test_angle = "[25]"
# print("发送: {}".format(test_angle))
# serial_dev.write_str(test_angle)

# # 测试 5：发送结束标记
# print("发送: TEST_END")
# serial_dev.write_str("TEST_END")

# # 测试 6：循环发送 5 次，验证持续发送稳定性
# for i in range(5):
#     msg = "LOOP_{}".format(i)
#     serial_dev.write_str(msg)
#     print("发送: {}".format(msg))

# print("=== 测试完成 ===")
# print("请在 STM32 端串口助手中确认是否收到以上所有数据")


from maix import uart, time

##########################################################
# 电脑串口测试程序
# 功能：
#   1. 循环发送测试数据到电脑
#   2. 接收电脑发来的数据并原样返回（回显）
#
# 使用方式：
#   1. 按接线图连接 USB 转串口模块
#   2. 电脑打开串口助手，波特率 115200
#   3. 本程序运行后会自动发送测试数据
#   4. 在串口助手发送任意数据，会收到相同数据
#
# 通过标准：
#   1. 电脑串口助手收到 "HELLO FROM MAIXCAM"
#   2. 电脑发送数据，MaixCAM 原样返回
##########################################################

print("=== 电脑串口测试 ===")

# 初始化串口
serial_dev = uart.UART("/dev/ttyS1", 115200)
print("串口初始化成功")

# 测试 1：自动发送测试数据
print("发送测试数据...")
for i in range(5):
    msg = "HELLO FROM MAIXCAM {}".format(i)
    serial_dev.write_str(msg)
    print("已发送: {}".format(msg))
    time.sleep(1)

print("等待接收电脑数据...")

# 测试 2：接收电脑发来的数据并回显
for i in range(30):
    # 检查是否有数据可读
    if serial_dev.read_len() > 0:
        data = serial_dev.read()
        if data:
            received = data.decode('utf-8', errors='ignore')
            print("收到: {}".format(received))
            # 回显：将收到的数据原样返回
            serial_dev.write_str(received)
            print("回显: {}".format(received))
    time.sleep(0.1)

print("=== 测试完成 ===")
