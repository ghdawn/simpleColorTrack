#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "capture.h"
#include "vcompress.h"

#include "itrsystem.h"
#include "itrdevice.h"
#include "basestruct.h"
#include "colortrack.h"
#include "yuv2hsl.h"
#include "gimbal.h"
#include "serialport.h"

#define SIMPLEM
#define TIMEEVALUATION
using namespace std;

const int MaxSendLength=65535;
const int MaxRecLength=20;
const int ListenPort=9031,SendPort=9032;

//压缩后的图像数据指针
const void *imgCompressData;
//压缩后的图像长度
int imgLength;

//控制指令指针
char* ControlData;
//控制指令的长度
int controllength=0;


//UDP发送接收缓冲区
char RecBuf[MaxRecLength];
char SendBuf[MaxSendLength];

//数据发送长度
int SendLength;

itr_system::Udp _udp(ListenPort,false);
itr_system::Udp::UdpPackage udpPackage;
itr_system::AsyncBuffer<Picture*> yuvBuffer;
itr_system::AsyncBuffer<F32*> matBuffer;
itr_protocol::StandSerialProtocol sspUdp;
ColorTrack tracker;
Config config;

unsigned int _width,_height;
unsigned int _size;

F32 fps,x,y,Area;
/**
* 程序运行模式
* 可能的取值与其对应意义
* 0  停机
* 1  摄像头工作，跟踪不工作
* 2  跟踪工作
*/
U8 mode=2;

pthread_mutex_t mutexTrack,mutexCompress;
bool newImg=false,newResult=false;
///SSP接受数据，进行命令解析
void SSPReceivefuc(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS,U8 *Package,S32 PackageLength)
{
    switch(Package[0])
    {
        case 0x31:
        mode=Package[1];
        break;
        case 0x32:

        break;
        case 0x33:
        config.color=Package[1];
        config.result=Package[2];
        break;
        case 0x34:
        config.fps=Package[1];
        config.pixel=Package[2];
        break;
    //直接转发
        default:
        break;
    }
}

//准备要发送的数据
S32 SSPSend(U8* Buffer,S32 Length)
{
    memcpy(SendBuf,Buffer,Length);
    // for (int i = 0; i < Length; ++i)
    // {
    //     printf("%X ",Buffer[i]);
    // }
    // printf("\n");
    memcpy(SendBuf+Length,(void*)imgCompressData,imgLength);
    SendLength=Length+imgLength;
    return SendLength;
}

//初始化参数
void Init()
{
    config.color=0;
    config.result=0;
    config.pixel=0;
    config.fps=30;

    udpPackage.IP="192.168.0.110";
    // udpPackage.IP="0.0.0.0";
    udpPackage.port=SendPort;

    itr_math::MathObjStandInit();

    sspUdp.Init(0xA5 ,0x5A ,SSPSend);//串口发送函数 代替 NULL
    sspUdp.ProcessFunction[0]=&SSPReceivefuc;

    GimbalInit();

    _width=Width[config.pixel];
    _height=Height[config.pixel];
    _size=_width*_height;

    yuvBuffer.Init(2);
    yuvBuffer.AddBufferToList(new Picture);
    yuvBuffer.AddBufferToList(new Picture);
    matBuffer.Init(2);
    matBuffer.AddBufferToList(new F32[2*_size]);
    matBuffer.AddBufferToList(new F32[2*_size]);
}

void* x264_thread(void* name)
{
    void *encoder = vc_open(_width, _height, config.fps);
    if (!encoder)
    {
        fprintf(stderr, "ERR: can't open x264 encoder\n");
        exit(-1);
    }
    Picture *pic;
    TimeClock tc;
    tc.Tick();
    while(1)
    {
        
        // 压缩图像
        pic=yuvBuffer.GetBufferToRead();
        if (pic==NULL)
        {
            // usleep(10);
            continue;
        }        
        pthread_mutex_lock(&mutexCompress);
        int rc = vc_compress(encoder, pic->data, pic->stride, &imgCompressData, &imgLength);
        if(rc>0)
            newImg=true;
        
        yuvBuffer.SetBufferToWrite(pic);
        if (rc < 0)
        {
            continue;
        }
        pthread_mutex_unlock(&mutexCompress);
        printf("Compress OK at time=%d\n",tc.Tick());
    }

    vc_close(encoder);
}

void* camera_thread(void *name) 
{
    void *capture = capture_open("/dev/video0", _width, _height, PIX_FMT_YUV420P);
    
    if (!capture)
    {
        fprintf(stderr, "ERR: can't open '/dev/video0'\n");
        exit(-1);
    }

    Picture *pic;
    ///用于yuv到hls转换
    yuv2hsl yuv2hsl_obj;
    F32* img_hs;
    TimeClock tc;
    tc.Tick();
    while(1)
    {
        pic=yuvBuffer.GetBufferToWrite();
        if (pic==NULL)
        {
            // usleep(10);
            continue;
        }
        capture_get_picture(capture, pic);
        yuvBuffer.SetBufferTRead(pic);
        img_hs=matBuffer.GetBufferToWrite();    
        while(img_hs==NULL)
        {
            // usleep(10);
            img_hs=matBuffer.GetBufferToWrite();    
        }
        
        yuv2hsl_obj.doyuv2hsl(_width,_height,pic->data[0],pic->data[1],pic->data[2],
          img_hs,img_hs+_size);
        matBuffer.SetBufferTRead(img_hs);
        printf("New Img OK at time=%d\n",tc.Tick());
    }

    capture_close(capture);
}

void* track_thread(void* name)
{
    
    F32* img_hs;
    int x_ever=0,y_ever=0,color_counter=0;
    TimeClock tc;
    tc.Tick();
    while(1)
    {
        img_hs=matBuffer.GetBufferToRead();
        if (img_hs==NULL)
        {
            continue;
        }
        if(mode==2)
        {
            pthread_mutex_lock(&mutexTrack);

            x_ever=0,y_ever=0,color_counter=0;
            unsigned int ind=0;
            for(int i=0; i<_height; i++)
            {
                for(int j=0; j<_width; j++)
                {
                    if(img_hs[ind]>5&&img_hs[ind]<20&&img_hs[ind+_size]<75&&img_hs[ind+_size]>65)
                    {
                        y_ever+=i;
                        x_ever+=j;
                        color_counter++;
                    }
                    ind++;
                }
            }
            if(color_counter)
            {
                x_ever/=color_counter;
                y_ever/=color_counter;
            }
            x=x_ever;
            y=y_ever;
            Area=color_counter;
            fps=1000/tc.Tick();
            GimbalControl( x, y,&ControlData,controllength);
printf("Track OK, at fps=%f\n", fps);
            // newResult=true;
            pthread_mutex_unlock(&mutexTrack);
            
        }
        matBuffer.SetBufferToWrite(img_hs);
    }

}

int main (int argc, char **argv)
{
    Init();

    //新建图像采集线程，图像压缩线程
    pthread_t tidx264,tidcam,tidtrack; 

    U8 tempbuff[100];
    TimeClock tc;
    tc.Tick();
    pthread_mutex_init(&mutexTrack,NULL);
    pthread_mutex_init(&mutexCompress,NULL);
    pthread_create(&tidcam, NULL, camera_thread, (void *)( "Camera" )); 
    pthread_create(&tidx264, NULL, x264_thread, (void *)( "x264" ));
    pthread_create(&tidtrack, NULL, track_thread, (void *)( "Track" ));


    for (; ; )
    {
        if(_udp.Receive(RecBuf,MaxRecLength))
        {
            //使用SSP进行解包
            sspUdp.ProcessRawByte((U8 *)RecBuf,MaxRecLength);
        }
        if (mode==0)
        {
            continue;
        }
        //获取图像，进行RGB，HSL的转换
        //将HS转存入矩阵中

        //用SSP封装，包括图像和跟踪结果
        SendLength=0;
        if(newImg && (config.result==0))
        {
            pthread_mutex_lock(&mutexCompress);
            pthread_mutex_lock(&mutexTrack);
            {
                int offset=0;
                tempbuff[offset++]=0x40;
                tempbuff[offset++]=mode;
                memcpy(tempbuff+offset,(void*)&fps,4);
                offset+=4;
                memcpy(tempbuff+offset,(void*)&x,4);
                offset+=4;
                memcpy(tempbuff+offset,(void*)&y,4);
                offset+=4;
                memcpy(tempbuff+offset,(void*)&Area,4);
                offset+=4;
                sspUdp.SSPSendPackage(0,tempbuff,offset);
            }
            pthread_mutex_unlock(&mutexCompress);
            pthread_mutex_unlock(&mutexTrack);
            // 发送结果
            udpPackage.pbuffer=SendBuf;
            udpPackage.len=SendLength;
            _udp.Send(udpPackage);

            newImg=false;
            newResult=false;
            printf("Send OK at time=%d\n",tc.Tick());

        }
        // usleep(30);

    }    
    
    return 0;
}

