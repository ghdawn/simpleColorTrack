#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "capture.h"
#include "vcompress.h"

#include "Udp.h"
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
const int Color_tracked=0;
const void *imgCompressData;
int imgLength;
char RecBuf[MaxRecLength];
char SendBuf[MaxSendLength];
int SendLength;
Communication::Udp _udp(ListenPort,false);
Communication::Udp::UdpPackage udpPackage;
itr_protocol::StandSerialProtocol sspUdp;
SerialPort sportobj;
ColorTrack tracker;

Config config;

U8 mode=2;
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

S32 SSPSend(U8* Buffer,S32 Length)
{
    memcpy(SendBuf,Buffer,Length);
    for (int i = 0; i < Length; ++i)
    {
        printf("%X ",Buffer[i]);
    }
    printf("\n");
    memcpy(SendBuf+Length,(void*)imgCompressData,imgLength);
    SendLength=Length+imgLength;
    return SendLength;
}


void Init()
{
    config.color=0;
    config.result=0;
    config.pixel=0;
    config.fps=30;

    udpPackage.IP="192.168.0.116";
    // udpPackage.IP="0.0.0.0";
    udpPackage.port=SendPort;

    itr_math::MathObjStandInit();

    sspUdp.Init(0xA5 ,0x5A ,SSPSend);//串口发送函数 代替 NULL
    sspUdp.ProcessFunction[0]=&SSPReceivefuc;

    GimbalInit();
    // sportobj.Init("/dev/ttyUSB0",115200);
}


int main (int argc, char **argv)
{
    Init();
    U8 tempbuff[100];
    char* ControlData;
    int controllength=0;
    F32 fps,x,y,Area;
    unsigned int _width=Width[config.pixel],_height=Height[config.pixel];
    unsigned int _size=_width*_height;
#ifdef  SIMPLEM
    unsigned int x_ever=0,y_ever=0,color_counter=0;
#endif // SIMPLEM
    TimeClock clock;
    yuv2hsl yuv2hsl_obj;///用于yuv到hls转换



    void *capture = capture_open("/dev/video0", _width, _height, PIX_FMT_YUV420P);

    U8 *img_hs=new U8[2*_size];
    itr_math::Matrix mat_H(_height,_width),mat_S(_height,_width);

    if (!capture)
    {
        fprintf(stderr, "ERR: can't open '/dev/video0'\n");
        exit(-1);
    }

    void *encoder = vc_open(_width, _height, config.fps);
    if (!encoder)
    {
        fprintf(stderr, "ERR: can't open x264 encoder\n");
        exit(-1);
    }
#ifdef TIMEEVALUATION
    static float timeyuv2hsl=0,timex264=0,timetrack=0,timeholy=0;
#endif // TIMEEVALUATION
    // int tosleep = 1000000 / VIDEO_FPS;
    for (; ; )
    {
#ifdef TIMEEVALUATION
    timeyuv2hsl=0,timex264=0,timetrack=0,timeholy=0;
#endif // TIMEEVALUATION
        if(_udp.Receive(RecBuf,MaxRecLength))
        {
            //使用SSP进行解包
            sspUdp.ProcessRawByte((U8 *)RecBuf,MaxRecLength);
        }
        if (mode==0)
        {
            continue;
        }
        clock.Tick();
        //获取图像，进行RGB，HSL的转换
        Picture pic;
        capture_get_picture(capture, &pic);
        yuv2hsl_obj.doyuv2hsl(_width,_height,pic.data[0],pic.data[1],pic.data[2],
                              img_hs,img_hs+_size);
#ifdef TIMEEVALUATION
        timeyuv2hsl=clock.Tick();
#endif // TIMEEVALUATION
        //将HS转存入矩阵中

#ifdef  SIMPLEM
        x_ever=0,y_ever=0,color_counter=0;
        // int tmpcolor=ColorTable[config.color];
        /*
        BObject.Threshold(H,20,5);
        BObject.Threshold(S,90,50);
        */
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
            x=x_ever;
            y=y_ever;
            Area=color_counter;
            GimbalControl( x, y,&ControlData,controllength);
#ifndef TIMEEVALUATION
            printf("%d %d %d\n",x_ever,y_ever,color_counter);
            printf("Control:%d\n", controllength);
            for (int i = 0; i < controllength; ++i)
            {
                printf("%x ",ControlData[i] );
            }
            printf("\n");
#endif // TIMEEVALUATION

        }

#ifndef TIMEEVALUATION
        else
        printf("0 0 0\n");
#endif // TIMEEVALUATION

#ifdef TIMEEVALUATION
        timetrack=clock.Tick();
#endif // TIMEEVALUATION
#else
        for(int i=0; i<_size; i++)
        {
            mat_H[i]=img_hs[i];
            mat_S[i]=img_hs[i+_size];
        }
        //进行跟踪
        std::vector<itr_vision::Block> blocklist;
        if(mode==2)
        {
            blocklist=tracker.Track(mat_H,mat_S,ColorTable[config.color]);
            x=y=Area=-1;
            if(blocklist.size()>0)
            {
                printf("%f %f %f\n",blocklist[0].x,blocklist[0].y,blocklist[0].Area);
                x=blocklist[0].x;
                y=blocklist[0].y;
                Area=blocklist[0].Area;
                GimbalControl( x, y,&ControlData,controllength);
                printf("Control:%d\n", controllength);
                for (int i = 0; i < controllength; ++i)
                {
                    printf("%x ",ControlData[i] );
                }
                printf("\n");
                // sportobj.Send((unsigned char*)*ControlData,controllength);
            }
        }
#endif // SIMPLEM

        // 压缩图像

        int rc = vc_compress(encoder, pic.data, pic.stride, &imgCompressData, &imgLength);
        if (rc < 0)
        {
            continue;
        }
        fps=1000/clock.Tick();
#ifdef TIMEEVALUATION
        timex264=fps*1000;
#endif // TIMEEVALUATION
        //用SSP封装，包括图像和跟踪结果
        SendLength=0;
        if(config.result==0)
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

        // 发送结果
        udpPackage.pbuffer=SendBuf;
        udpPackage.len=SendLength;
#ifndef TIMEEVALUATION
        printf("Len:%d\n",SendLength );
#else
        timeholy=timeyuv2hsl+timetrack+timex264;
        printf("yuv2hsl:%2f\tratio: %2f\n",timeyuv2hsl,timeyuv2hsl/timeholy*100);
        printf("track  :%2f\tratio: %2f\n",timetrack,timetrack/timeholy*100);
        printf("x264c  :%2f\tratio: %2f\n",timex264,timex264/timeholy*100);
#endif // TIMEEVALUATION
        _udp.Send(udpPackage);

        // 等
        // usleep(tosleep);
    }

    vc_close(encoder);
    capture_close(capture);
    delete[] img_hs;
    return 0;
}

