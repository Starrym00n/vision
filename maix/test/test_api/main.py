from maix import camera, time

##########################################################
# 测试 4：MaixPy 核心 API 兼容性验证
# 验证目标：find_rects / to_lab / find_blobs 三个核心 API 是否可用
# 运行方式：MaixVision 直接加载运行
#
# 通过标准：
#   1. find_rects() 不报错，返回列表（即使为空也 OK）
#   2. to_lab() 不报错，返回图像对象
#   3. find_blobs() 不报错，返回列表
#
# 如果某项 API 报错：
#   - AttributeError → 当前固件不支持此 API，需升级固件
#   - ImportError → 检查 maix 模块是否正确安装
#   - 其他错误 → 记录错误信息，可能需要调整参数
##########################################################

print("=== MaixPy API 兼容性测试 ===")

# 初始化摄像头
print("初始化摄像头...")
cam = camera.Camera(640, 360)
print("摄像头 OK")

# 采集一帧图像
img = cam.read()
print("图像采集 OK")

# --- 测试 1: find_rects ---
print("")
print("--- 测试 find_rects ---")
try:
    rects = img.find_rects(threshold=4)
    print("find_rects OK, 检测到 {} 个矩形".format(len(rects)))
    for i, r in enumerate(rects):
        corners = r.corners()
        print("  矩形 {}: 角点={}".format(i, corners))
except AttributeError as e:
    print("find_rects 失败: {} → 固件可能不支持此 API".format(e))
except Exception as e:
    print("find_rects 异常: {}".format(e))

# --- 测试 2: to_lab ---
print("")
print("--- 测试 to_lab ---")
try:
    img_lab = img.to_lab()
    print("to_lab OK, 返回类型: {}".format(type(img_lab)))
    # 尝试获取 LAB 图像的基本信息
    try:
        print("  LAB 图像尺寸: {}x{}".format(img_lab.width(), img_lab.height()))
    except:
        print("  (无法获取尺寸，但转换成功)")
except AttributeError as e:
    print("to_lab 失败: {} → 固件可能不支持此 API".format(e))
except Exception as e:
    print("to_lab 异常: {}".format(e))

# --- 测试 3: find_blobs ---
print("")
print("--- 测试 find_blobs ---")
try:
    # 使用一个宽泛的 LAB 阈值进行测试
    test_thresh = (0, 100, -128, 127, -128, 127)
    img_lab = img.to_lab()
    blobs = img_lab.find_blobs(
        [test_thresh],
        pixels_threshold=100,
        area_threshold=100,
        merge=True
    )
    print("find_blobs OK, 检测到 {} 个 blob".format(len(blobs)))
    for i, b in enumerate(blobs[:5]):  # 最多打印 5 个
        print("  blob {}: center=({},{}), pixels={}".format(
            i, b.cx(), b.cy(), b.pixels()))
    if len(blobs) > 5:
        print("  ... 还有 {} 个 blob".format(len(blobs) - 5))
except AttributeError as e:
    print("find_blobs 失败: {} → 固件可能不支持此 API".format(e))
except Exception as e:
    print("find_blobs 异常: {}".format(e))

# --- 测试 4: crop ---
print("")
print("--- 测试 crop ---")
try:
    roi = img.crop(100, 100, 50, 50)
    print("crop OK, 返回类型: {}".format(type(roi)))
except AttributeError as e:
    print("crop 失败: {} → 固件可能不支持此 API".format(e))
except Exception as e:
    print("crop 异常: {}".format(e))

# --- 测试 5: draw 系列 ---
print("")
print("--- 测试 draw API ---")
try:
    img.draw_line(0, 0, 100, 100, color=(255, 0, 0), thickness=2)
    img.draw_circle(320, 180, 30, color=(0, 255, 0), thickness=2)
    img.draw_string(10, 10, "API Test", color=(255, 255, 0), scale=2)
    print("draw_line / draw_circle / draw_string 全部 OK")
except Exception as e:
    print("draw 异常: {}".format(e))

print("")
print("=== API 兼容性测试完成 ===")
print("请确认以上各项均为 OK，如有失败项需先解决再进行后续测试")
