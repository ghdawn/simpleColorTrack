#include "colortrack.h"

ColorTrack::ColorTrack()
{
    //ctor
}

/*1.read from file to Matrix         */
/*2.Binarization                                */
/*3.ConnectedAnalysis                   */
/*4.Output the target coordinate*/
std::vector<itr_vision::Block> ColorTrack::Track( Matrix &H, Matrix &S, int color)
{

    itr_vision::Binarization::Threshold(H, color - 15, color + 15);
    itr_vision::Binarization::Threshold(S,70,100);
    int _size=H.GetCol()*H.GetRow();
    for(int i=0; i<_size; ++i)
    {
        if(H[i]*S[i]<10)
        {
            H[i]=0;
        }
        else
        {
            H[i]=255;
        }
    }
    itr_vision::ConnectedAnalysis CAObject;
    std::vector<itr_vision::Block> blocks;
    CAObject.Contour(H,blocks);

    return (blocks);
}
