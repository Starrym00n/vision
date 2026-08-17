from maix import gpio, pinmap, time

##########################################################
# 测试 3：GPIO 输入读取验证
# 验证目标：A19 引脚能否正确读取 STM32 输出的高低电平
# 运行方式：MaixVision 直接加载运行
# 前置条件：用杜邦线将 A19 手动接 GND 或 3.3V 进行测试
#
# 通过标准：
#   1. A19 接 GND 时，打印 value = 0
#   2. A19 接 3.3V 时，打印 value = 1
#   3. 切换电平后 1 秒内能检测到变化
#
# ⚠️ 安全警告：
#   MaixCAM Pro 引脚为 3.3V 耐受，禁止接入 5V 电压！
#
# 如果失败：
#   - 确认 pinmap.set_pin_function 执行成功
#   - 检查杜邦线连接是否牢固
#   - 尝试换用其他引脚（如 A14 板载 LED 仅支持输出）
##########################################################

print("=== GPIO 输入读取测试 ===")
print("配置 A19 为 GPIO 输入...")

# 将 A19 引脚复用为 GPIO 功能
pinmap.set_pin_function("A19", "GPIOA19")
# 创建 GPIO 输入对象
mode_pin = gpio.GPIO("GPIOA19", gpio.Mode.IN)

print("GPIO 初始化成功")
print("")
print("请用杜邦线操作：")
print("  - 接 GND → 预期 value = 0（棋子识别模式）")
print("  - 接 3.3V → 预期 value = 1（角度检测模式）")
print("")

# 循环读取 30 次，每次间隔 1 秒，方便手动切换电平观察
for i in range(30):
    val = mode_pin.value()
    state = "LOW (棋子识别)" if val == 0 else "HIGH (角度检测)"
    print("[{}/30] A19 = {} -> {}".format(i + 1, val, state))
    time.sleep(1)

print("=== 测试完成 ===")
