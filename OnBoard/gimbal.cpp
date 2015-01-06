#include "itrbase.h"
#include "gimbal.h"
#include "stdio.h"

itr_protocol::StandSerialProtocol sspObj;

float kpx = 0.5;
float kdx = 0;
float kpy = 0.5;
float kdy = 0;

float U0 = 160;
float V0 = 120;

static float xformer=0;
static float yformer=0;

U8 Buffer[20];
S32 len;

class SSPSendfun : public itr_protocol::StandSerialProtocol::SSPDataSendFun
{
    S32 Do(U8 *buffer, S32 Length)
    {
        for (int i = 0; i < Length; ++i) {
            Buffer[i] = buffer[i];
        }
        return len = Length;
    }
};


void GimbalInit()
{
    sspObj.Init(0xA5, 0x5A, new SSPSendfun);
    FILE* fp=fopen("pidpara.data","r");
    if(fp>0)
    {
        fscanf(fp,"%f %f %f %f",&kpx,&kdx,&kpy,&kdy);
        fclose(fp);
    }
}
void GimbalStop(char**ControlData,int &length)
{
    ASU8(&Buffer[0]) = 0x30;
    ASU8(&Buffer[1]) = 0;
    sspObj.SSPSendPackage(0,Buffer,2);
    *ControlData=(char*)Buffer;
    length=len;
}

void GimbalControl(float x,float y,char**ControlData,int &length)
{
    float omegax;
    float omegay;
    omegax = (x-U0)*kpx + (xformer-x)*kdx;
    omegay = (y-V0)*kpy + (yformer-y)*kdy;
    xformer=x;
    yformer=y;
    

    ASU8(&Buffer[0]) = 0x30;
    ASU8(&Buffer[1]) = 1;
    ASS16(&Buffer[2]) = S16(omegax*10);
    ASS16(&Buffer[4]) = S16(omegay*10);

    sspObj.SSPSendPackage(0,Buffer,6);
    *ControlData=(char*)Buffer;
    length=len;
}

void GimbalUpdatePID(float Kpx,float Kdx,float Kpy,float Kdy)
{
    kpx =Kpx;
    kdx =Kdx;
    kpy =Kpy;
    kdy =Kdy;
    FILE* fp=fopen("pidpara.data","w");
    fprintf(fp,"%f %f %f %f",kpx,kdx,kpy,kdy);
    fclose(fp);
}