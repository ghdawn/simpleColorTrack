#ifndef YUV2RGB_H
#define YUV2RGB_H
#include "itrbase.h"

class yuv2hsl
{
    public:
        yuv2hsl();
        virtual ~yuv2hsl();
        void doyuv2hsl(int Width, int Height,U8*Y,U8*U,U8*V,U8*H,U8*S);
    protected:
    private:
        void RGB2HSL(int *RGB, float *HSL);

};

#endif // YUV2RGB_H
