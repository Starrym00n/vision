# MaixCAM Pro / MaixCam2 移植可行性分析与实现逻辑

## 1. 项目背景

本项目是 **IPCS13 综合设计 —— 三子棋游戏装置** 的视觉部分，原代码运行在 **树莓派5** 上，使用 Picamera2 采集图像、OpenCV 进行棋盘/棋子识别、串口返回结果给 STM32 主控。

目标是将代码移植到 **Sipeed MaixCAM Pro / MaixCam2** 平台，使用 **MaixPy v4** + **OpenCV + NumPy** 实现相同功能。

---

## 2. 原代码架构分析

原代码 [opencv_ds_v1.00.py](../Pi/opencv_ds_v1.00.py) 实现了两种工作模式，通过 GPIO18 电平切换：

### 模式 1：棋子颜色识别（GPIO 低电平）
1. 采集图像 → 缩放至 640×360
2. 灰度化 → 高斯模糊 → Canny 边缘检测 → 闭运算
3. `findContours` 查找轮廓 → `approxPolyDP` 近似为四边形
4. 筛选面积在 [1100, 10000] 范围内的小矩形（棋盘格子）
5. 计算每个小矩形中心点 → `sort_points()` 按空间位置排序为 3×3 矩阵
6. 切换到模式 2

### 模式 2：棋子颜色判定（紧接模式 1）
1. 在每个中心点周围采样像素颜色
2. 将像素转 HSV，与预设色域比对，判定颜色（0=黄/空、1=黑、2=白）
3. 按映射关系重排为 `b[0]~b[8]`（对应物理棋盘的 row/col）
4. 通过串口发送棋子状态字符串，如 `[0,1,2,0,1,0,2,0,1]`

### 角度识别模式（GPIO 高电平）
1. 类似的边缘检测流程
2. 筛选面积 > 33000 的大矩形（整个棋盘边框）
3. `minAreaRect` 获取旋转角度
4. 串口发送 `[XX]` 格式角度值

### 串口协议（面向 STM32）
- 棋子状态：`[0,1,2,0,1,0,2,0,1]`（9 位，逗号分隔，无空格）
- 棋盘角度：`[05]`（两位数字，方括号包裹）

---

## 3. 平台差异对比

| 组件 | 树莓派5 | MaixCAM Pro / MaixCam2 |
|------|---------|------------------------|
| **摄像头** | `Picamera2`（CSI 接口） | `maix.camera`（MIPI CSI） |
| **图像格式** | `capture_array()` → numpy BGR | `cam.read()` → `maix.image.Image`，需转换为 numpy |
| **串口** | `serial.Serial("/dev/ttyAMA2", 9600)` | `uart.UART("/dev/ttyS1", 9600)` |
| **GPIO** | `RPi.GPIO`（BCM 编号） | `maix.gpio` + `maix.pinmap`（引脚名如 A19） |
| **OpenCV** | 系统级安装 | MaixPy v4 基于标准 Python 3，支持 `import cv2` |
| **NumPy** | 系统级安装 | 同上，标准 Python 包 |
| **显示** | `cv2.imshow()` + `cv2.waitKey()` | 无 GUI；需显示可用 `maix.display`，生产环境不需要 |
| **运行环境** | Raspberry Pi OS（完整 Linux） | Buildroot 定制 Linux（精简） |

---

## 4. 可行性结论

### ✅ 完全可行
- MaixPy v4 运行 **标准 Python 3**，支持 `import cv2` 和 `import numpy`（[官方文档确认](https://wiki.sipeed.com/maixpy/doc/zh/index.html)）
- 摄像头 API `maix.camera` 功能完整，支持分辨率配置（[摄像头文档](https://github.com/sipeed/MaixPy/blob/main/docs/doc/zh/vision/camera.md)）
- UART 串口 `maix.uart` 支持自定义波特率和引脚映射（[UART 文档](https://wiki.sipeed.com/maixpy/doc/zh/peripheral/uart.html)）
- GPIO 通过 `maix.gpio` + `maix.pinmap` 实现输入读取（[GPIO 文档](https://wiki.sipeed.com/maixpy/doc/zh/peripheral/gpio.html)）

### ⚠️ 需要注意

1. **图像转换开销**：`maix.image.Image` → numpy 数组有额外拷贝开销，对实时性有一定影响
2. **OpenCV 裁剪版**：MaixPy 固件中的 OpenCV 可能是裁剪版，`findContours`、`approxPolyDP` 等函数需实测验证
3. **`cv2.imshow` 不可用**：MaixCAM Pro 无 X11 桌面，所有 GUI 显示代码需移除或替换为 `maix.display`
4. **UART0 是系统口**：MaixCAM Pro 的 UART0（`/dev/ttyS0`，引脚 A16/A17）是系统终端口，**不能用于 STM32 通信**，应使用 UART1（`/dev/ttyS1`）
5. **TX 引脚开机保护**：A16（UART0_TX）是 boot 模式检测引脚，开机时**不可被拉低**，否则板子无法启动

### ❓ 需实测验证（无板端证据，仅为推断）
- `cv2.findContours` 在 MaixPy OpenCV 中的完整兼容性（包括返回值格式）
- `maix.image.Image` 转 numpy 的具体 API（`img.to_numpy()` 或其他方法）
- 实际帧率是否满足实时识别需求
- 预装 OpenCV 是否包含 `imgproc` 模块

---

## 5. 移植实现逻辑

### 5.1 模块替换映射

| 原代码 | 移植后 |
|--------|--------|
| `import cv2; import numpy as np` | 保留（MaixPy v4 支持） |
| `from picamera2 import Picamera2` | `from maix import camera` |
| `import serial` | `from maix import uart` |
| `import RPi.GPIO as GPIO` | `from maix import gpio, pinmap` |
| `import time` | `from maix import time`（MaixPy 时间模块） |

### 5.2 摄像头初始化

```python
# 原：picam2 = Picamera2()
# 原：config = picam2.create_preview_configuration(main={"format":'RGB888',"size":(1280,720)})
# 原：picam2.start()

# 移植后：
cam = camera.Camera(640, 360)  # 直接设置目标分辨率
```

> 说明：原代码先以 1280×720 初始化再 `resize` 到 640×360，MaixCAM Pro 上直接使用目标分辨率更高效。

### 5.3 图像获取与转换

```python
# 原：img = picam2.capture_array()  → 返回 numpy BGR 数组

# 移植后（方案 A，推荐）：
img_maix = cam.read()           # 返回 maix.image.Image
img = img_maix.to_numpy()       # 转为 numpy 数组（RGB 格式）

# 移植后（方案 B，备选 - 直接用 OpenCV）：
# cap = cv2.VideoCapture(0)
# ret, img = cap.read()          # 返回 numpy BGR 格式
```

> **重要**：`to_numpy()` 返回的是 RGB 格式，而原代码基于 BGR 处理。需要 `cv2.cvtColor(img, cv2.COLOR_RGB2BGR)` 或调整后续处理的颜色通道顺序。

### 5.4 串口初始化与收发

```python
# 原：serialPort = serial.Serial("/dev/ttyAMA2", 9600)
# 原：serialPort.write(uart.encode())

# 移植后：
serial_dev = uart.UART("/dev/ttyS1", 9600)  # 使用 UART1
serial_dev.write_str(uart)                    # 直接发送字符串
# 或
serial_dev.write(uart.encode())               # 编码后发送字节
```

### 5.5 GPIO 输入（模式选择）

```python
# 原：
# GPIO.setmode(GPIO.BCM)
# GPIO.setup(18, GPIO.IN)
# GPIO.input(18)

# 移植后：
from maix import gpio, pinmap
pinmap.set_pin_function("A19", "GPIOA19")       # 选择合适的引脚
mode_pin = gpio.GPIO("GPIOA19", gpio.Mode.IN)
mode_pin.value()                                # 读取电平：0 或 1
```

### 5.6 移除 GUI 相关代码

所有 `cv2.imshow()` 和 `cv2.waitKey()` 调用需要移除。如需调试显示，可使用：

```python
from maix import display
disp = display.Display()
disp.show(img_maix)  # 显示 maix.image.Image
```

### 5.7 主循环调整

```python
# 原：while True + time.sleep()
# 移植后：while True + time.sleep_ms() 或 time.sleep()
# MaixPy 的 time 模块兼容 time.sleep()，也提供 time.sleep_ms()
```

---

## 6. 关键注意事项

### 6.1 颜色空间一致性
原代码中摄像头输出为 RGB888 格式，但 OpenCV 处理基于 BGR。树莓派的 `Picamera2` 配置为 `RGB888` 但实际 `capture_array()` 返回 BGR（Picamera2 内部做了转换）。MaixCAM Pro 的 `camera.read()` 返回 RGB，转 numpy 后需要确认通道顺序。

### 6.2 串口波特率
原代码使用 9600 波特率。MaixCAM Pro 的 UART 支持 9600，但文档指出 **115200 是最通用的波特率**，其他波特率可能存在误差。建议保持 9600 与 STM32 端一致，但需实测通信稳定性。

### 6.3 性能考量
MaixCAM Pro 搭载 ARM A53 双核 + 3.2 TOPS NPU，CPU 性能弱于树莓派5。主要瓶颈在：
- 图像格式转换（maix.image → numpy）
- OpenCV 处理（Canny、findContours 等 CPU 密集操作）

建议优化方向：
- 降低处理帧率（不需要每帧都识别）
- 减少不必要的图像拷贝
- 考虑使用 MaixPy 内置的图像处理 API 替代部分 OpenCV 操作

### 6.4 开机自启
原代码支持树莓派 `systemctl` 自启。MaixCAM Pro 的 MaixPy 支持在 SD 卡根目录放置 `main.py` 自动执行，或通过 `/boot/maixpy/` 配置自启脚本。

---

## 7. 验收清单

- [ ] MaixCAM Pro 能正常初始化摄像头并获取图像
- [ ] OpenCV 的 `findContours` 和 `approxPolyDP` 正常工作
- [ ] 棋盘格子检测准确（面积阈值可能需微调）
- [ ] 中心点排序正确（3×3 矩阵顺序一致）
- [ ] HSV 颜色判定准确（色域阈值可能需微调）
- [ ] UART 串口能与 STM32 正常通信
- [ ] GPIO 输入能正确读取模式切换信号
- [ ] 棋子状态格式 `[X,X,X,X,X,X,X,X,X]` 与原协议一致
- [ ] 棋盘角度格式 `[XX]` 与原协议一致
- [ ] 整体识别速度满足实时要求（< 1s/帧）

---

## 8. 参考资料

- [MaixPy 快速开始](https://wiki.sipeed.com/maixpy/doc/zh/index.html)
- [MaixPy UART 串口文档](https://wiki.sipeed.com/maixpy/doc/zh/peripheral/uart.html)
- [MaixPy GPIO 文档](https://wiki.sipeed.com/maixpy/doc/zh/peripheral/gpio.html)
- [MaixPy Camera 文档](https://github.com/sipeed/MaixPy/blob/main/docs/doc/zh/vision/camera.md)
- [MaixPy Pinmap 文档](https://wiki.sipeed.com/maixpy/doc/zh/peripheral/pinmap.html)
- [IPCS13 三子棋游戏装置任务书](C:/Users/ASUS/Desktop/IPCS13综合设计/B_三子棋游戏装置.pdf)
