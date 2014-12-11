#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "ix264.h"
#include "itrsystem.h"
#include "itrdevice.h"
#include "basestruct.h"
#include "colortrack.h"
#include "lktracking.h"
#include "gimbal.h"
#include "detection.h"

#define SIMPLEM 0
#define TIMEEVALUATION 0
using namespace std;

const int MaxSendLength=65535;
const int MaxRecLength=25;
const int ListenPort=9031,SendPort=9032;

const int SleepTime=5;
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
itr_system::AsyncBuffer<U8*> yuvBuffer;
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

int cameraID=0,cameraTunnel=0;
F32 fps,x,y,Area;
/**
* 程序运行模式
* 可能的取值与其对应意义
* 0  停机
* 1  摄像头工作，跟踪不工作
* 2  跟踪工作
*/

const U8 IDLE=0;
const U8 CAPTURE=1;
const U8 TRACK=2;
const U8 EXIT=3;
U8 mode=IDLE;


///SSP接受数据，进行命令解析

void SSPReceivefuc(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS,U8 *Package,S32 PackageLength)
{
    F32 *a,*b,*c,*d;
    switch(Package[0])
    {
        case 0x41:
        mode=Package[1];
        if(mode!=TRACK)
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
void Init(int argc, char **argv)
{
    config.color=0;
    config.pixel=0;
    config.fps=30;

    udpPackage.IP=argv[1];
    udpPackage.port=SendPort;

    itr_math::MathObjStandInit();

    sspUdp.Init(0xA5 ,0x5A ,SSPSend);//串口发送函数 代替 NULL
    sspUdp.ProcessFunction[0]=&SSPReceivefuc;

    cameraID=argv[2][0]-'0';
    cameraTunnel=argv[3][0]-'0';
    printf("camera: %d %d\n", cameraID,cameraTunnel);
    if(argc>4)
        uartOK=(uart.Init(argv[argc-1],115200)==0);
    else
        uartOK=false;

    GimbalInit();

    _width=Width[config.pixel];
    _height=Height[config.pixel];
    _size=_width*_height;

    targetPos.Width=40;
    targetPos.Height=40;
    targetPos.X=(_width-targetPos.Width)*0.5;
    targetPos.Y=(_height-targetPos.Height)*0.5;

    yuvBuffer.Init(2);
    yuvBuffer.AddBufferToList(new U8[2*_size]);
    yuvBuffer.AddBufferToList(new U8[2*_size]);
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
    
    itrx264::ix264 compress;
    compress.Open(_width, _height, config.fps);
    
    U8 *data[4];
    S32 stride[4];
    U8* pic;
    U8* _imgcomp;
    TimeClock tc;
    tc.Tick();
    stride[0]=_width;
    stride[1]=_width/2;
    stride[2]=_width/2;
    stride[3]=0;
    while (mode != EXIT)
    {

        // 压缩图像
        pic=yuvBuffer.GetBufferToRead();
        while (pic == NULL)
        {
            usleep(SleepTime);
            pic = yuvBuffer.GetBufferToRead();
        }
        _imgcomp = compressBuffer.GetBufferToWrite();
        while (_imgcomp==NULL)
        {
            usleep(SleepTime);
            _imgcomp=compressBuffer.GetBufferToWrite();
        }
        tc.Tick();
        data[0]=pic;
        data[1]=pic+_size;
        data[2]=pic+_size+_size/4;
        data[3]=NULL;
        int rc = compress.Compress(data, stride,&imgCompressData , &imgLength);  //前两位是压缩后长度

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

    compress.Close();
}

void* camera_thread(void *name)
{
    // printf("camera start !\n");
    // void *capture = capture_open("/dev/video0", _width, _height, PIX_FMT_YUV420P);
    itr_device::v4linux capture;
    capture.Open(cameraID,_width,_height,2);
    capture.SetTunnel(cameraTunnel);
    printf("camera opened !\n");

    U8 *pic;

    F32* img_g;
    TimeClock tc;
    tc.Tick();
    while (mode != EXIT)
    {
        if (mode==IDLE)
        {
            usleep(SleepTime);
            continue;
        }
        tc.Tick();
        pic=yuvBuffer.GetBufferToWrite();
        while (pic == NULL)
        {
            usleep(SleepTime);
            pic = yuvBuffer.GetBufferToWrite();
        }
        // capture_get_picture(capture, pic);
        capture.FetchFrame(pic,2*_size,NULL);
        img_g=matBuffer.GetBufferToWrite();
        while(img_g==NULL)
        {
	        usleep(SleepTime);
            img_g=matBuffer.GetBufferToWrite();
        }

        for (int i = 0; i < _size; ++i)
        {
            img_g[i]=pic[i];
        }
        yuvBuffer.SetBufferToRead(pic);
        matBuffer.SetBufferToRead(img_g);

        printf("New Img\n ");
    }

    capture.Close();
}

void* track_thread(void* name)
{
    F32* img_g;
    U8* tempbuff;
    F32 _vx=0,_vy=0;
    lktracking* tracker=new lktracking;
    Detection* detect=new Detection;
    U8 offset=0;
    bool inited=false;
    
    KalmanFilter kf;
    kf.Init(4);
    F32 data[24]= {1,0,1,0,
                   0,1,0,1,
                   0,0,1,0,
                   0,0,0,1,
                   1,0,0,0,
                   0,1,0,0
                  };
    kf.F_x.CopyFrom(data);
    kf.F_n.SetDiag(1);

    Matrix H(4, 4), Hv(2, 4), Hx(2, 4), Rv(2, 2), Rx(2, 2), Q(4, 4), R(4, 4);
    Rv.SetDiag(0.306);
    Rx.SetDiag(0.3);
    Hv.CopyFrom(data+8);
    Hx.CopyFrom(data+16);
    Q.SetDiag(0.2);
    R.SetDiag(0.2);
    Vector z(2), X(4), n(4), Z(4);

    RectangleS rect(targetPos.X,targetPos.Y,targetPos.Width,targetPos.Height);

FILE* fp;
    TimeClock tc, kalman;
    tc.Tick();
    while (mode != EXIT)
    {

        img_g=matBuffer.GetBufferToRead();
        if (img_g==NULL)
        {
            continue;
        }
      //  printf("mode int track\n",mode);
        if(mode==TRACK)
        {
            tempbuff=trackBuffer.GetBufferToWrite();
            while(tempbuff==NULL)
            {
                usleep(SleepTime);
                tempbuff=trackBuffer.GetBufferToWrite();
            }
            tc.Tick();
            Matrix img(_height,_width,img_g);
            H.SetDiag(0.0);
            if(!inited)
            {
                tracker->Init(img,targetPos);
                detect->Init(img, rect);
                inited=true;
                printf("Init: X:%f\tY:%f\n",targetPos.X,targetPos.Y);
                kf.x[0]=targetPos.X;
                kf.x[1]=targetPos.Y;
                kf.x[2]=0;
                kf.x[3]=0;
                fp=fopen("pos.txt","w");
                kalman.Tick();
            }
            else
            {
                F32 deltaT = kalman.Tick() / 1000.0;
//                printf("deltaT:%f\n",deltaT);
//                kf.F_x(0,2)=kf.F_x(1,3)=deltaT;
                for (int i = 0; i < 2; ++i) {
                    itr_math::NumericalObj->RandGaussian(n[i]);
                    n[i] *= 0.2;
                }
                n[2] = n[0];
                n[3] = n[1];
                kf.F_n(0, 0) = kf.F_n(1, 1) = deltaT * deltaT * 0.5;
                kf.F_n(2, 2) = kf.F_n(3, 3) = deltaT;
                X=kf.UpdateModel(Q,n);
                targetPos.X=X[0];
                targetPos.Y=X[1];
                if(tracker->Go(img,targetPos,_vx,_vy))  // Pos information are in the targetPos, _vx and _vy are speeds.
                {   //fps,x,y,Area
                    z[0]=_vx;
                    z[1]=_vy;
//                    X=kf.UpdateMeasure(Hv, Rv,z);
//                    printf("        %f %f\n%f %f %f %f\n",z[0],z[1],X[0],X[1],X[2],X[3]);
                    Z[2] = _vx;
                    Z[3] = _vy;
                    H(2, 2) = H(3, 3) = 1;
                }
                rect.X=X[0];
                rect.Y=X[1];
                if(detect->Go(img, rect))
                {
//                    z[0]=rect.X;
//                    z[1]=rect.Y;
//                    X=kf.UpdateMeasure(Hx, Rx, z);
//                    printf("%f %f\n%f %f %f %f\n",z[0],z[1],X[0],X[1],X[2],X[3]);
                    Z[0] = rect.X;
                    Z[1] = rect.Y;
                    H(0, 0) = H(1, 1) = 1;
                }
                X = kf.UpdateMeasure(H, R, Z);
//                X=kf.UpdateMeasure(Hv, Rv,z);
                printf("%f %f %f %f\n%f %f %f %f\n", Z[0], Z[1], Z[2], Z[3], X[0], X[1], X[2], X[3]);
                offset=0;
                fps=1000.0/tc.Tick();
                memcpy(tempbuff,&fps,4);
                x=X[0]+targetPos.Width*0.5;
                memcpy(tempbuff+4,&x,4);
                y=X[1]+targetPos.Height*0.5;
                memcpy(tempbuff+8,&y,4);
                Area=targetPos.Width*targetPos.Height;
                memcpy(tempbuff+12,&Area,4);
                printf("Track:%f %f\n",X[0],X[1]);
                fprintf(fp, "%f %f\n", X[0],X[1]);
                GimbalControl(x,y,&controlData,controlLength);
                if(uartOK)
                    uart.Send((unsigned char*)controlData,controlLength);
            }
            printf("Track OK, at time=%f\n", 1000/fps);
        }
        else
        {
            if(inited)
            {
                delete tracker;
                tracker=new lktracking;
                delete(detect);
                detect=new Detection;
                inited=false;
                targetPos.Width=40;
                targetPos.Height=40;
                targetPos.X=(_width-targetPos.Width)*0.5;
                targetPos.Y=(_height-targetPos.Height)*0.5;
                rect.X=targetPos.X;
                rect.Y=targetPos.Y;
                fclose(fp);
            }
        }
        matBuffer.SetBufferToWrite(img_g);
        trackBuffer.SetBufferToRead(tempbuff);
    }

}

int main (int argc, char **argv)
{

    Init(argc,argv);
   
    //新建图像采集线程，图像压缩线程
    pthread_t tidx264,tidcam,tidtrack;

    U8 tempbuff[100];
    TimeClock tc;
    tc.Tick();

    pthread_create(&tidcam, NULL, camera_thread, (void *)( "Camera" ));
    pthread_create(&tidx264, NULL, x264_thread, (void *)( "x264" ));
    pthread_create(&tidtrack, NULL, track_thread, (void *)( "Track" ));

    int offset=0;
    int recLength;
    for (; ; )
    { 
        recLength=_udp.Receive(RecBuf,MaxRecLength);
        if(recLength>0)
        {
            sspUdp.ProcessRawByte((U8 *)RecBuf,recLength);
        }
        if(mode==EXIT)
        {
            uart.Close();
            return 0;
        }
        if (mode==IDLE)
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
        if(mode==TRACK)
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

        printf("Send OK at time=%d\n",tc.Tick());
    }

    return 0;
}

