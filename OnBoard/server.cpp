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
#include "yuv2hsl.h"
#include "gimbal.h"

using namespace std;

const int MaxSendLength=65535;
const int MaxRecLength=25;
const int ListenPort = 9031, ClientPort = 9032, EstiPort = 9033;

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
int TrackResultLength;
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

F32 fps,x,y,Area;
U32 cameraID = 0, cameraTunnel = 0;
/**
* 程序运行模式
* 可能的取值与其对应意义
* 0  停机
* 1  摄像头工作，跟踪不工作
* 2  跟踪工作
*/
const U8 IDLE = 0;
const U8 CAPTURE = 1;
const U8 TRACK = 2;
const U8 EXIT = 3;
U8 mode = IDLE;

///SSP接受数据，进行命令解析
class SSPReceiveFunc : public itr_protocol::StandSerialProtocol::SSPDataRecFun
{
    void Do(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS, U8 *Package, S32 PackageLength)
    {
        F32 *a, *b, *c, *d;
        switch (Package[0])
        {
            case 0x41:
                mode = Package[1];
                if (mode != TRACK)
                {
                    GimbalStop(&controlData, controlLength);
                    if (uartOK)
                        uart.Send((unsigned char *) controlData, controlLength);
                }
                break;
            case 0x42:
                MemoryCopy(Package, Package + 1, 16);
                a = (F32 *) (Package);
                b = (F32 *) (Package + 4);
                c = (F32 *) (Package + 8);
                d = (F32 *) (Package + 12);
                GimbalUpdatePID(*a, *b, *c, *d);
                break;
            case 0x44:
                config.color = Package[1];
                break;
            case 0x43:
                config.fps = Package[1];
                config.pixel = Package[2];
                break;
            case 0x45:
                MemoryCopy(Package, Package + 1, 8);
                a = (F32 *) (Package);
                b = (F32 *) (Package + 4);
                targetPos.Width = *a;
                targetPos.Height = *b;
                break;
                //直接转发
            default:
                if (uartOK)
                    uart.Send((U8 *) SSFS, SSP->GetSSFSLength(SSFS));
                break;
        }
    }
};


class SSPSend : public itr_protocol::StandSerialProtocol::SSPDataSendFun {
    S32 Do(U8 *Buffer, S32 Length) {
        TrackResultLength = Length;
        memcpy(SendBuf, Buffer, Length);

        U8 *img = compressBuffer.GetBufferToRead();
        while (img == NULL) {
            img = compressBuffer.GetBufferToRead();
        }
        printf("PointSend:%p ", img);
        memcpy(SendBuf + Length, (img + 4), *((int *) img));
        SendLength = Length + *((int *) img);

        compressBuffer.SetBufferToWrite(img);
        printf("%p ", img);
        cout << endl;
        return SendLength;
    }
};
//准备要发送的数据




//初始化参数
void Init(int argc, char **argv)
{
    config.color=0;
    config.pixel=0;
    config.fps=30;

    udpPackage.IP=argv[1];
    udpPackage.port = ClientPort;

    itr_math::MathObjStandInit();

    sspUdp.Init(0xA5, 0x5A, new SSPSend);//串口发送函数 代替 NULL
    sspUdp.AddDataRecFunc(new SSPReceiveFunc, 0);

    cameraID=argv[2][0]-'0';
    cameraTunnel=argv[3][0]-'0';

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
    targetPos.X = (_width - targetPos.Width) * 0.5f;
    targetPos.Y = (_height - targetPos.Height) * 0.5f;


    compressBuffer.Init(2);
    yuvBuffer.Init(2);
    yuvBuffer.AddBufferToList(new U8[2*_size]);
    yuvBuffer.AddBufferToList(new U8[2*_size]);
    matBuffer.Init(2);
    matBuffer.AddBufferToList(new F32[2*_size]);
    matBuffer.AddBufferToList(new F32[2*_size]);
    trackBuffer.Init(2);

    trackBuffer.AddBufferToList(new U8[20]);
    trackBuffer.AddBufferToList(new U8[20]);


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
    U8 *_imgcomp = NULL;
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
        if (pic==NULL)
        {
            usleep(10);
            continue;
        }
        _imgcomp = compressBuffer.GetBufferToWrite();
        while (_imgcomp==NULL)
        {
            usleep(10);
            _imgcomp=compressBuffer.GetBufferToWrite();
        }
        printf("Point:%p ", _imgcomp);
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
        printf("%p ", _imgcomp);
        cout << endl;
        memcpy(_imgcomp+4,imgCompressData,imgLength);
        *(int *) _imgcomp = imgLength;
        printf("%p", _imgcomp);
        cout << endl;
        compressBuffer.SetBufferToRead(_imgcomp);
    }
    compress.Close();
}

void* camera_thread(void *name)
{
    itr_device::v4linux capture;
    capture.Open(cameraID,_width,_height,2);
    capture.SetTunnel(cameraTunnel);
    printf("camera opened !\n");

    U8 *pic;
    F32* img_g;
    TimeClock tc;
    tc.Tick();
    yuv2hsl yuv2hslobj;
    while (mode != EXIT)
    {
        if (mode == IDLE)
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
        capture.FetchFrame(pic,_size*3/2,NULL);
        img_g=matBuffer.GetBufferToWrite();
        while(img_g==NULL)
        {
	        usleep(10);
            img_g=matBuffer.GetBufferToWrite();
        }
        yuv2hslobj.doyuv2hsl(_width,_height,pic,pic+_size,pic+_size+_size/4,img_g,img_g+_size);
        yuvBuffer.SetBufferToRead(pic);
        matBuffer.SetBufferToRead(img_g);
    }
    capture.Close();
}
void* track_thread(void* name)
{
    F32* img_hs;
    U8* tempbuff;
    U8 offset=0;
    ColorTrack tracker;
    TimeClock tc;
    tc.Tick();
    while (mode != EXIT)
    {
        img_hs=matBuffer.GetBufferToRead();
        if (img_hs==NULL)
        {
            continue;
        }
        if (mode == TRACK)
        {
            tc.Tick();
            Matrix matH(_height,_width,img_hs),matS(_height,_width,img_hs+_size);
            std::vector<itr_vision::Block> list=tracker.Track(matH,matS,ColorTable[config.color]);
            if(list.size()>0)
            {
                x=list[0].x;
                y=list[0].y;
                Area=list[0].Area;
                tempbuff=trackBuffer.GetBufferToWrite();
                while(tempbuff==NULL)
                {
                    usleep(10);
                    tempbuff=trackBuffer.GetBufferToWrite();
                }
                offset+=4;
                memcpy(tempbuff+offset,(void*)&x,4);
                offset+=4;
                memcpy(tempbuff+offset,(void*)&y,4);
                offset+=4;
                memcpy(tempbuff+offset,(void*)&Area,4);
                offset+=4;
                trackBuffer.SetBufferToRead(tempbuff);
                GimbalControl( x, y,&controlData,controlLength);
                if(uartOK)
                    uart.Send((unsigned char*)controlData,controlLength);
            }
            else
            {
                x=y=Area=0;
                GimbalStop(&controlData,controlLength);
                if(uartOK)
                    uart.Send((unsigned char*)controlData,controlLength);
            }
            matBuffer.SetBufferToWrite(img_hs);
            printf("\nTrack OK, at %f ,%f \t time=%d\n", x, y, tc.Tick());
        }
        else
        {
            matBuffer.SetBufferToWrite(img_hs);
        }
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
    while (mode != EXIT)
    { 
        if(_udp.Receive(RecBuf,MaxRecLength))
        {
            //使用SSP进行解包
            sspUdp.ProcessRawByte((U8 *)RecBuf,MaxRecLength);
        }
        if (mode == IDLE)
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
        if (mode == TRACK)
        {
            U8* tracktemp=trackBuffer.GetBufferToRead();
            if(tracktemp!=NULL)
            {
                MemoryCopy(tempbuff+offset,(void*)tracktemp,16);
                fps = 1000.0f / tc.Tick();
                MemoryCopy(tempbuff + offset, (void *) &fps, 4);
                trackBuffer.SetBufferToWrite(tracktemp);
            }
        }
        offset+=16;
        sspUdp.SSPSendPackage(0,tempbuff,offset);
            // 发送结果
        udpPackage.pbuffer=SendBuf;
        udpPackage.port = EstiPort;
        udpPackage.len = TrackResultLength;
        _udp.Send(udpPackage);

        udpPackage.port = ClientPort;
        udpPackage.len=SendLength;
        _udp.Send(udpPackage);
        printf("Send OK at time=%f\n", 1000.0 / fps);
    }
    return 0;
}

