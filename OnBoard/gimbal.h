#ifndef _GIMBAL_H
#define _GIMBAL_H

void GimbalInit();
void GimbalControl(float x,float y,char**ControlData,int &length);
void GimbalUpdatePID(float kpx,float kdx,float kpy,float kdy);
#endif
