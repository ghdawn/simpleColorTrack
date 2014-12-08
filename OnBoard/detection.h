#include "itrbase.h"
#include "itralgorithm.h"
#include "itrvision.h"

using namespace itr_math;
using namespace itr_algorithm;

class Detection {
public:
    Detection();

    void Init(const Matrix &img, const RectangleS &rect);

    void Train(const Matrix &img, const RectangleS &rect);

    bool Go(const Matrix &img, RectangleS &rect);

    int ncomp;
    int nfeat;
    int patchsize;

    class Dis : public itr_algorithm::NearestNeighbor::Operator {
    public:
        F32 GetDis(Vector &v1, Vector &v2) {
            return ((F32) acos(v1.NormalCorrelationCoefficient(v2)));
        }
    };

private:

    itr_math::Matrix imginput, imgInt, imgInt2, imgpatch, imgnorm;
    F32 EI;
    F32 VI;
    Fern fern;
    NearestNeighbor nnc;
    itr_vision::ConvoluteSquare conv;
    RectangleS rects[150];

};

