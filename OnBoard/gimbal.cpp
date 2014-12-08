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
S32 SSPSendfun(U8* buffer,S32 Length)
{
    for (int i = 0; i < Length; ++i)
    {
        Buffer[i]=buffer[i];
    }
    return len=Length;
}

void GimbalInit()
{
    sspObj.Init(0xA5 ,0x5A ,SSPSendfun);
    FILE* fp=fopen("pidpara.data","r");
    if(fp>0)
    {
        fscanf(fp,"%f %f %f %f",&kpx,&kdx,&kpy,&kdy);
        fclose(fp);
    }
}
void GimbalStop(char**ControlData,int &length)
{
    ASU8(&Buffer[0]) = 0x31;
    ASU8(&Buffer[1]) = 3;
    ASS16(&Buffer[2]) = S16(0);
    ASU8(&Buffer[4]) =3;
    ASS16(&Buffer[5]) = S16(0);

    sspObj.SSPSendPackage(0,Buffer,7);
    *ControlData=(char*)Buffer;
    length=len;
}

void GimbalControl(float x,float y,char**ControlData,int &length)
{
    const int Limit=50;
    float omegax;
    float omegay;
    omegax = (x-U0)*kpx + (xformer-x)*kdx;
    omegay = (y-V0)*kpy + (yformer-y)*kdy;
    xformer=x;
    yformer=y;
    omegax=(omegax>Limit)?Limit:omegax;
    omegax=(omegax<-Limit)?-Limit:omegax;
    omegay=(omegay>Limit)?Limit:omegay;
    omegay=(omegay<-Limit)?-Limit:omegay;
    ASU8(&Buffer[0]) = 0x31;
    ASU8(&Buffer[1]) = 3;
    ASS16(&Buffer[2]) = S16(omegax*10);
    ASU8(&Buffer[4]) =3;
    ASS16(&Buffer[5]) = S16(omegay*10);

    sspObj.SSPSendPackage(0,Buffer,7);
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