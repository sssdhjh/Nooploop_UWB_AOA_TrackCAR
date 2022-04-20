# Nooploop_UWB_AOA_TrackCAR
《空循环Nooploop》的Linktrack AOA测距模块的研究运用，实现基于飞思卡尔智能车平台的自动跟随

<!-- Brief introduction ->
本项目以飞思卡尔智能车(STM32F103VET6)为实验平台,空循环的超宽带产品LinkTrack_aoa测距模块( Tag and anchor）做数据的采集，ROS为上位机对数据处理、判断、通过ZigBee通信模块串口发送控制给STM下位机，STM下位机响应上位机的控制指令，做出相应的底层运动。实现标签跟随。

<!-- Prerequisites -->
- [Robot Operating System (ROS)](http://wiki.ros.org) (middleware for robotics)
  
    运行本程序需要ROS支持，如还未安装请先安装ROS。

- [Serial Library](https://github.com/nooploop-dev/serial.git)

    使用前还需要下载官方的串口驱动,按下面的步骤编译:
    cd ~/catkin_workspace/src
    git clone https://github.com/nooploop-dev/serial.git
    make
    make test
    sudo make install


    本程序和硬件设备之间采用串口通信，请先点击安装该串口通信库，注意，如果是第一次使用串口设备，通常需要开启串口操作权限，详情参考[Fix serial port permission denied errors on Linux](https://websistent.com/fix-serial-port-permission-denied-errors-linux/)
	
 先连接上模块,查看USB设备:
   
    ls -l /dev/ttyUSB*
	
 如检测到USB0,给串口权限(重启后权限无效,需重新执行):
 
    sudo chmod 777 /dev/ttyUSB0
   
 永久打开串口权限,重启后仍然生效:
   
    whoami  #查看自己的<username>用户名
    sudo usermod -a -G dialout <username>  #将当前用户加入到dialout用户组，使其具有操作ttyUSB0的权限



<!-- preparation in advance -->
根据空循环的官网AOA的README.md文件，要实现标签与基站的通信，必须先下载串口通信库：

按如下步骤克隆代码并编译

    cd catkin_workspace/src
    git clone --recursive https://github.com/nooploop-dev/nlink_parser.git 
    cd ../
    catkin_make
    source devel/setup.bash

注意，每次打开新命令行窗口都需要执行 `source devel/setup.bash` 重新获取该ROS工作空间环境变量.

<!-- LinkTrack AOA -->


   roscore
   catkin_make
   source ./devel/setup.bash
   
<!-- ----------------------------------------------   发布方运行  ----------------------------------------------- -->

    rosrun nlink_parser linktrack_aoa

参数
   - **`port_name`** 设备串行端口名称，默认值: `/dev/ttyUSB0`.
   - **`baud_rate`** 设备波特率，默认值: `921600`.
  
订阅的话题

* **`/nlink_linktrack_data_transmission`** ([std_msgs::String])

	你可以通过对该话题发布消息，将数据发送给LinkTrack节点，进而利用数传功能

发布的话题

  - **`/nlink_linktrack_nodeframe0`** ([nlink_parser::LinktrackNodeframe0])
  - **`/nlink_linktrack_aoa_nodeframe0`** ([nlink_parser::LinktrackAoaNodeframe0])


<!-- ----------------------------------------------   订阅方运行  ----------------------------------------------- -->

   rosrun nlink_parser track_aoa_sub

参数
   - **`port_name`** ZigBee端口名称，默认值: `/dev/ttyUSB1`.   <!-- 根据实际情况修改设备名称 -->
   - **`baud_rate`** 设备波特率，默认值: `115200`.
 

订阅的话题

* **`nlink_linktrack_aoa_nodeframe0"`** ((const char [31]))

  获取标签与基站的实时参数,可通过串口通信发送出去


   
   



