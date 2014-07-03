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
using namespace std;

const int MaxSendLength=1000;
const int MaxRecLength=20;
const int ListenPort=9031,SendPort=9032;
Communication::Udp _udp(ListenPort,false);
Communication::Udp::UdpPackage package;
ColorTrack tracker;

Config config;

void Init()
{
    config.color=0;
    config.result=0;
    config.pixel=0;
    config.fps=30;

    package.IP="0.0.0.0";
    package.port=SendPort;
}

bool start=false;
bool track=false;
///SSP接受数据，进行命令解析
void SSPReceivefuc(itr_protocol::StandSerialProtocol* SSP, itr_protocol::StandSerialFrameStruct* SSFS,U8* Package,S32 PackageLength)
{
     switch(Package[0])
            {
                case 2:
                    start=false;
                    track=false;
                    break;
                case 3:
                    start=true;
                    track=false;
                    //直接转发
                    break;
                case 4:
                    start=true;
                    track=true;
                    break;
                case 5:
                    config.color=Package[1];
                    config.result=Package[2];
                    break;
                case 6:
                	config.fps=Package[1];
                    config.pixel=Package[2];
                    break;
            }
}
/** webcam_server: 打开 /dev/video0, 获取图像, 压缩, 发送到 localhost:3020 端口
 *
 * 	使用 320x240, fps=10
 */



int main (int argc, char **argv)
{
	Init();
	char RecBuf[MaxRecLength];
    char SendBuf[MaxSendLength];
     itr_protocol::StandSerialProtocol ssp_obj;
     yuv2rgb yuv2rgb_obj;
	void *capture = capture_open("/dev/video0", Width[config.pixel], Height[config.pixel], PIX_FMT_YUV420P);

	U8* img_hs=new U8[2*Width[config.pixel]*Height[config.pixel]];

	if (!capture) {
		fprintf(stderr, "ERR: can't open '/dev/video0'\n");
		exit(-1);
	}

	void *encoder = vc_open(Width[config.pixel], Height[config.pixel], config.fps);
	if (!encoder) {
		fprintf(stderr, "ERR: can't open x264 encoder\n");
		exit(-1);
	}


	// int tosleep = 1000000 / VIDEO_FPS;
	for (; ; ) {
		if(_udp.Receive(RecBuf,MaxRecLength))
        {
            //TODO:使用SSP进行解包
            ssp_obj.Init(0xA5 ,0x5A ,NULL);
            ssp_obj.ProcessFunction[0]=&SSPReceivefuc;
            ssp_obj.ProcessRawByte((U8*)RecBuf,MaxRecLength);
        }
        // if (!start)
        //     continue;

        //TODO:获取图像，进行RGB，HSL的转换
		Picture pic;
		capture_get_picture(capture, &pic);
        yuv2rgb_obj.doyuv2hsl(Width[config.pixel],Height[config.pixel],pic.data[0],pic.data[1],pic.data[2],
                                                    img_hs,img_hs+Width[config.pixel]*Height[config.pixel]);


        //TODO：进行跟踪
        if(track)
        {
//            list=tracker.Track(*,*);

        }

		// 压缩图像
		const void *outdata;
		int outlen;
		int rc = vc_compress(encoder, pic.data, pic.stride, &outdata, &outlen);
		printf("%d\n",rc );
		if (rc < 0) continue;

		//用SSP封装，包括图像和跟踪结果


		// 发送结果
		package.pbuffer=(char*)outdata;
		package.len=outlen;
		_udp.Send(package);

		// 等
		// usleep(tosleep);
	}

	vc_close(encoder);
	capture_close(capture);

	return 0;
}

