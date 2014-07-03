#ifndef COLORTRACK_H
#define COLORTRACK_H
#include <vector>
#include "itrbase.h"
#include "itrvision.h"

using itr_math::Matrix;
class ColorTrack
{
    public:
        /** Default constructor */
        ColorTrack();
        void Init(int Width,int Height);
        std::vector<itr_vision::Block> Track(const Matrix& H,const Matrix& S, int color);
    protected:
    private:
};

#endif // COLORTRACK_H
