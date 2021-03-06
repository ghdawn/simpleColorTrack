#include "lktracking.h"
#include <iostream>
using namespace std;
using itr_math::Vector;
lktracking::lktracking():
    FeatureNum(100),frame1Feature(FeatureNum),frame2Feature(FeatureNum),fbFeature(FeatureNum)
{
    x=new F32[FeatureNum];
    y=new F32[FeatureNum];
    indexNo=new F32[FeatureNum];
    dist=new F32[FeatureNum];
    debugcount=1;
    trackedPoints=0;
}

void lktracking::Init(const Matrix &img,RectangleF &rect)
{
    _last=new Pyramid;
    _last->Init(img.GetCol(),img.GetRow(),4,2);
    _current=new Pyramid;
    _current->Init(img.GetCol(),img.GetRow(),4,2);
    _current->Generate(img);

    _select_pointer =new SelectKLTFeature(img.GetCol(),img.GetRow());
    _select_pointer->mindist=6;
    _select_pointer->AddImage(_current);

    ransac.Init(&oper);
    ransac.Times=10;

    tracker.max_iterations=10;
}

void lktracking::pairdistance(const vector<Point2D> &feature,vector<F32> &dist)
{
    int n=feature.size(),k=0;
    for(int i=0; i<n; ++i)
        for(int j=0; j<n; ++j)
        {
            if(i==j)
            {
                dist[k]=0.1;
            }
            else
            {
                dist[k]=(feature[i]-feature[j]).GetDistance();
            }
            ++k;
        }
}

F32 lktracking::getScale(S32 pointcount)
{
    S32 n=pointcount*pointcount;
    vector<F32> pointDist1(n),pointDist2(n),distDiv(n);
    vector<Point2D> feature1(pointcount),feature2(pointcount);
    int i,j=0;
    for(i=0; i<FeatureNum; ++i)
    {
        if(frame2Feature[i].Quality>=0)
        {
            feature1[j]=frame1Feature[i];
            feature2[j]=frame2Feature[i];
            j++;
        }
    }
    pairdistance(feature1,pointDist1);
    pairdistance(feature2,pointDist2);
    itr_math::CalculateObj->Div(&pointDist2[0],&pointDist1[0],n,&distDiv[0]);

    F32 median;
//    itr_math::StatisticsObj->Mean(&distDiv[n/4],n/2,median);
    itr_math::StatisticsObj->Median(&distDiv[0],n,median);

    return median;
}

int lktracking::ncc_filter(const Matrix  &input1,const Matrix  &input2)
{
    RectangleS rect;
    rect.Width=rect.Height=3;
    int i,length=rect.Width*rect.Height;
    F32 mean1,mean2;
    int drop=0;
    Matrix patch1(rect.Width,rect.Height),patch2(rect.Width,rect.Height);
    Vector v1(length,patch1.GetData());
    Vector v2(length,patch2.GetData());
    for (i = 0; i <FeatureNum; ++i)
    {
        rect.X=frame1Feature[i].X-rect.Width/2;
        rect.Y=frame1Feature[i].Y-rect.Height/2;
        Pick::Rectangle(input1,rect,patch1);
        rect.X=frame2Feature[i].X-rect.Width/2;
        rect.Y=frame2Feature[i].Y-rect.Height/2;
        Pick::Rectangle(input2,rect,patch2);
        assert(v1.GetData()==patch1.GetData());
        itr_math::StatisticsObj->Mean(patch1.GetData(),length,mean1);
        itr_math::StatisticsObj->Mean(patch2.GetData(),length,mean2);
        itr_math::CalculateObj->Offset(v1.GetData(),-mean1,length,v1.GetData());
        itr_math::CalculateObj->Offset(v2.GetData(),-mean2,length,v2.GetData());
        itr_math::CalculateObj->Normalization(v1.GetData(),length,v1.GetData());
        itr_math::CalculateObj->Normalization(v2.GetData(),length,v2.GetData());
        dist[i]=v1*v2;
    }

    for (i = 0; i <FeatureNum; ++i)
    {
        if(dist[i]<0.8)
        {
            frame2Feature[i].Quality=-LKTracker::NCCError;
            ++drop;
        }
    }
    return drop;
    //getchar();
}


int lktracking::fb_filter()
{
    int i,drop=0;
    for (i = 0; i <FeatureNum; ++i)
    {
        dist[i]=(frame1Feature[i]-frame2Feature[i]).GetDistance();
    }
    F32 median;
    itr_math::StatisticsObj->Median(dist,FeatureNum,median);
    for (i = 0; i <FeatureNum; ++i)
    {
        if(dist[i]>median)
        {
            frame2Feature[i].Quality=-LKTracker::FBError;
            ++drop;
        }
    }
    return drop;
}

void lktracking::Debug_Info()
{
    ///特征点匹配关系输出
    Matrix cor;
    vector<Point2D> outU(FeatureNum),outV(FeatureNum);
    int count=0;
    for(int i=0; i<FeatureNum; ++i)
    {
        if(frame2Feature[i].Quality>=0)
        {
            outU[count]=frame1Feature[i];
            outV[count]=frame2Feature[i];
            count++;
        }
    }
    Draw::Correspond(_last->img[0],_current->img[0],outU,outV,count,cor);
    char file[20];
    sprintf(file,"bin/Debug/corr/corr%d",debugcount++);
    IOpnm::WritePGMFile(file,cor);
}

bool lktracking::InSide(const Point2D &point,const RectangleF &rect)const
{
    if(point.X>rect.X+rect.Width)
    {
        return false;
    }
    if(point.X<rect.X)
    {
        return false;
    }
    if(point.Y>rect.Y+rect.Height)
    {
        return false;
    }
    if(point.Y<rect.Y)
    {
        return false;
    }
    return true;
}
bool lktracking::Go(const Matrix &img,RectangleF &rect,F32 &Vx,F32 &Vy)
{
    TimeClock clock;  
    int i;
    bool Tracked=true;
    clock.Tick();
    printf("\n*****Begin  Track !*****\n\n");
    FeatureNum= _select_pointer->SelectGoodFeature(rect,frame1Feature,trackedPoints);

    printf("Feature: %d  at Time %d\n",FeatureNum,clock.Tick());


    Pyramid *temp=_last;
    _last=_current;
    _current=temp;
    _current->Generate(img);        
   
    printf("Pyramid OK  at Time %d\n",clock.Tick());
    
    tracker.Compute(frame1Feature,frame2Feature,FeatureNum,_last,_current);
    tracker.Compute(frame2Feature,fbFeature,FeatureNum,_current,_last);
    
    trackedPoints=0;
    for (i = 0; i < FeatureNum; ++i)
    {
        if (frame2Feature[i].Quality == 0)
        {
            ++trackedPoints;
        }
    }
    printf("Tracked:%d at Time%d\n",trackedPoints,clock.Tick());
    ///Filter
    if(FeatureNum>0)
    {
        printf("FBFilter: %d  \n",fb_filter());
        printf("NCCFilter: %d  \n",ncc_filter(_last->img[0],_current->img[0]));
    }

    ///计算光流速度速度

    trackedPoints=0;
    for (i = 0; i < FeatureNum; ++i)
    {
        if (frame2Feature[i].Quality == 0)
        {
            x[trackedPoints] = frame2Feature[i].X - frame1Feature[i].X;
            y[trackedPoints] = frame2Feature[i].Y - frame1Feature[i].Y;
            indexNo[trackedPoints]=i;
            ++trackedPoints;
        }
    }

    // cout << "Points: "<<trackedPoints << endl;
    printf("Points:%d at Time %d\n",trackedPoints,clock.Tick());
    ///如果有跟踪到的点,则对x方向和y方向做RANSAC过滤,把速度不相符的点过滤掉
    if(trackedPoints>0)
    {
        S32 drop=0,dropx,dropy;
        //RANSAC
        ransac.Number=trackedPoints/2+1;
        ransac.Process(x,trackedPoints, dropx);
        printf("drop: %d ",dropx);
        for(i=0; i<trackedPoints; ++i)
            if(fabs(x[i]-Ransac<F32,F32>::INF)<1)
            {
                if(frame2Feature[indexNo[i]].Quality==0)
                {
                    ++drop;
                    frame2Feature[indexNo[i]].Quality=-LKTracker::LARGE_RESIDUE;
                }
            }
        std::sort(x,x+trackedPoints);
        Vx=x[(trackedPoints-dropx)/2];
//        if(fabs(Vx)>25)
//        {
//            Tracked=false;
//            //printf("Failure!!\n");
//        }

        ransac.Process(y,trackedPoints, dropy);
        printf("%d \n",dropy);

        for(i=0; i<trackedPoints; ++i)
            if(fabs(y[i]-Ransac<F32,F32>::INF)<1)
            {
                if(frame2Feature[indexNo[i]].Quality==0)
                {
                    ++drop;
                    frame2Feature[indexNo[i]].Quality=-LKTracker::LARGE_RESIDUE;
                }
            }

        std::sort(y,y+trackedPoints);
        Vy=y[(trackedPoints-dropy)/2];
//        if(fabs(Vy)>25)
//        {
//            Tracked=false;
//            printf("Failure!!\n");
//        }
        printf("vx=%f vy=%f\n",Vx,Vy);
        trackedPoints-=drop;
    }
    printf("V at %d\n", clock.Tick());

    if(trackedPoints<=0)
    {
        Tracked=false;
    }

    if(Tracked)
    {
        F32 boxScale=1.0f;
        // boxScale=getScale(trackedPoints);
        // printf("scale:%f\n",boxScale);
        if(boxScale>1.05)
        {
            Tracked=false;
        }
        else
        {
            rect.X+=Vx-rect.Width*(boxScale-1.0f)/2.0f;
            rect.Y+=Vy-rect.Height*(boxScale-1.0f)/2.0f;
            rect.Width*=boxScale;
            rect.Height*=boxScale;
        }
    }

    ///选择下一帧图像中的特征点

    trackedPoints=0;
    for (i = 0; i < FeatureNum; ++i)
    {
        if (frame2Feature[i].Quality == 0)
        {
            frame1Feature[trackedPoints].X=frame2Feature[i].X;
            frame1Feature[trackedPoints].Y=frame2Feature[i].Y;
            frame1Feature[trackedPoints].Quality=frame1Feature[i].Quality;
            ++trackedPoints;

        }
    }

    _select_pointer->AddImage(_current);
    printf("Final at time %d\n", clock.Tick());
// static int t=0,all=0;
// t++;
// all+=clock.Tick();
//     printf("Track Time: %d",all/t);
     printf("\n*****End  Track !*****\n\n");
    //printf("\033[12A");
    return (Tracked);
}

lktracking::~lktracking()
{
    delete[] x;
    delete[] y;
    delete[] indexNo;
    delete[] dist;
    delete _current;
    delete _last;
    if(_select_pointer!=NULL)
    {
        delete _select_pointer;
    }
}
