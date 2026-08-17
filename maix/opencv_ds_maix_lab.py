import cv2
import numpy as np
import math
from maix import camera, uart, gpio, pinmap, time

##########################################################
# 三子棋游戏装置 —— 视觉识别主程序
# 适用于 MaixCAM Pro / MaixCam2 平台（MaixPy v4）
#
# 功能概述：
#   本程序负责三子棋比赛中视觉侧的全部感知任务，
#   通过摄像头俯视棋盘，识别 3×3 棋盘上每个格子的棋子颜色，
#   并通过串口将结果发送给 STM32 主控板，由主控驱动机械臂完成对弈。
#
# 两种工作模式（由 GPIO 引脚电平切换）：
#   GPIO 低电平 → 棋子识别模式：
#     mode 1：扫描棋盘，定位 9 个格子的中心坐标（仅执行一次）
#     mode 2：检测每个格子的棋子颜色，循环输出结果
#   GPIO 高电平 → 角度检测模式：
#     识别棋盘整体外框，计算放置偏移角度，供机械臂补偿
#
# 串口协议（面向 STM32）：
#   棋子状态：[012010201]  —— 9 位数字，对应 3×3 棋盘各格
#             0=空位  1=黑子  2=白子
#   棋盘角度：[05]          —— 两位数字，0-90 度
#
# 坐标映射：
#   视觉扫描顺序（摄像头视角）→ 物理棋盘顺序（机械臂视角）
#   经过 180° 旋转换算，使输出坐标与机械臂操作坐标一致
#
# 相比树莓派原版的改动：
#   1. 摄像头/串口/GPIO 全部使用 maixpy 原生 API
#   2. 矩形检测使用 maix.image.find_rects() 替代 OpenCV 的
#      findContours + approxPolyDP 流程
#   3. 棋子颜色判定使用 LAB 色彩空间 + find_blobs() 替代
#      逐像素 HSV 判断，性能大幅提升
#   4. 串口通讯协议与原版完全一致，不做修改
##########################################################


# ==========================================================
# 全局变量
# ==========================================================

# b: 最终发送给 STM32 的棋子数组（物理棋盘顺序，9 元素）
b = [0] * 9
# a: 中间变量，暂存视觉扫描的原始结果（摄像头视角顺序）
a = [0] * 9
# mode: 状态机标志
#   1 = 待扫描模式（需要重新定位格子坐标）
#   2 = 已锁定模式（格子坐标已缓存，持续检测棋子颜色）
mode = 1


# ==========================================================
# 硬件初始化
# ==========================================================

# --- GPIO 配置（模式选择引脚）---
# STM32 通过此引脚电平控制视觉侧的工作模式：
#   低电平(0)：棋子识别（机械臂放置棋盘后触发）
#   高电平(1)：角度检测（机械臂放置棋盘前触发）
pinmap.set_pin_function("A19", "GPIOA19")    # 将 A19 引脚复用为 GPIO 功能
mode_pin = gpio.GPIO("GPIOA19", gpio.Mode.IN) # 设置为输入模式，读取 STM32 信号

# --- 串口配置 ---
# 使用 UART1（/dev/ttyS1）与 STM32 通信
# 注意：UART0（/dev/ttyS0，引脚 A16/A17）是系统终端口，绝对不能用于通信
#       A16 是 boot 模式检测引脚，开机时被拉低会导致无法启动
serial_dev = uart.UART("/dev/ttyS1", 115200)

# --- 摄像头配置 ---
# 直接以目标分辨率 640×360 初始化
# 棋盘在摄像头下方约 10cm 处，此分辨率下棋盘占画面约 50%
cam = camera.Camera(640, 360)


# ==========================================================
# LAB 色彩空间阈值配置
# ==========================================================
# LAB 色彩空间说明：
#   L 通道：亮度（0=纯黑, 100=纯白）
#   A 通道：绿(-128) ↔ 红(+127) 色调偏移
#   B 通道：蓝(-128) ↔ 黄(+127) 色调偏移
#
# 三子棋棋子颜色对应关系：
#   '0' = 黄色/空位：棋盘底色为黄色，无棋子时采样到黄色
#   '1' = 黑子：低亮度（L 值小），A/B 接近中性
#   '2' = 白子：高亮度（L 值大），A/B 接近中性
#
# 格式：(L_min, L_max, A_min, A_max, B_min, B_max)
#
# ⚠️ 以下阈值为根据原版 HSV 范围推算的初始值，
#    必须在实际环境中使用 MaixPy IDE 的阈值编辑器标定校准。
#    标定方法：分别将黄色底板、黑子、白子放入摄像头视野，
#    逐个调整阈值使 find_blobs 能准确区分三种颜色。
# ==========================================================
lab_thresholds = {
    '0': (50, 100, -20, 30, 50, 127),    # 黄色/空位：高 B 值（偏黄）
    '1': (0, 30, -20, 20, -20, 20),       # 黑色：低亮度
    '2': (80, 100, -15, 15, -15, 15),     # 白色：高亮度、低色度
}


# ==========================================================
# 棋子坐标缓存
# ==========================================================
# or_point: 原始检测到的 9 个格子中心点（未排序）
or_point = [(0, 0)] * 9
# sorted_points: 按空间位置排序后的 9 个中心点（行优先 3×3 顺序）
sorted_points = [(0, 0)] * 9


# ==========================================================
# 工具函数
# ==========================================================

def get_rotation_angle(rect):
    """从 find_rects 返回的矩形对象中提取旋转角度

    原理：
      maix.image.find_rects() 返回的矩形对象有 rotation() 方法，
      返回值为弧度制旋转角度。本函数将其转为角度制，并限制在 0-45° 范围。

    角度调整逻辑：
      1. 弧度 → 角度（round 取整）
      2. 范围限制到 -180° ~ 180°
      3. 进一步限制到 0° ~ 45°（棋盘旋转不会超过 45°，超过则等效于反向旋转）

    参数:
        rect: maix.image.find_rects() 返回的矩形对象

    返回:
        int: 0-45 范围内的旋转角度（度）
    """
    angle_rad = rect.rotation()
    angle = round(math.degrees(angle_rad))

    # 调整角度范围至 -180 至 180
    if angle > 180:
        angle -= 360
    # 调整角度范围至 -45 至 45（棋盘旋转超过 45° 等效于反方向小角度旋转）
    if angle > 45:
        angle = angle - 90
    angle = abs(angle)
    angle = round(angle)
    return angle


def sort_points(points):
    """坐标排序函数：将无序的 9 个中心点排列为 3×3 行优先顺序

    排序算法：
      1. 按 Y 坐标升序排序（从画面顶部到底部）
      2. 每 3 个点为一组，分成三行
      3. 每行内按 X 坐标升序排序（从画面左侧到右侧）
      4. 合并为一维列表

    排序后的顺序：
      [0]=左上  [1]=中上  [2]=右上
      [3]=左中  [4]=正中  [5]=右中
      [6]=左下  [7]=中下  [8]=右下

    参数:
        points: 9 个 (x, y) 坐标的列表

    返回:
        list: 排序后的 9 个坐标
    """
    # 按 y 坐标排序
    points = sorted(points, key=lambda p: p[1])
    # 将排序后的点分成三行，每行三个点
    rows = [points[i:i+3] for i in range(0, len(points), 3)]
    # 对每一行按 x 坐标排序
    sorted_points = [sorted(row, key=lambda p: p[0]) for row in rows]
    # 将排序后的点合并成一个列表
    sorted_points = [point for row in sorted_points for point in row]
    return sorted_points


def find_rect_center(rect):
    """从 find_rects 返回的矩形对象中提取中心点坐标

    原理：
      find_rects() 返回的矩形对象提供 corners() 方法获取四个角点，
      对四个角点坐标取平均值即为中心点。

    参数:
        rect: maix.image.find_rects() 返回的矩形对象

    返回:
        tuple: (center_x, center_y) 中心点坐标
    """
    corners = rect.corners()
    sum_x = sum(c[0] for c in corners)
    sum_y = sum(c[1] for c in corners)
    return (sum_x / len(corners), sum_y / len(corners))


def calc_rect_area(rect):
    """计算 find_rects 返回的矩形对象面积

    原理：
      使用四个角点的向量叉积公式计算任意四边形面积：
      S = |Σ(x_i * y_{i+1} - y_i * x_{i+1})| / 2

    参数:
        rect: maix.image.find_rects() 返回的矩形对象

    返回:
        float: 四边形面积（像素²）
    """
    corners = rect.corners()
    ax, ay = corners[0]
    bx, by = corners[1]
    cx, cy = corners[2]
    dx, dy = corners[3]
    area = abs((ax * by + bx * cy + cx * dy + dx * ay)
               - (ay * bx + by * cx + cy * dx + dy * ax)) / 2.0
    return area


def determine_color_lab(img, center, radius=10):
    """在 LAB 色彩空间中判定指定点周围区域的颜色分类

    工作流程：
      1. 以 center 为中心，裁剪 radius 范围内的矩形区域（ROI）
      2. 将 ROI 从 RGB 转换到 LAB 色彩空间
      3. 对每种预设颜色（黄/黑/白），在 LAB 空间中执行 find_blobs
      4. 统计每种颜色匹配的总像素数
      5. 像素数最多者即为该区域的颜色分类

    为什么用 LAB 而不是 HSV：
      - LAB 是感知均匀的色彩空间，人眼感知到的颜色差异与数值差异一致
      - 对于黄色（棋盘底色）、黑色、白色的区分，LAB 的 B 通道（蓝-黄）
        和 L 通道（亮度）天然适合这类分类
      - find_blobs 在 LAB 空间中批量处理，比逐像素 HSV 转换快得多

    参数:
        img:    maix.image.Image 对象（RGB 格式）
        center: 中心点坐标 (x, y)
        radius: 采样半径（像素），默认 10

    返回:
        str: 颜色分类 '0'(黄/空位) / '1'(黑子) / '2'(白子)
    """
    x, y = center
    # 计算裁剪区域，确保不超出图像边界
    x_start = max(x - radius, 0)
    y_start = max(y - radius, 0)
    w = min(radius * 2 + 1, img.width() - x_start)
    h = min(radius * 2 + 1, img.height() - y_start)

    # 裁剪感兴趣区域（ROI）并转换到 LAB 色彩空间
    roi = img.crop(x_start, y_start, w, h)
    roi_lab = roi.to_lab()

    # 对每种颜色，计算 LAB 区域内匹配的像素总数
    best_name = '1'   # 默认为黑色（防止无匹配时返回空值）
    best_pixels = 0
    for name, thresh in lab_thresholds.items():
        blobs = roi_lab.find_blobs(
            [thresh],
            pixels_threshold=1,   # 最小像素数阈值，设为 1 以捕获所有匹配
            area_threshold=1,     # 最小面积阈值，同上
            merge=False           # 不合并相邻 blob，保持独立计数
        )
        # 累加所有匹配 blob 的像素数
        total_pixels = sum(b.pixels() for b in blobs)
        if total_pixels > best_pixels:
            best_pixels = total_pixels
            best_name = name

    return best_name


def draw_rect_on_image(img, rect, color=(0, 255, 0), thickness=2):
    """在图像上绘制矩形轮廓（调试可视化用）

    由于 find_rects 返回的不是 OpenCV 格式轮廓，
    需要手动连接四个角点来绘制矩形。

    参数:
        img:       maix.image.Image 对象
        rect:      find_rects 返回的矩形对象
        color:     线条颜色 (R, G, B)，默认绿色
        thickness: 线条粗细，默认 2 像素
    """
    corners = rect.corners()
    for i in range(4):
        img.draw_line(corners[i][0], corners[i][1],
                      corners[(i + 1) % 4][0], corners[(i + 1) % 4][1],
                      color=color, thickness=thickness)


# ==========================================================
# 主循环
# ==========================================================
# 整体状态机结构：
#
#   ┌─── GPIO == 0 ─── 棋子识别分支
#   │     ├── mode == 1 → 扫描棋盘格子，锁定 9 个中心点坐标
#   │     │               （仅执行一次，锁定后切换到 mode 2）
#   │     └── mode == 2 → 检测棋子颜色，发送结果（每帧循环执行）
#   │
#   └─── GPIO == 1 ─── 角度检测分支
#         └── 识别棋盘外框，计算偏移角度，发送 [XX]
#             （每帧执行，同时重置 mode = 1，为下次棋子识别做准备）
#
while True:
    if mode_pin.value() == 0:
        # ==================================================
        # GPIO 低电平：棋子识别模式
        # 由 STM32 控制，当棋盘放置到位后拉低此引脚
        # ==================================================

        if mode == 1:
            # --------------------------------------------------
            # 模式 1：扫描棋盘，定位 9 个格子的中心坐标
            # 此模式仅执行一次，锁定坐标后切换到 mode 2
            # --------------------------------------------------
            print("识别9个方块中")

            # 采集一帧图像（maix.camera 返回 maix.image.Image 对象）
            img = cam.read()

            # 使用 maix.py 内置的矩形检测
            # 内部自动完成：灰度化 → 高斯模糊 → Canny 边缘检测 → Hough 变换
            # threshold 参数：2~8，控制检测灵敏度
            #   较小值 → 检测更多矩形但质量较低（易受干扰）
            #   较大值 → 更严格，只检测清晰的矩形
            rects = img.find_rects(threshold=4)

            # 筛选符合条件的矩形（棋盘小格子）
            # 小格子在 640×360 分辨率下的面积范围约 1100~10000 像素²
            rectangles = []
            for r in rects:
                area = calc_rect_area(r)
                if 1100 < area < 10000:
                    rectangles.append(r)

            # 至少检测到 6 个以上的矩形才认为棋盘可见
            # （3×3 = 9 个格子，允许部分被遮挡，但至少要超过一半）
            if len(rectangles) > 5:
                rect_num = 0

                for rect in rectangles:
                    # 计算每个矩形的中心点坐标
                    center_point = find_rect_center(rect)

                    # 调试可视化：绘制矩形轮廓
                    draw_rect_on_image(img, rect)

                    # 坐标取整并存入原始数组
                    center_point = (round(center_point[0]), round(center_point[1]))
                    or_point[rect_num] = center_point
                    rect_num += 1

                # 对 9 个中心点进行空间排序（行优先 3×3 顺序）
                sorted_points = sort_points(or_point)

                # 调试可视化：在每个中心点绘制红点和编号
                for i in range(9):
                    img.draw_circle(sorted_points[i][0], sorted_points[i][1],
                                    5, color=(255, 0, 0), thickness=-1)
                    img.draw_string(sorted_points[i][0], sorted_points[i][1],
                                    str(i + 1), color=(255, 0, 0), scale=2)

                # 坐标锁定完成，切换到棋子颜色检测模式
                mode = 2

        if mode == 2:
            # --------------------------------------------------
            # 模式 2：检测 9 个格子中的棋子颜色
            # 格子坐标已在 mode 1 中锁定，本模式持续循环执行
            # 每帧检测一次并发送结果给 STM32
            # --------------------------------------------------
            point = ''
            print('检测9个方块中的棋子中')

            # 采集新一帧图像（检测当前棋子状态）
            img = cam.read()

            # 逐个格子检测棋子颜色
            for i in range(9):
                center_point = sorted_points[i]

                # 在中心点周围采样，通过 LAB 色彩空间判定颜色
                most_common_color = determine_color_lab(img, center_point)

                # 拼接结果字符串，格式："0,1,2,0,1,0,2,0,1"
                point = point + str(most_common_color)

                if i == 8:
                    point = point
                else:
                    point = point + ','

            # --- 坐标映射：摄像头视角 → 物理棋盘视角 ---
            # 摄像头安装方向与机械臂操作方向相反（旋转 180°）
            # 需要将视觉扫描结果重排为物理棋盘的行列顺序
            #
            # 视觉顺序 (a)：              物理棋盘 (b)：
            # a[0]=左上  a[1]=中上  a[2]=右上
            # a[3]=左中  a[4]=正中  a[5]=右中
            # a[6]=左下  a[7]=中下  a[8]=右下
            #
            # b[0]=a[6]  b[1]=a[3]  b[2]=a[0]    第三行反转 → 第一行
            # b[3]=a[7]  b[4]=a[4]  b[5]=a[1]    第二行反转 → 第二行
            # b[6]=a[8]  b[7]=a[5]  b[8]=a[2]    第一行反转 → 第三行
            a = point.split(',')

            b[0] = a[6]
            b[1] = a[3]
            b[2] = a[0]
            b[3] = a[7]
            b[4] = a[4]
            b[5] = a[1]
            b[6] = a[8]
            b[7] = a[5]
            b[8] = a[2]

            # --- 串口发送棋子状态 ---
            try:
                time.sleep(0.5)
                # 格式化为 STM32 期望的格式：去除空格、引号、逗号
                # 例如 ['0','1','2',...] → "[012010201]"
                uart = str(b)
                uart = uart.replace(' ', '')
                uart = uart.replace("'", '')
                uart = uart.replace(',', '')

                serial_dev.write_str(uart)
                print(uart)
            except Exception as e:
                print(e)

    else:
        # ==================================================
        # GPIO 高电平：棋盘角度检测模式
        # 由 STM32 控制，当机械臂准备放置棋盘时拉高此引脚
        # 检测棋盘外框的整体旋转角度，供机械臂补偿
        # ==================================================
        print("识别棋盘角度中")

        # 每次进入角度模式都重置为 mode 1
        # 确保下次棋子识别时重新扫描格子坐标（棋盘可能被重新放置）
        mode = 1

        # 采集图像2
        img = cam.read()

        # 使用 find_rects 检测矩形
        rects = img.find_rects(threshold=4)

        # 筛选大矩形（整个棋盘外框）
        # 棋盘外框在 640×360 分辨率下的面积 > 33000 像素²
        rectangles = []
        for r in rects:
            area = calc_rect_area(r)
            if area > 33000:
                rectangles.append(r)

        # 仅当恰好检测到 1 个大矩形时才输出角度
        # （避免多个大矩形干扰导致误判）
        if len(rectangles) == 1:
            for rect in rectangles:
                # 计算旋转角度
                angle = get_rotation_angle(rect)
                center_point = find_rect_center(rect)
                time.sleep(0.3)
                try:
                    # 格式化角度为两位数字符串
                    # 例如 5° → "[05]"，25° → "[25]"
                    if angle < 10:
                        uart = '[0' + str(angle) + ']'
                    else:
                        uart = '[' + str(angle) + ']'
                    uart = str(uart)

                    # 通过串口发送角度值给 STM32
                    serial_dev.write_str(uart)
                    print(uart)
                except Exception as e:
                    print(e)

                # 调试可视化：绘制检测到的棋盘外框
                draw_rect_on_image(img, rect)

                rect_num += 1
                center_point = (round(center_point[0]), round(center_point[1]))
                img.draw_string(center_point[0], center_point[1],
                                str(rect_num), color=(255, 0, 0), scale=1)
