import cv2
import numpy as np
from maix import camera, uart, gpio, pinmap, time

##########################################################
# 该代码实现了识别棋盘进行追踪定位各个区块中心点坐标并进行排序,
# 通过串口返回检测各个区块的棋子情况，坐标位置，棋盘角度
# 通过GPIO引脚选择串口的实时输出内容
# 适用于 MaixCAM Pro / MaixCam2 平台（MaixPy v4）
##########################################################

b = [0]*9
a = [0]*9
mode = 1

# 设置 GPIO 引脚（模式选择）
pinmap.set_pin_function("A19", "GPIOA19")  # 将 A19 设为 GPIO 功能
mode_pin = gpio.GPIO("GPIOA19", gpio.Mode.IN)  # 设置为输入模式

serial_dev = uart.UART("/dev/ttyS1", 115200)  # 使用 UART1 与 STM32 通信，115200 波特率

# 摄像头配置，直接使用 640x360 分辨率
cam = camera.Camera(640, 360)

color_ranges = [
    {'name': '0', 'lower': np.array([20, 100, 100]), 'upper': np.array([30, 255, 255])},
    {'name': '2', 'lower': np.array([0, 0, 180]), 'upper': np.array([180, 50, 255])},
    {'name': '1', 'lower': np.array([0, 0, 0]), 'upper': np.array([180, 255, 80])}
]
# 预设的色域范围 (HSV空间)

or_point = [(0,0)]*9
sorted_points = [(0,0)]*9
point = '['


def get_rotation_angle(contour):
    # 使用 minAreaRect 获取最小面积的旋转矩形
    rect = cv2.minAreaRect(contour)
    center, (width, height), angle = rect

    # 调整角度范围至 -180 至 180
    if angle > 180:
        angle -= 360
    # 调整角度范围至 -45 至 45
    if angle > 45:
        angle = angle -90
    angle = abs(angle)
    angle = round(angle)
    return angle

def sort_points(points):#坐标排序函数
    # 按y坐标排序
    points = sorted(points, key=lambda p: p[1])
    # 将排序后的点分成三行，每行三个点
    rows = [points[i:i+3] for i in range(0, len(points), 3)]
    # 对每一行按x坐标排序
    sorted_points = [sorted(row, key=lambda p: p[0]) for row in rows]
    # 将排序后的点合并成一个列表
    sorted_points = [point for row in sorted_points for point in row]
    return sorted_points

def find_center(coordinates):
    # 将坐标从三维数组转换为二维数组
    coordinates_2d = coordinates.squeeze()

    # 计算横坐标和纵坐标的总和
    sum_x = np.sum(coordinates_2d[:, 0])
    sum_y = np.sum(coordinates_2d[:, 1])

    # 计算点的数量
    num_points = len(coordinates_2d)

    # 计算横坐标和纵坐标的平均值
    center_x = sum_x / num_points
    center_y = sum_y / num_points

    # 返回中心点坐标
    return (center_x, center_y)

# 获取指定点周围的颜色（参数）
def get_colors_around_point(image, center, radius=10):
    # 计算边界
    x, y = center
    x_start, x_end = max(x - radius, 0), min(x + radius + 1, image.shape[1])
    y_start, y_end = max(y - radius, 0), min(y + radius + 1, image.shape[0])

    # 提取区域
    region = image[y_start:y_end, x_start:x_end]

    # 获取颜色
    colors = []
    for i in range(region.shape[0]):
        for j in range(region.shape[1]):
            color = region[i, j]
            colors.append(color)

    return colors, region

def determine_color_domain(colors, color_ranges):
    # 初始化计数器
    color_counts = {color['name']: 0 for color in color_ranges}

    # 遍历每个颜色，将其与预设的色域进行比较
    for color in colors:
        # 将BGR颜色转换为HSV颜色
        hsv_color = cv2.cvtColor(np.uint8([[color]]), cv2.COLOR_BGR2HSV)[0][0]

        # 将单个像素颜色包装成二维数组
        hsv_color_2d = np.array([[hsv_color]], dtype=np.uint8)

        for color_range in color_ranges:
            # 使用cv2.inRange比较颜色与色域
            if cv2.inRange(hsv_color_2d, color_range['lower'], color_range['upper']):
                color_counts[color_range['name']] += 1

    # 找到出现次数最多的颜色
    most_common_color = max(color_counts, key=color_counts.get)
    return most_common_color



while True:
    if mode_pin.value() == 0:
        # 默认0信号时运行
        if mode == 1:
            print("识别9个方块中")
            # MaixCAM: cam.read() 返回 maix.image.Image，需转为 numpy BGR 格式供 OpenCV 处理
            img = cam.read()
            img = img.to_numpy()               # RGB 格式的 numpy 数组
            img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)  # 转为 BGR 以匹配 OpenCV 处理流程

            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)  # 转换为灰度图像
            blur = cv2.GaussianBlur(gray, (5, 5), 0)  # 高斯模糊去噪
            edges = cv2.Canny(blur, 0, 80)  # Canny边缘检测

            # 进行闭运算填补轮廓中的空洞
            kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))
            closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)

            # 查找所有轮廓
            contours, _ = cv2.findContours(closed, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)

            rectangles = []


            for cnt in contours:
                # 近似轮廓
                approx = cv2.approxPolyDP(cnt, 0.05 * cv2.arcLength(cnt, True), True)

                # 如果近似轮廓有四个顶点，认为它是矩形（这里设置面积阈值）
                if len(approx) == 4 and cv2.contourArea(approx) > 1100 and cv2.contourArea(approx) < 10000:
                    rectangles.append(approx)


            if(len(rectangles) > 5):
                # 如果检测到矩形数量大于5，则输出矩形顶点坐标
                rect_num = 0

                for rect in rectangles:
                    # 输出每个矩形的顶点坐标
                    center_point = find_center(rect)
                    # 在原图上绘制矩形
                    cv2.drawContours(img, [rect], -1, (0, 255, 0), 2)
                    # 在原图上绘制矩形的序号和中心点

                    center_point = (round(center_point[0]), round(center_point[1]))# 坐标取整数
                    or_point[rect_num] = center_point
                    rect_num += 1

                sorted_points = sort_points(or_point) # 对坐标进行排序
                for i in range(9):
                    cv2.circle(img, sorted_points[i], 5, (0, 0, 255), -1)
                    center_point = sorted_points[i]
                    cv2.putText(img, str(i+1), center_point, cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 0, 255), 2)

                mode = 2

        if mode == 2:
            point = ''
            print('检测9个方块中的棋子中')
            # MaixCAM: 同上，转换为 BGR 格式
            img = cam.read()
            img = img.to_numpy()
            img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)

            # 指定中心点
            for i in range(9):
                center_point = sorted_points[i]

                # 获取周围像素的颜色
                colors, region = get_colors_around_point(img, center_point)

                # 判断颜色属于哪个色域
                most_common_color = determine_color_domain(colors, color_ranges)

                # 打印结果
                point = point + str(most_common_color)

                if i == 8:
                    point = point
                else:
                    point = point + ','

            a = point.split(',')
            # print(point)

            b[0] = a[6]
            b[1] = a[3]
            b[2] = a[0]
            b[3] = a[7]
            b[4] = a[4]
            b[5] = a[1]
            b[6] = a[8]
            b[7] = a[5]
            b[8] = a[2]


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
        # 默认1信号时停止，运行棋盘角度检测
        print("识别棋盘角度中")
        mode = 1
        # MaixCAM: 同上，转换为 BGR 格式
        img = cam.read()
        img = img.to_numpy()
        img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)

        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)  # 转换为灰度图像
        blur = cv2.GaussianBlur(gray, (5, 5), 0)  # 高斯模糊去噪
        edges = cv2.Canny(blur, 0, 80)  # Canny边缘检测

        # 进行闭运算填补轮廓中的空洞
        kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))
        closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)

        # 查找所有轮廓
        contours, _ = cv2.findContours(closed, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)

        rectangles = []

        for cnt in contours:
            # 近似轮廓
            approx = cv2.approxPolyDP(cnt, 0.05 * cv2.arcLength(cnt, True), True)

            # 如果近似轮廓有四个顶点，认为它是矩形（这里设置面积阈值）
            if len(approx) == 4 and cv2.contourArea(approx) > 33000 :
                rectangles.append(approx)


        if(len(rectangles) == 1): # 如果检测到矩形数量等于1，则输出矩形顶点坐标
            rect_num = 0
            for rect in rectangles:
                # 输出每个矩形的顶点坐标
                angle = get_rotation_angle(rect)
                center_point = find_center(rect)
                time.sleep(0.3)
                try:
                    if angle < 10:
                        uart = '[0' + str(angle) + ']'
                    else:
                        uart = '[' + str(angle) + ']'
                    uart = str(uart)

                    # 发送数据
                    serial_dev.write_str(uart)
                    print(uart)
                except Exception as e:
                    print(e)
                # 在原图上绘制矩形
                cv2.drawContours(img, [rect], -1, (0, 255, 0), 2)
                # 在原图上绘制矩形的序号和中心点
                rect_num += 1

                center_point = (round(center_point[0]), round(center_point[1]))# 坐标取整数
                cv2.putText(img, str(rect_num), center_point, cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)
