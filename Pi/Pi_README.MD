#  ![logo](https://ts3.cn.mm.bing.net/th?id=ODLS.4cf2c2ab-ae19-4b91-84ac-824df7926ffe&w=32&h=32&qlt=90&pcl=fffffa&r=0&o=6&pid=1.2)树莓派端代码の使用说明

- 该代码实现了识别棋盘进行追踪定位各个区块中心点坐标并进行排序,
- 通过串口返回检测各个区块的棋子颜色，坐标位置，棋盘角度
- 通过GPIO跳线选择串口的实时输出内容（角度或棋盘棋子情况）
- 支持树莓派开机自启，支持无屏幕终端运行

## 1.摄像头の配置
```python
# 树莓派5调用摄像头获取视频流的py代码，其他设备请自行修改
picam2 = Picamera2()
config = picam2.create_preview_configuration(main={"format":'RGB888',"size":(1280,720)})
picam2.configure(config)
picam2.start()
img = picam2.capture_array()
```
摄像头使用csi接口直接连接到树莓派主板上，无需其他配置
## 2.串口の配置

```python
serialPort = serial.Serial("/dev/ttyAMA0", 9600)
```
该行代码配置了使用树莓派5的引脚GPIO14、GPIO15作为串口输出与stm32进行通信，请按照实际情况自行修改

## 3.识别模式の选择
树莓派运行时有棋盘的角度识别和棋子颜色识别两种模式，请根据实际运行情况使用stm32控制电平进行选择
```python
# 设置 GPIO 模式
GPIO.setmode(GPIO.BCM)
# 定义 GPIO 端口
GPIO_PORT = 18  # 设置 GPIO 18作为模式选择引脚
# 设置 GPIO 方向为输入
GPIO.setup(GPIO_PORT, GPIO.IN)
```
当树莓派的GPIO18引脚为高电平时，树莓派运行棋盘角度识别模式，当为低电平时，运行棋子颜色识别模式

## 3.设置开机自启
在Linux系统中有很多的开机自启方式，如：`systemd`、`upstart`、`rc.local`等，我在此使用了配置`systemctl`的方式进行自启，systemctl是一个systemd工具，主要负责控制systemd系统和服务管理器。
systemd是一个系统管理守护进程、工具和库的集合，用于取代System V初始进程。Systemd的功能是用于集中管理和配置类UNIX系统。在Linux生态系统中，Systemd被部署到了大多数的标准Linux发行版中，只有为数不多的几个发行版尚未部署。

```shell
nano /etc/systemd/system/opencv_app.service
```

```shell
[Unit]
#服务描述，写有意义的内容，便于识别
Description=opencv_app service

[Service]
Type=simple
#设置应用的工作目录请根据自己文件存放位置修改
WorkingDirectory=/home/pi/opencv
ExecStart=/bin/python3 /home/pi/opencv/main.py
ExecStop=/bin/kill -s TERM $MAINPID

[Install]
WantedBy=multi-user.target
```
- 如果/etc/systemd/system/opencv_app.service有过修改，需要进行更新，执行下面这个命令
`systemctl daemon-reload`
- 查看服务是否被正确识别
`systemctl list-unit-files|grep opencv_app`


- 使用`chmod +x /home/pi/opencv/main.py` 启动脚本设置执行权限


- 启动系统服务`systemctl start opencv_app.service`
- 停止系统服务`systemctl stop opencv_app.service`
- 查看系统服务是否成功启动`systemctl status opencv_app.service -l`
