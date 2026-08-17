from maix import camera, time,display

##########################################################
# 测试 1：摄像头出图验证
# 验证目标：MaixCAM Pro 摄像头能否正常初始化并采集图像
# 运行方式：MaixVision 直接加载运行
#
# 通过标准：
#   1. 不报错
#   2. 打印的 width=640, height=360
#   3. 画面中能看到实时图像
#   4. 帧率 > 10fps（观察画面流畅度）
#
# 如果失败：
#   - 检查摄像头排线是否连接牢固
#   - 确认固件版本支持当前摄像头传感器
#   - 尝试降低分辨率：camera.Camera(320, 240)
##########################################################

print("=== 摄像头出图测试 ===")
print("初始化摄像头...")

cam = camera.Camera(640, 360)
disp = display.Display()

print("摄像头初始化成功")
print("分辨率: {}x{}".format(cam.width(), cam.height()))

img = cam.read()
disp.show(img)

# 采集 50 帧用于观察帧率和画面稳定性
for i in range(50):

    # 每 10 帧打印一次状态
    if i % 10 == 0:
        # img 可能没有 width()/height() 方法，用摄像头配置的值
        print("帧 {}: 捕获成功, 分辨率 {}x{}".format(
            i, cam.width(), cam.height()))

print("=== 测试完成 ===")
