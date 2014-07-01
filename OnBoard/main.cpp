#include "Udp.h"
#include "itrdevice.h"
#include "basestruct.h"
#include "colortrack.h"
using namespace std;

const int MaxLength=1000;
const int ListenPort=9031,SendPort=9032;
Communication::Udp _udp(ListenPort,false);

itr_device::ICamera *camera=NULL;

ColorTrack tracker;

Config config;

void Init()
{
    config.color=0;
    config.result=0;
    config.camera=0;
    config.pixel=0;

//    camera=new itr_device::v4linux;
 //   camera.Open(0,Width[config.pixel],Height[[config.pixel]],0);
}

bool start=false;
bool track=false;

void Option(char* RecBuf)
{
            switch(RecBuf[0])
            {
                case 0:
                    start=false;
                    track=false;
                    break;
                case 1:
                    start=true;
                    track=false;
                    break;
                case 2:
                    start=true;
                    track=true;
                    break;
                case 3:
                    config.color=RecBuf[1];
                    config.result=RecBuf[2];
                    break;
                case 4:
                    config.camera=RecBuf[1];
                    config.pixel=RecBuf[2];
                    if(camera!=NULL)
                    {
                        delete camera;
//                        camera=new itr_device::ASICamera;
//                        camera.Open(0,Width[config.pixel],Height[[config.pixel]],0);
                    }
                    break;
            }

}
int main()
{
    Init();
    char RecBuf[5];
    char SendBuf[MaxLength];
    while(1)
    {
        if(_udp.Receive(RecBuf,5))
        {
            Option(RecBuf);
        }
        if (!start)
            continue;

        //TODO：进行跟踪
        if(track)
        {
//            list=tracker.Track(*,*);
        }

        //TODO：X264压缩
        //

        //TODO：准备待发送数据

        //TODO: 按照SSP
    }
    cout << "Hello world!" << endl;
    return 0;
}
