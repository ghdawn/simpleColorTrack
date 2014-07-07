#ifndef _GIMBAL_H
#define _GIMBAL_H

extern float kpx;
extern float kdx;
extern float kpy;
extern float kdy;

void GimbalInit();
void GimbalControl(float x,float y,char**ControlData,int &length);

#endif
