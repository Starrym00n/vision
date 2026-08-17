from maix import camera, time

##########################################################
# 测试 5：棋盘矩形检测验证
# 验证目标：能否正确检测到 3×3 棋盘的 9 个格子并计算中心点
# 运行方式：MaixVision 直接加载运行
# 前置条件：将空棋盘（无棋子）放在摄像头下方约 10cm 处
#
# 通过标准：
#   1. 检测到 ≥6 个矩形（棋盘至少 6 个格子可见）
#   2. 矩形面积在 1100~10000 范围内（小格子）
#   3. 排序后的 9 个中心点顺序正确（行优先）
#   4. 连续 10 帧检测结果稳定（中心点波动 < 5 像素）
#
# 如果失败：
#   - 检测到 0 个矩形 → 调整 find_rects 的 threshold 参数
#   - 检测到太多矩形 → 缩小面积范围或增大 threshold
#   - 中心点顺序错误 → 检查棋盘放置方向
#   - 帧间波动大 → 检查摄像头固定是否牢固
##########################################################

import math

print("=== 棋盘矩形检测测试 ===")
print("请将空棋盘放在摄像头下方")
print("")

# 初始化
cam = camera.Camera(640, 360)


def find_rect_center(rect):
    """计算矩形中心点（四角点均值）"""
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


def sort_points(points):
    """将无序点排列为 3x3 行优先顺序"""
    points = sorted(points, key=lambda p: p[1])
    rows = [points[i:i+3] for i in range(0, len(points), 3)]
    sorted_points = [sorted(row, key=lambda p: p[0]) for row in rows]
    return [point for row in sorted_points for point in row]


# 连续采集 10 帧进行稳定性测试
all_frames_centers = []
test_count = 10

for frame_i in range(test_count):
    img = cam.read()

    # 检测所有矩形
    rects = img.find_rects(threshold=4)

    # 筛选小格子（面积 1100~10000）
    small_rects = []
    for r in rects:
        area = calc_rect_area(r)
        if 1100 < area < 10000:
            small_rects.append((r, area))

    # 筛选大矩形（面积 > 33000，整个棋盘外框）
    big_rects = []
    for r in rects:
        area = calc_rect_area(r)
        if area > 33000:
            big_rects.append((r, area))

    # 按面积排序，方便观察
    small_rects.sort(key=lambda x: x[1])

    print("--- 帧 {} ---".format(frame_i + 1))
    print("  find_rects 总检测数: {}".format(len(rects)))
    print("  小格子 (1100<area<10000): {} 个".format(len(small_rects)))
    print("  大矩形 (area>33000): {} 个".format(len(big_rects)))

    # 打印每个小格子的面积
    for i, (r, area) in enumerate(small_rects):
        center = find_rect_center(r)
        print("    格子 {}: 面积={:.0f}, 中心=({:.0f},{:.0f})".format(
            i, area, center[0], center[1]))

    # 如果检测到足够多的格子，进行排序测试
    if len(small_rects) >= 6:
        centers = [find_rect_center(r) for r, _ in small_rects[:9]]
        sorted_centers = sort_points(centers)
        all_frames_centers.append(sorted_centers)

        print("  排序后 9 个中心点：")
        for row in range(3):
            pts = sorted_centers[row * 3:(row + 1) * 3]
            print("    行{}: ({:.0f},{:.0f}) ({:.0f},{:.0f}) ({:.0f},{:.0f})".format(
                row, pts[0][0], pts[0][1], pts[1][0], pts[1][1], pts[2][0], pts[2][1]))
    else:
        print("  ⚠️ 检测到的格子不足 6 个，无法进行排序测试")
        all_frames_centers.append(None)

    # 在图像上绘制检测结果
    for r, area in small_rects:
        corners = r.corners()
        for i in range(4):
            img.draw_line(corners[i][0], corners[i][1],
                          corners[(i + 1) % 4][0], corners[(i + 1) % 4][1],
                          color=(0, 255, 0), thickness=2)

    time.sleep(0.5)

# --- 稳定性分析 ---
print("")
print("=== 稳定性分析 ===")
valid_frames = [c for c in all_frames_centers if c is not None]
if len(valid_frames) >= 2:
    # 比较相邻帧的中心点波动
    max_drift = 0
    for i in range(1, len(valid_frames)):
        for j in range(9):
            dx = abs(valid_frames[i][j][0] - valid_frames[i - 1][j][0])
            dy = abs(valid_frames[i][j][1] - valid_frames[i - 1][j][1])
            drift = max(dx, dy)
            if drift > max_drift:
                max_drift = drift

    print("相邻帧最大中心点偏移: {:.1f} 像素".format(max_drift))
    if max_drift < 5:
        print("✅ 稳定性良好（偏移 < 5 像素）")
    else:
        print("⚠️ 稳定性不足（偏移 >= 5 像素），请检查摄像头固定")
else:
    print("有效帧数不足，无法分析稳定性")

print("")
print("=== 测试完成 ===")
