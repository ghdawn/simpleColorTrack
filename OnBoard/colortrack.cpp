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

    itr_vision::Binarization BObject;
    itr_vision::ConnectedAnalysis CAObject;
    std::vector<itr_vision::Block> blocks;
    itr_vision::IOpnm::WritePGMFile("H1.pgm",H);
    itr_vision::IOpnm::WritePGMFile("S1.pgm",S);
    const float dh=10;
    const float ds=20;
    BObject.Threshold(H,color+50,0);
    BObject.Threshold(S,100,50);
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
    CAObject.Contour(H,blocks);
    itr_vision::IOpnm::WritePGMFile("H2.pgm",H);
    itr_vision::IOpnm::WritePGMFile("S2.pgm",S);
    return (blocks);
}
