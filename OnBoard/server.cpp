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
#include "lktracking.h"
#include "yuv2hsl.h"
#include "gimbal.h"

#define SIMPLEM
#define TIMEEVALUATION
using namespace std;

const int MaxSendLength=65535;
const int MaxRecLength=25;
const int ListenPort=9031,SendPort=9032;

//压缩后的图像数据指针
const void *imgCompressData;
//压缩后的图像长度
int imgLength;

//控制指令指针
char* controlData;
//控制指令的长度
int controlLength=0;


//UDP发送接收缓冲区
char RecBuf[MaxRecLength];
char SendBuf[MaxSendLength];

//数据发送长度
int SendLength;

itr_system::Udp _udp(ListenPort,false);
itr_system::Udp::UdpPackage udpPackage;
itr_system::AsyncBuffer<Picture*> yuvBuffer;
itr_system::AsyncBuffer<F32*> matBuffer;
itr_system::AsyncBuffer<U8*> trackBuffer;
itr_system::AsyncBuffer<U8*> compressBuffer;
itr_system::SerialPort uart;
bool uartOK=false;

itr_protocol::StandSerialProtocol sspUdp;


itr_math::RectangleF targetPos;
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
U8 mode=0;


///SSP接受数据，进行命令解析

void SSPReceivefuc(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS,U8 *Package,S32 PackageLength)
{
    F32 *a,*b,*c,*d;
    switch(Package[0])
    {
        case 0x41:
        mode=Package[1];
        if(mode!=2)
        {
            GimbalStop(&controlData,controlLength);
            if(uartOK)
                uart.Send((unsigned char*)controlData,controlLength);
        }
        break;
        case 0x42:
        MemoryCopy(Package,Package+1,16);
        a=(F32*)(Package);
        b=(F32*)(Package+4);
        c=(F32*)(Package+8);
        d=(F32*)(Package+12);
        GimbalUpdatePID(*a,*b,*c,*d);
        break;
        case 0x44:
        config.color=Package[1];
        break;
        case 0x43:
        config.fps=Package[1];
        config.pixel=Package[2];
        break;
        case 0x45:
        MemoryCopy(Package,Package+1,8);
        a=(F32*)(Package);
        b=(F32*)(Package+4);
        targetPos.Width=*a;
        targetPos.Height=*b;
        break;
    //直接转发
        default:
        if(uartOK)
            uart.Send((U8*)SSFS,SSP->GetSSFSLength(SSFS));
        break;
    }
}



//准备要发送的数据
S32 SSPSend(U8* Buffer,S32 Length)
{
    memcpy(SendBuf,Buffer,Length);

    U8* img=compressBuffer.GetBufferToRead();
    while(img==NULL)
    {
         img=compressBuffer.GetBufferToRead();
    }
    memcpy(SendBuf+Length,(U8*)(img+4),*((int*)img));
    SendLength=Length+*((int*)img);

    compressBuffer.SetBufferToWrite(img);
    return SendLength;
}

//初始化参数
void Init()
{
    config.color=0;
    config.pixel=0;
    config.fps=30;

    // udpPackage.IP="255.255.255.255";
    udpPackage.IP="192.168.199.159";
    udpPackage.port=SendPort;

    itr_math::MathObjStandInit();

    sspUdp.Init(0xA5 ,0x5A ,SSPSend);//串口发送函数 代替 NULL
    sspUdp.ProcessFunction[0]=&SSPReceivefuc;

    uartOK=(uart.Init("/dev/ttyUSB0",115200)==0);

    GimbalInit();

    _width=Width[config.pixel];
    _height=Height[config.pixel];
    _size=_width*_height;

    targetPos.Width=40;
    targetPos.Height=40;
    yuvBuffer.Init(2);
    yuvBuffer.AddBufferToList(new Picture);
    yuvBuffer.AddBufferToList(new Picture);
    matBuffer.Init(2);
    matBuffer.AddBufferToList(new F32[2*_size]);
    matBuffer.AddBufferToList(new F32[2*_size]);
    trackBuffer.Init(2);
    trackBuffer.AddBufferToList(new U8[20]);
    trackBuffer.AddBufferToList(new U8[20]);
    compressBuffer.Init(2);
    compressBuffer.AddBufferToList(new U8[2*_size]);
    compressBuffer.AddBufferToList(new U8[2*_size]);
}

void* x264_thread(void* name)
{
 //   printf("x264 start !\n");
    void *encoder = vc_open(_width, _height, config.fps);
    if (!encoder)
    {
        fprintf(stderr, "ERR: can't open x264 encoder\n");
        exit(-1);
    }
    Picture *pic;
    U8* _imgcomp;
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
        // pthread_mutex_lock(&mutexCompress);


        while (_imgcomp==NULL)
        {
            usleep(10);
            _imgcomp=compressBuffer.GetBufferToWrite();
        }
        tc.Tick();
        int rc = vc_compress(encoder, pic->data, pic->stride,&imgCompressData , & imgLength);  //前两位是压缩后长度

        yuvBuffer.SetBufferToWrite(pic);
        if (rc < 0)
        {
            compressBuffer.SetBufferToWrite(_imgcomp);
            continue;
        }

        *(int*)_imgcomp=imgLength;
        memcpy(_imgcomp+4,imgCompressData,imgLength);

        compressBuffer.SetBufferToRead(_imgcomp);

    //    printf("Compress OK at time=%d\n",tc.Tick());
    }

    vc_close(encoder);
}

void* camera_thread(void *name)
{
    printf("camera start !\n");
    void *capture = capture_open("/dev/video0", _width, _height, PIX_FMT_YUV420P);
    printf("camera opened !\n");
    if (!capture)
    {
        fprintf(stderr, "ERR: can't open '/dev/video0'\n");
        exit(-1);
    }

    Picture *pic;
    ///用于yuv到hls转换
    yuv2hsl yuv2hsl_obj;
    F32* img_g;
    TimeClock tc;
    tc.Tick();
    while(1)
    {
        if (mode==0)
        {
            usleep(10);
            continue;
        }
        tc.Tick();
        pic=yuvBuffer.GetBufferToWrite();
        if (pic==NULL)
        {
            usleep(10);
            continue;
        }
        capture_get_picture(capture, pic);
//printf("New Img OK at time=%d\n",tc.Tick());
        img_g=matBuffer.GetBufferToWrite();
        while(img_g==NULL)
        {
	    usleep(10);
            img_g=matBuffer.GetBufferToWrite();
        }

        // yuv2hsl_obj.doyuv2hsl(_width,_height,pic->data[0],pic->data[1],pic->data[2],
        //   img_hs,img_hs+_size);
        for (int i = 0; i < _size; ++i)
        {
            img_g[i]=pic->data[0][i];
        }
        yuvBuffer.SetBufferToRead(pic);
        matBuffer.SetBufferToRead(img_g);
    //    printf("New Img YUV to HSL =%d\n",tc.Tick());
    }

    capture_close(capture);
}

void* track_thread(void* name)
{

    F32* img_g;
    U8* tempbuff;
    F32 _vx=0,_vy=0;
    lktracking* tracker=NULL;
    U8 offset=0;
    bool inited=false;
    TimeClock tc;
    tc.Tick();
    while(1)
    {
       // printf("track start !\n");
        img_g=matBuffer.GetBufferToRead();
        if (img_g==NULL)
        {
            continue;
        }
      //  printf("mode int track\n",mode);
        if(mode==2)
        {
            tempbuff=trackBuffer.GetBufferToWrite();
            if(tempbuff==NULL)
            {
                matBuffer.SetBufferToRead(img_g);
                continue;
            }
            tc.Tick();
            Matrix img(_height,_width,img_g);
            if(!inited)
            {
                tracker=new lktracking;
                targetPos.X=(_width-targetPos.Width)*0.5;
                targetPos.Y=(_height-targetPos.Height)*0.5;
                tracker->Init(img,targetPos);
                inited=true;
                printf("Init: X:%f\tY:%f\n",targetPos.X,targetPos.Y);
            }
            else
            {
                if(tracker->Go(img,targetPos,_vx,_vy))  // Pos information are in the targetPos, _vx and _vy are speeds.
                {   //fps,x,y,Area
                    offset=0;
                    fps=1000/tc.Tick();
                    memcpy(tempbuff,&fps,4);
                    x=targetPos.X+targetPos.Width*0.5;
                    memcpy(tempbuff,&x,4);
                    y=targetPos.Y+targetPos.Height*0.5;
                    memcpy(tempbuff,&y,4);
                    Area=targetPos.Width*targetPos.Height;
                    memcpy(tempbuff,&Area,4);
                    printf("track:%f\t%f\t%f\n",x,y,Area);
                }
                else
                {
                    continue;   //TODO: to be reconsidered.
                }
            }
            printf("Track OK, at time=%f\n", 1000/fps);
        }
        else
        {
            if(tracker!=NULL)
            {
                delete tracker;
                tracker=NULL;
            }
        }
        matBuffer.SetBufferToWrite(img_g);
        trackBuffer.SetBufferToRead(tempbuff);
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

    pthread_create(&tidcam, NULL, camera_thread, (void *)( "Camera" ));
    pthread_create(&tidx264, NULL, x264_thread, (void *)( "x264" ));
    pthread_create(&tidtrack, NULL, track_thread, (void *)( "Track" ));

    int offset=0;
    for (; ; )
    {
        //printf("main start !\n" );
        if(_udp.Receive(RecBuf,MaxRecLength))
        {
            //使用SSP进行解包
            sspUdp.ProcessRawByte((U8 *)RecBuf,MaxRecLength);
        }
        if (mode==0)
        {
            usleep(10);
            continue;
        }

        //用SSP封装，包括图像和跟踪结果
        SendLength=0;
        offset=0;
        memset(tempbuff,0,sizeof(tempbuff));
        tempbuff[offset++]=0x40;
        tempbuff[offset++]=mode;
        if(mode==2)
        {
            U8* tracktemp=trackBuffer.GetBufferToRead();

            if(tracktemp!=NULL)
            {
                MemoryCopy(tempbuff+offset,(void*)tracktemp,16);
                trackBuffer.SetBufferToWrite(tracktemp);
            }
        }
        offset+=16;
        sspUdp.SSPSendPackage(0,tempbuff,offset);
            // 发送结果
        udpPackage.pbuffer=SendBuf;
        udpPackage.len=SendLength;
        _udp.Send(udpPackage);

        //printf("Send OK at time=%d\n",tc.Tick());
    }

    return 0;
}

