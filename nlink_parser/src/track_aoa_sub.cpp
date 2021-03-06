#include "ros/ros.h"
#include <serial/serial.h>
#include "std_msgs/String.h"
#include <nlink_parser/LinktrackAoaNodeframe0.h>
#include <serial/serial.h>
#include <iostream>
using namespace std;


#define N 5
float buffer[N];
int  i = 0;
float get_dis,get_angle,get_fp_rssi,aver;
serial::Serial ser;
std::string data, state, result;

int serial_write(serial::Serial &ser, std::string &serial_msg)  //写操作
{
    ser.write(serial_msg); 
    return 0;
}

int serial_read(serial::Serial &ser, std::string &result)   //读操作
{
    result = ser.read(ser.available());  
    return 0;
}

float filter(float *dis_data)  //均值滤波处理
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
//    cout<<"函数滤波处理sum:"<<aver<<endl;

   return aver;

}

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

void guess(float average,float angle,float fp_rssi)  //判断距离
{

    if(angle>=20)
    {
        data = "-20";
        state = "向左";
        serial_write(ser, data);
        ser.flush();  //等待串口数据发送结束
        ser.flushInput();  //清除输入缓冲区数据
    }
    else if(angle<-20){
        data = "20";
        state = "向右";
        serial_write(ser, data);
        ser.flush();  
        ser.flushInput();
    }
    else{
        data = "0";
        state = "复位";
        serial_write(ser, data);
        ser.flush(); 
        ser.flushInput();
    }

  //先进行信号强度的判断，信号正常
    if(fp_rssi>=-87.0)      //信号大于-87db
    {
        if(average>=2.0){
            data="j";
            state="前进";
            serial_write(ser, data);
            ser.flush(); 
            ser.flushInput();

            cout << " the data write to serial is : " <<  data.c_str() << endl;
            cout << " the state of robot is :" << state.c_str() << endl<<endl;
            ROS_INFO("均值滤波处理后的距离：%.2f\n",average);
            ROS_INFO("------------------------------------------------");
        }
        
       else if(average<0.3){
            data="k";
            state="后腿";
            serial_write(ser, data);
            ser.flush(); 
            ser.flushInput();

            cout << " the data write to serial is : " <<  data.c_str() << endl;
            cout << " the state of robot is :" << state.c_str() << endl<<endl;
            ROS_INFO("均值滤波处理后的距离：%.2f\n",average);
            ROS_INFO("------------------------------------------------");      
        }

        else{
            data="s";
            state="停止";
            serial_write(ser, data);
            ser.flush();  //等待串口数据发送结束
            ser.flushInput();

            cout << " the data write to serial is : " <<  data.c_str() << endl;
            cout << " the state of robot is :" << state.c_str() << endl<<endl;
            ROS_INFO("均值滤波处理后的距离：%.2f\n",average);
            ROS_INFO("------------------------------------------------");
         }
    }
    
    //信号衰弱
    if(fp_rssi<=-87.0)  //信号小于-87db
    {
            data="s";
            state="停止";
            serial_write(ser, data);
            ser.flush();  //等待串口数据发送结束
            ser.flushInput();

            ROS_WARN("警告！信号源不在控制信号范围内！");
            ROS_INFO("------------------------------------------------");
      }
}

void callback(const nlink_parser::LinktrackAoaNodeframe0::ConstPtr &msg)  //回调处理
{
    ROS_INFO("标签订阅到的消息:角色：%d,id值:%d,供电电压：%.2f",msg->role,msg->id,msg->voltage);

    if(msg->nodes.size()>0)
    {
        // cout<<"未处理的距离"<<msg->nodes[0].dis<<endl;
        //cout<<"size:"<<msg->nodes.size()<<endl;

        float d=msg->nodes[0].dis;
        get_angle=msg->nodes[0].angle;

        get_dis=filter(&d);
        cout<<"滤波后的距离"<< get_dis<<endl;


        cout<<endl;
        
        /*
            情况：当小车背对着标签时，当距离增大，小车会越来越远。
            分析：因距离为正值，没有负值，很难利用距离的关系来判断车身的方向
            解决：可以利用模块的信号强度 fp_rssi rx_rssi，当信号强度过低时停止
        */

        get_fp_rssi=msg->nodes[0].fp_rssi;
        ROS_INFO("信号强度：%.2f",get_fp_rssi);

        guess(get_dis,get_angle,get_fp_rssi);   //判断方位
        
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
