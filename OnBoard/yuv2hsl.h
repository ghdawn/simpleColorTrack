#ifndef YUV2RGB_H
#define YUV2RGB_H
#include "itrbase.h"

class yuv2hsl
{
    public:
        yuv2hsl();
        void doyuv2hsl(int Width, int Height,U8*Y,U8*U,U8*V,F32 *H,F32 *S);
    protected:
    private:
        void RGB2HSL(int *RGB, float *HSL);

};

#endif // YUV2RGB_H
