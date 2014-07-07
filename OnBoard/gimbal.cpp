#include "gimbal.h"
#include "itrbase.h"

itr_protocol::StandSerialProtocol sspObj;

float kpx = 0.5;
float kdx = 0;
float kpy = 0.5;
float kdy = 0;

float U0 = 160;
float V0 = 120;

U8 Buffer[];
S32 SendResultPrepare(U8* Buffer,S32 Length)
{
    return SendLength;
}

void GimbalInit()
{
    sspObj.Init(0xA5 ,0x5A ,SendResultPrepare);
}

void GimbalControl(float x,float y,char* ControlData,int length)
{
    float omegax;
    float omegay;
    omegax = (X[0]-U0)*kpx + X[2]*kdx;
    omegay = (X[1]-V0)*kpy + X[3]*kdy;

    ASU8(&buffer[0]) = 0x30;
    ASU8(&buffer[1]) = 1;
    ASS16(&buffer[2]) = S16(omegax*10);
    ASS16(&buffer[4]) = S16(omegay*10);
    sspObj.SSPSendPackage(0,buffer,6);
}