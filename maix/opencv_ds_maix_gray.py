import math
from maix import camera, uart, gpio, pinmap, time

##########################################################
# 三子棋游戏装置 —— 视觉识别主程序（相对灰度版）
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
# 颜色识别方案（相对灰度）：
#   将 ROI 区域转为灰度图，计算区域平均灰度值，
#   通过两道门限将亮度轴分为三个区间：
#     低灰度 → 黑子    中灰度 → 空位（黄色底板）    高灰度 → 白子
#
# 相比 LAB 版的改动：
#   1. 不依赖 to_lab() / find_blobs()，无需 LAB 色彩空间支持
#   2. 只需 maix.image.to_grayscale() 或内置灰度转换
#   3. 算法更简单，对固件兼容性要求更低
#   4. 新增参数调节说明，便于现场标定
##########################################################


# ==========================================================
# 全局变量
# ==========================================================
b = [0] * 9
a = [0] * 9
# mode: 状态机标志
#   1 = 待扫描模式（需要重新定位格子坐标）
#   2 = 已锁定模式（格子坐标已缓存，持续检测棋子颜色）
mode = 1


# ==========================================================
# 灰度阈值配置（核心参数，需现场标定）
# ==========================================================
#
#   原理说明：
#     摄像头采集 RGB 图像 → 转为灰度（0~255）→ 计算 ROI 平均灰度
#     通过两道门限划分三个区间：
#
#     灰度值分布：
#     0 ──────────┤──────────┤──────────┤────────── 255
#                  ↑          ↑          ↑
#               dark_thr   mid_thr    （辅助区间）
#              黑子区      空位区      白子区
#
#     平均灰度 < dark_thr  → '1' 黑子（棋子吸光，灰度低）
#     dark_thr ≤ 平均灰度 < mid_thr → '0' 空位（黄色底板居中）
#     平均灰度 ≥ mid_thr  → '2' 白子（棋子反光，灰度高）
#
#   调节方法：
#     1. 先放黑色棋子在棋盘上，运行 test_gray_ref.py
#        记录打印出的平均灰度值 → 以此为基准设定 dark_thr
#     2. 再放白色棋子，记录平均灰度值 → 以此为基准设定 mid_thr
#     3. 最后确认空位（黄色底板）的灰度值落在 dark_thr ~ mid_thr 之间
#     4. 三个区间之间留 10~20 的余量防止边界抖动
#
# ==========================================================
dark_thr = 80     # 黑子与空位的分界线（低于此值判为黑子）
mid_thr = 180     # 空位与白子的分界线（高于此值判为白子）
# ==========================================================


# ==========================================================
# 硬件初始化
# ==========================================================
pinmap.set_pin_function("A19", "GPIOA19")
mode_pin = gpio.GPIO("GPIOA19", gpio.Mode.IN)

serial_dev = uart.UART("/dev/ttyS1", 115200)

# 摄像头：640×360 分辨率
cam = camera.Camera(640, 360)


# ==========================================================
# 棋子坐标缓存
# ==========================================================
or_point = [(0, 0)] * 9
sorted_points = [(0, 0)] * 9


# ==========================================================
# 工具函数
# ==========================================================

def get_rotation_angle(rect):
    """从 find_rects 返回的矩形对象中提取旋转角度（0-45°）"""
    angle_rad = rect.rotation()
    angle = round(math.degrees(angle_rad))

    if angle > 180:
        angle -= 360
    if angle > 45:
        angle = angle - 90
    angle = abs(angle)
    angle = round(angle)
    return angle


def sort_points(points):
    """将无序的 9 个中心点排列为 3×3 行优先顺序"""
    points = sorted(points, key=lambda p: p[1])
    rows = [points[i:i+3] for i in range(0, len(points), 3)]
    sorted_points = [sorted(row, key=lambda p: p[0]) for row in rows]
    return [point for row in sorted_points for point in row]


def find_rect_center(rect):
    """从 find_rects 返回的矩形对象中提取中心点坐标"""
    corners = rect.corners()
    sum_x = sum(c[0] for c in corners)
    sum_y = sum(c[1] for c in corners)
    return (sum_x / len(corners), sum_y / len(corners))


def calc_rect_area(rect):
    """向量叉积计算四边形面积"""
    corners = rect.corners()
    ax, ay = corners[0]
    bx, by = corners[1]
    cx, cy = corners[2]
    dx, dy = corners[3]
    area = abs((ax * by + bx * cy + cx * dy + dx * ay)
               - (ay * bx + by * cx + cy * dx + dy * ax)) / 2.0
    return area


def determine_color_gray(img, center, radius=10):
    """通过相对灰度判定指定点周围区域的颜色分类

    工作流程：
      1. 以 center 为中心，裁剪 radius 范围内的矩形区域（ROI）
      2. 将 ROI 转为灰度图
      3. 计算灰度图的平均值
      4. 根据 dark_thr / mid_thr 两道门限判定颜色

    判定规则：
      平均灰度 < dark_thr  → '1' 黑子（棋子吸光，灰度低）
      dark_thr ≤ 平均灰度 < mid_thr → '0' 空位（黄色底板居中）
      平均灰度 ≥ mid_thr  → '2' 白子（棋子反光，灰度高）

    参数:
        img:    maix.image.Image 对象
        center: 中心点坐标 (x, y)
        radius: 采样半径（像素），默认 10

    返回:
        tuple: (颜色分类字符串, 平均灰度值)
    """
    x, y = center
    # 计算裁剪区域，确保不超出图像边界
    x_start = max(x - radius, 0)
    y_start = max(y - radius, 0)
    w = min(radius * 2 + 1, img.width() - x_start)
    h = min(radius * 2 + 1, img.height() - y_start)

    # 裁剪 ROI 并转为灰度
    roi = img.crop(x_start, y_start, w, h)
    roi_gray = roi.to_grayscale()

    # 计算 ROI 平均灰度值
    # 方法：对灰度 ROI 执行 find_blobs 找最大区域，
    #       或直接用 pixel() 逐像素累加求平均
    # 这里使用更轻量的方式：取 ROI 中心点周围若干采样点的灰度平均值
    total = 0
    count = 0
    # 在 ROI 中均匀采样（每 2 像素取一个点，避免全量遍历过慢）
    for sy in range(0, roi_gray.height(), 2):
        for sx in range(0, roi_gray.width(), 2):
            pixel = roi_gray.pixel(sx, sy)
            # to_grayscale() 返回的 pixel() 可能是整数或元组
            if isinstance(pixel, (list, tuple)):
                total += pixel[0]  # 取第一个通道
            else:
                total += pixel
            count += 1

    if count == 0:
        return '1', 0  # 防止空区域

    avg_gray = total // count

    # 根据灰度门限判定颜色
    if avg_gray < dark_thr:
        return '1', avg_gray    # 黑子
    elif avg_gray < mid_thr:
        return '0', avg_gray    # 空位（黄色底板）
    else:
        return '2', avg_gray    # 白子


def draw_rect_on_image(img, rect, color=(0, 255, 0), thickness=2):
    """在图像上绘制矩形轮廓（调试可视化用）"""
    corners = rect.corners()
    for i in range(4):
        img.draw_line(corners[i][0], corners[i][1],
                      corners[(i + 1) % 4][0], corners[(i + 1) % 4][1],
                      color=color, thickness=thickness)


# ==========================================================
# 主循环
# ==========================================================
while True:
    if mode_pin.value() == 0:
        # ---- GPIO 低电平：棋子识别模式 ----

        if mode == 1:
            # ---- 模式 1：扫描棋盘格子中心坐标 ----
            print("识别9个方块中")

            img = cam.read()

            # 矩形检测
            rects = img.find_rects(threshold=4)

            # 筛选小格子（面积 1100~10000 像素²）
            rectangles = []
            for r in rects:
                area = calc_rect_area(r)
                if 1100 < area < 10000:
                    rectangles.append(r)

            if len(rectangles) > 5:
                rect_num = 0
                for rect in rectangles:
                    center_point = find_rect_center(rect)
                    draw_rect_on_image(img, rect)

                    center_point = (round(center_point[0]), round(center_point[1]))
                    or_point[rect_num] = center_point
                    rect_num += 1

                sorted_points = sort_points(or_point)

                for i in range(9):
                    img.draw_circle(sorted_points[i][0], sorted_points[i][1],
                                    5, color=(255, 0, 0), thickness=-1)
                    img.draw_string(sorted_points[i][0], sorted_points[i][1],
                                    str(i + 1), color=(255, 0, 0), scale=2)

                mode = 2

        if mode == 2:
            # ---- 模式 2：检测 9 个格子的棋子颜色 ----
            point = ''
            print('检测9个方块中的棋子中')

            img = cam.read()

            for i in range(9):
                center_point = sorted_points[i]

                # 通过相对灰度判定颜色（返回颜色和灰度值）
                color, gray_val = determine_color_gray(img, center_point)

                # 打印每个格子的灰度值，便于调试
                print("  格子{}: 灰度={}, 判定={}".format(i + 1, gray_val, color))

                point = point + str(color)

                if i == 8:
                    point = point
                else:
                    point = point + ','

            a = point.split(',')

            # 坐标映射：摄像头视角 → 物理棋盘视角（180° 旋转）
            b[0] = a[6]
            b[1] = a[3]
            b[2] = a[0]
            b[3] = a[7]
            b[4] = a[4]
            b[5] = a[1]
            b[6] = a[8]
            b[7] = a[5]
            b[8] = a[2]

            # 串口发送棋子状态
            try:
                time.sleep(0.5)
                uart = str(b)
                uart = uart.replace(' ', '')
                uart = uart.replace("'", '')
                uart = uart.replace(',', '')

                serial_dev.write_str(uart)
                print(uart)
            except Exception as e:
                print(e)

    else:
        # ---- GPIO 高电平：棋盘角度检测模式 ----
        print("识别棋盘角度中")
        mode = 1

        img = cam.read()
        rects = img.find_rects(threshold=4)

        # 筛选大矩形（棋盘外框，面积 > 33000）
        rectangles = []
        for r in rects:
            area = calc_rect_area(r)
            if area > 33000:
                rectangles.append(r)

        if len(rectangles) == 1:
            for rect in rectangles:
                angle = get_rotation_angle(rect)
                center_point = find_rect_center(rect)
                time.sleep(0.3)
                try:
                    if angle < 10:
                        uart = '[0' + str(angle) + ']'
                    else:
                        uart = '[' + str(angle) + ']'
                    uart = str(uart)

                    serial_dev.write_str(uart)
                    print(uart)
                except Exception as e:
                    print(e)

                draw_rect_on_image(img, rect)

                rect_num += 1
                center_point = (round(center_point[0]), round(center_point[1]))
                img.draw_string(center_point[0], center_point[1],
                                str(rect_num), color=(255, 0, 0), scale=1)
