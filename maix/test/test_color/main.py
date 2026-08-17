from maix import camera, time

##########################################################
# 测试 6：LAB 颜色识别验证
# 验证目标：能否通过 LAB 色彩空间正确区分黑子、白子和空位（黄色底板）
# 运行方式：MaixVision 直接加载运行
# 前置条件：准备黑子、白子各一枚，黄色棋盘底板
#
# 测试步骤：
#   Phase 1 - 单色标定：
#     a. 只放黄色底板（无棋子）→ 应全部判定为 '0'
#     b. 放一枚黑子 → 该位置应判定为 '1'
#     c. 放一枚白子 → 该位置应判定为 '2'
#
#   Phase 2 - 混合识别：
#     在不同位置放置黑子和白子，验证分类准确率
#
# 通过标准：
#   1. 空位（黄色底板）识别为 '0'，准确率 > 95%
#   2. 黑子识别为 '1'，准确率 > 95%
#   3. 白子识别为 '2'，准确率 > 95%
#
# 如果失败：
#   - 某种颜色识别率低 → 调整 lab_thresholds 中对应阈值
#   - 黑白混淆 → 加大 L 通道的区分范围
#   - 黄色和白色混淆 → 调整 B 通道（蓝-黄轴）
#
# ⚠️ 标定方法：
#   先单独测试每种颜色，记录 LAB 空间中的实际值范围，
#   再更新 lab_thresholds 字典中的阈值。
##########################################################

print("=== LAB 颜色识别测试 ===")
print("请准备：黄色棋盘底板、黑子、白子")
print("")

# 初始化
cam = camera.Camera(640, 360)

# ==========================================================
# LAB 阈值配置（与主程序一致）
# 格式：(L_min, L_max, A_min, A_max, B_min, B_max)
# ==========================================================
lab_thresholds = {
    '0': (50, 100, -20, 30, 50, 127),    # 黄色/空位：高 B 值（偏黄）
    '1': (0, 30, -20, 20, -20, 20),       # 黑色：低亮度
    '2': (80, 100, -15, 15, -15, 15),     # 白色：高亮度、低色度
}


def determine_color_lab(img, center, radius=10):
    """在 LAB 色彩空间中判定指定点周围区域的颜色分类"""
    x, y = center
    x_start = max(x - radius, 0)
    y_start = max(y - radius, 0)
    w = min(radius * 2 + 1, img.width() - x_start)
    h = min(radius * 2 + 1, img.height() - y_start)

    roi = img.crop(x_start, y_start, w, h)
    roi_lab = roi.to_lab()

    best_name = '1'
    best_pixels = 0
    for name, thresh in lab_thresholds.items():
        blobs = roi_lab.find_blobs(
            [thresh],
            pixels_threshold=1,
            area_threshold=1,
            merge=False
        )
        total_pixels = sum(b.pixels() for b in blobs)
        if total_pixels > best_pixels:
            best_pixels = total_pixels
            best_name = name

    return best_name, best_pixels


# ==========================================================
# Phase 1：单色标定测试
# ==========================================================
print("=" * 40)
print("Phase 1: 单色标定测试")
print("=" * 40)
print("")

# 在画面中心设置 5 个测试采样点
test_points = [
    (220, 130),  # 左上区域
    (420, 130),  # 右上区域
    (320, 180),  # 正中央
    (220, 230),  # 左下区域
    (420, 230),  # 右下区域
]

print("请依次执行以下操作：")
print("")

# --- 测试 1a: 纯黄色底板 ---
print("--- 1a: 只放黄色底板（无棋子）---")
input("    放好后按回车继续...") if False else None
# 无 input 环境下直接采集
time.sleep(2)

results_0 = []
for _ in range(3):
    img = cam.read()
    for pt in test_points:
        color, pixels = determine_color_lab(img, pt)
        results_0.append(color)
    time.sleep(0.3)

count_correct = results_0.count('0')
total = len(results_0)
print("    采样 {} 次，结果: {}".format(total, results_0))
print("    '0'(黄色) 出现 {} 次 / {} 次，准确率 {:.0%}".format(
    count_correct, total, count_correct / total if total > 0 else 0))

# --- 测试 1b: 黑子 ---
print("")
print("--- 1b: 在中央放置一枚黑子 ---")
print("    请将黑子放在画面正中央，等待 2 秒...")
time.sleep(2)

results_1 = []
for _ in range(3):
    img = cam.read()
    # 只检测中心点（黑子所在位置）
    color, pixels = determine_color_lab(img, test_points[2])
    results_1.append(color)
    # 同时检测周围点（应仍为黄色底板）
    for pt in [test_points[0], test_points[4]]:
        c_around, p_around = determine_color_lab(img, pt)
    time.sleep(0.3)

count_correct = results_1.count('1')
total = len(results_1)
print("    中心点采样 {} 次，结果: {}".format(total, results_1))
print("    '1'(黑子) 出现 {} 次 / {} 次，准确率 {:.0%}".format(
    count_correct, total, count_correct / total if total > 0 else 0))

# --- 测试 1c: 白子 ---
print("")
print("--- 1c: 在中央放置一枚白子 ---")
print("    请将白子放在画面正中央，等待 2 秒...")
time.sleep(2)

results_2 = []
for _ in range(3):
    img = cam.read()
    color, pixels = determine_color_lab(img, test_points[2])
    results_2.append(color)
    time.sleep(0.3)

count_correct = results_2.count('2')
total = len(results_2)
print("    中心点采样 {} 次，结果: {}".format(total, results_2))
print("    '2'(白子) 出现 {} 次 / {} 次，准确率 {:.0%}".format(
    count_correct, total, count_correct / total if total > 0 else 0))

# ==========================================================
# Phase 2：混合识别测试
# ==========================================================
print("")
print("=" * 40)
print("Phase 2: 混合识别测试")
print("=" * 40)
print("")
print("请在棋盘上放置若干黑子和白子，空出部分格子")
print("等待 3 秒开始采集...")
time.sleep(3)

print("")
print("连续采集 5 帧，每帧检测 9 个位置：")
print("")

for frame_i in range(5):
    img = cam.read()
    frame_result = ""
    for pt in test_points:
        color, pixels = determine_color_lab(img, pt)
        frame_result += color + " "
    print("  帧 {}: [{}]".format(frame_i + 1, frame_result.strip()))
    time.sleep(0.5)

# --- 阈值微调指南 ---
print("")
print("=" * 40)
print("阈值微调指南")
print("=" * 40)
print("如果识别不准确，按以下步骤调整 lab_thresholds：")
print("")
print("1. 在代码顶部修改 lab_thresholds 字典")
print("2. LAB 阈值格式: (L_min, L_max, A_min, A_max, B_min, B_max)")
print("")
print("   常见调整场景：")
print("   - 黑子被误判为其他颜色 → 降低 '1' 的 L_max")
print("   - 白子被误判为黄色 → 提高 '2' 的 L_min")
print("   - 黄色底板被误判为空 → 调整 '0' 的 B_min")
print("   - 黑白混淆 → 加大 '1' 和 '2' 的 L 通道间距")
print("")
print("=== 测试完成 ===")
