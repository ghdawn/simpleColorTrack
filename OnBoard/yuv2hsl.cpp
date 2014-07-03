#include "yuv2hsl.h"

yuv2rgb::yuv2rgb()
{
    //ctor
}

yuv2rgb::~yuv2rgb()
{
    //dtor
}
void doyuv2hsl(int Width, int Height,U8*Y,U8*U,U8*V,U8*H,U8*S)
{

}
void RGB2HSL(int *RGB, float *HSL)
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
    //v = Max(r, g);
    v = r>g?r:g;
    //v = Max(v, b);
    v=v>b?v:b;
    //m = Min(r, g);
    m=r>g?g:r;
    //m = Min(m, b);
    m=m<b?m:b;
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
