#include "yuv2hsl.h"
#include<stdlib.h>

#define debugthis

yuv2hsl::yuv2hsl()
{
    //ctor
}

void yuv2hsl::RGB2HSL(int *RGB, float *HSL)
{
    float r = RGB[0] / 255.0;
    float g = RGB[1] / 255.0;
    float b = RGB[2] / 255.0;
    float v;
    float m;
    float vm;
    float r2, g2, b2;

    HSL[0] = 0;
    HSL[1] = 0;
    HSL[2] = 0;

    v = r>=g?r:g;
    v=v>=b?v:b;
    m=r>=g?g:r;
    m=m<=b?m:b;
    HSL[2] = (m + v) / 2.0;
    if (HSL[2] <= 0.0)
    {
        return;
    }
    vm = v - m;
    HSL[1] = vm;
    if (HSL[1] > 0.0)
    {
        HSL[1] /= (HSL[2] <= 0.5) ? (v + m) : (2.0 - v - m);
    }
    else
    {
        return;
    }
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
    {
        HSL[0] = (g == m ? 5.0 + b2 : 1.0 - g2);
    }
    else if (g == v)
    {
        HSL[0] = (b == m ? 1.0 + r2 : 3.0 - b2);
    }
    else
    {
        HSL[0] = (r == m ? 3.0 + g2 : 5.0 - r2);
    }
    HSL[0] /= 6.0;
}
void yuv2hsl::doyuv2hsl(int Width, int Height,U8*Y,U8*U,U8*V,U8*H,U8*S)
{
    int rgb[3]={0};
    float hsl[3]={0};
    int _size=Width*Height;
    int u, v;
#ifdef  debugthis
    static int ii=0;
    FILE *fp=NULL;
    char fname[20]="rgbtest0001.ppm";
    sprintf(fname,"rgbtest%4d.ppm",ii++);

    if((fp=fopen(fname,"w"))==NULL)
    {
        printf("fail to create file rgbtest%4d.ppm\n",ii);
        exit(0);
    }
    fprintf(fp,"P3\n%d %d\n255\n",Width,Height);
#endif // debugthis
    for(int i=0; i<_size; i++)
    {
        u=U[i]-128;
        v=V[i]-128;

        int rdif= v+((v*103)>>8);
        int invgdif =((u*88)>>8)+((v*183)>>8);
        int bdif=u+((u*198)>>8);

        rgb[0]=Y[i]+rdif;
        rgb[1]=Y[i]-invgdif;
        rgb[2]=Y[i]+bdif;

        //防止出现溢出
        if(rgb[0]>255)rgb[0]=255;
        if(rgb[0]<0)rgb[0]=0;

        if(rgb[1]>255)rgb[1]=255;
        if(rgb[1]<0)rgb[1]=0;

        if(rgb[2]>255)rgb[2]=255;
        if(rgb[2]<0)rgb[2]=0;
#ifdef  debugthis
        fprintf(fp,"%d%d%d",rgb[0],rgb[1],rgb[2]);
#endif // debugthis
        RGB2HSL(rgb,hsl);
        H[i]=hsl[0];
        S[i]=hsl[1];
        //L[i]=hsl[2];
    }
#ifdef debugthis
    fclose(fp);
#endif // debugthis
}
