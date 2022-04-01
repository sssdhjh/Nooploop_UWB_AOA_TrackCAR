#include "ros/ros.h"
#include <serial/serial.h>
#include "std_msgs/String.h"
#include <nlink_parser/LinktrackAoaNodeframe0.h>
#include <serial/serial.h>
#include <iostream>
using namespace std;


#define N 5
float buffer[N];
int  i = 0;   //必须是全局变量，否则不会流水线处理数据
float get_dis,aver;
serial::Serial ser;
std::string data, state, result;
char ch;


//http://blog.sina.com.cn/s/blog_53a7da550102y772.html  标准键盘码值表-十六进制

const unsigned char header[2] = {0x4C,0x4F};   //L、O
const unsigned char ender[2] = {0x56, 0x45};    //V、E


union sendData    //共用体，用于十六进制的转化
{
	short d;    //2个字节，16个位
	unsigned char data[2];   //2个字节  16个位
}dis_union;


/********************************************************
函数功能：通信协议，将数据包装成数据，一整帧的发送
入口参数：平均速度， 预留控制指令
出口参数：无
********************************************************/
void writedata(float aver,unsigned char ctrlFlag)
{
    unsigned char buf[8] = {0};  //unsigned char在计算机中使用补码存储
    int i, length = 0;

    dis_union.d=aver;    //short后是个整数
    cout<<"存到buf的dis值:"<<dis_union.d<<endl;
    // angle_union.d=angle;
    // cout<<"存到buf的angle值:"<<angle_union.d<<endl;


   // 设置消息头
    for(i = 0; i < 2; i++){
        buf[i] = header[i];             //buf[0]  buf[1]
    }

     // 设置速度和角度
    length = 3;
    buf[2] = length;                    //buf[2]

    for ( i = 0; i < 1; i++)
    {
        buf[i+3]=dis_union.data[i];  //buf[3]
    }
    // 预留控制指令
    buf[2 + length - 1] = ctrlFlag;       //buf[4]
    
    //CRC		校验和从有效数据开始取异或即可
    buf[2 + length] =  buf[1]^ buf[2]^ buf[3]^ buf[4];

    //消息尾
    buf[2 + length + 1] = ender[0];     //buf[6]
    buf[2 + length + 2] = ender[1];     //buf[7]

    //打印数组
    int count =sizeof(buf)/sizeof(unsigned char);
    for (int i = 0; i < count; i++)
    {
        cout<<buf[i]<<endl;
    }
        ser.write(buf,count);
        ser.flush();
        ROS_INFO("data send successful");
}


/********************************************************
函数功能：流水线均值滤波处理
入口参数：采集到的距离信息
出口参数：平均值
********************************************************/
float filter(float *dis_data)
{

    int count;
    float  sum = 0;

    buffer[i]=*dis_data;   
    i++;
    if ( i == N )
    {
        i=0;
    }

    for ( count=0; count<N; count++)
   {
      sum += buffer[count];
   }

   aver=(sum/N);

    //打印
    //cout<<"函数滤波处理sum:"<<aver<<endl;

   return aver;

}




/********************************************************
函数功能：设置和检验串口
入口参数：无
出口参数：无
********************************************************/
void serial_port() //启动串口
{
    try
    {
        ser.setPort("/dev/ttyUSB1"); 
        ser.setBaudrate(115200);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000); 
        ser.setTimeout(to);
        ser.open();
    }
    catch (serial::IOException &e)
    {
        ROS_ERROR_STREAM("Unable to open port ");
        return;
    }

     //检测串口是否已经打开，并给出提示信息 
    if(ser.isOpen())
    {
        ROS_INFO_STREAM("Serial Port initialized"); 
    }

    else{
        return;
    }

}


/********************************************************
函数功能：判断标签与基站的实时距离，并发送控制字符
入口参数：滤波处理后的平均值
出口参数：char 字符
********************************************************/
char guess(float average)  //判断距离
{

    if(average>=1.0){
        ch='j';
        state="前进";
        cout << " the data write to serial is : " << ch << endl;
        cout << " the state of robot is :" << state.c_str() << endl<<endl;
        ROS_INFO("均值滤波处理后的距离：%.2f\n",average);
        return ch;    
    }

    else{
        ch='s';
        state="停止";
        cout << " the data write to serial is : " <<  ch << endl;
        cout << " the state of robot is :" << state.c_str() << endl<<endl;
        ROS_INFO("均值滤波处理后的距离：%.2f\n",average);
        ROS_INFO("------------------------------------------------");
        return ch;
    }

}



/********************************************************
函数功能：回调函数，集成处理，循环执行
入口参数：发布方的消息文件
出口参数：无
********************************************************/
void callback(const nlink_parser::LinktrackAoaNodeframe0::ConstPtr &msg)  //回调处理
{
    ROS_INFO("标签订阅到的消息:角色：%d,id值:%d,供电电压：%.2f",msg->role,msg->id,msg->voltage);

    if(msg->nodes.size()>0)   //必须判断，nodes可能为空
    {
        // cout<<"未处理的距离"<<msg->nodes[0].dis<<endl;
        // cout<<"size:"<<msg->nodes.size()<<endl;

        float d=msg->nodes[0].dis;
        // float angle=msg->nodes[0].angle;

        get_dis=filter(&d);
        cout<<"滤波后的距离"<< get_dis<<endl;

        ch = guess(get_dis);
        writedata(get_dis,ch);  //参数个数可调
        // ser.write("a");

        cout<<endl;

        
    }

    else if(msg->nodes.size()==0)
    {
        ROS_WARN("Error!Please Connect the Anchor!");
    }

    else{
        return;
    }

}

int main(int argc, char  *argv[])
{
    setlocale(LC_ALL,"");
    ros::init(argc,argv,"distance");
    ros::NodeHandle nh;
    serial_port();
    ros::Subscriber sub = nh.subscribe("nlink_linktrack_aoa_nodeframe0",200,callback);
    ros::spin();
    ser.close();
    return 0;
}