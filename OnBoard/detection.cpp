#include "detection.h"
#include "iostream"

Detection::Detection() :
        patchsize(15), ncomp(11), nfeat(15) {
}

void Detection::Init(const Matrix &img, const RectangleS &rect) {
    int width = img.GetCol(), height = img.GetRow();
    conv.Init(width, height);
    imginput.Init(height, width);
    imgInt2.Init(height, width);
    imgInt.Init(height, width);
    imgnorm.Init(patchsize, patchsize);
    imgpatch.Init(rect.Height, rect.Width);

    fern.Init(nfeat, ncomp, rect.Width * rect.Height);
    nnc.Init(new Dis);

    Train(img, rect);
}

void Detection::Train(const Matrix &img, const RectangleS &rect) {
    conv._KLTComputeSmoothedImage(img, 2, imginput);
    //Variance Init
    itr_vision::IntegralImg::Integral(imginput, imgInt);
    itr_vision::IntegralImg::Integral2(imginput, imgInt2);
    EI = imgInt(rect.Y, rect.X) + imgInt(rect.Y + rect.Height, rect.X + rect.Width) - imgInt(rect.Y + rect.Height, rect.X) - imgInt(rect.Y, rect.X + rect.Width);
    VI = (imgInt2(rect.Y, rect.X) + imgInt2(rect.Y + rect.Height, rect.X + rect.Width) - imgInt2(rect.Y + rect.Height, rect.X) - imgInt2(rect.Y, rect.X + rect.Width)) * (rect.Height * rect.Width) - EI * EI;

    itr_vision::Pick::Rectangle(imginput, rect, imgpatch);
    itr_vision::Scale::Bilinear(imgpatch, imgnorm);
    Vector tar(patchsize * patchsize);
    for (int i = 0; i < patchsize * patchsize; i++) {
        tar[i] = imgnorm[i];
    }
    nnc.Train(tar, true);


    // Training
    int Num = 100;

    itr_vision::GenRect::genrectin(rect, rects, Num);
    for (int k = 0; k < Num; k++) {
        itr_vision::Pick::Rectangle(imginput, rects[k], imgpatch);
        itr_vision::Scale::Bilinear(imgpatch, imgnorm);
        fern.Train(imgpatch, true);
        for (int i = 0; i < patchsize * patchsize; i++) {
            tar[i] = imgnorm[i];
        }
        nnc.Train(tar, true);
    }

    itr_vision::GenRect::genrectout(rect, rects, Num);
    for (int k = 0; k < Num; k++) {
        itr_vision::Pick::Rectangle(imginput, rects[k], imgpatch);
        itr_vision::Scale::Bilinear(imgpatch, imgnorm);
        fern.Train(imgpatch, false);
        for (int i = 0; i < patchsize * patchsize; i++) {
            tar[i] = imgnorm[i];
        }
        nnc.Train(tar, false);
    }
}

bool Detection::Go(const Matrix &img, RectangleS &rect) {
    conv._KLTComputeSmoothedImage(img, 2, imginput);
    itr_vision::IntegralImg::Integral(imginput, imgInt);
    itr_vision::IntegralImg::Integral2(imginput, imgInt2);
    //begin work
    TimeClock clock;

    int pcount;
    F32 e;
    F32 v;
    clock.Tick();
    vector<itr_math::Matrix> imglist;
    int x_last = rect.X, y_last = rect.Y;
    bool detected = false;
    int t1, t2;
    F32 cur, best = 0;
    pcount = 0;

    clock.Tick();
    int range = 20;
    for (int y = y_last - range; y < y_last + range; y += 1) {
        for (int x = x_last - range; x < x_last + range; x += 1) {
            if (x < 0 || x >= imginput.GetCol())continue;
            if (y < 0 || y >= imginput.GetRow())continue;
            //Variance filter
            e = imgInt(y, x) + imgInt(y + rect.Height, x + rect.Width) - imgInt(y + rect.Height, x) - imgInt(y, x + rect.Width);
            v = (imgInt2(y, x) + imgInt2(y + rect.Height, x + rect.Width) - imgInt2(y + rect.Height, x) - imgInt2(y, x + rect.Width)) * (rect.Height * rect.Width) - e * e;

            if (e < EI * 0.4 || e > EI * 1.6) {
                continue;
            }
            if (v < VI * 0.4 || v > VI * 1.6) {
                continue;
            }

            rect.X = x;
            rect.Y = y;
            itr_vision::Pick::Rectangle(imginput, rect, imgpatch);

            cur = fern.Classify(imgpatch);
            if (cur > best) {
                rects[0].X = x;
                rects[0].Y = y;
                itr_vision::Draw::Rectangle(imginput, rects[0], 255);
                best = cur;
            }
//            if (cur > 0.5) {
//                rects[imglist.size()].X = x;
//                rects[imglist.size()].Y = y;
//                imglist.push_back(imgpatch);
//            }
//            pcount++;
        }
    }
    t1 = clock.Tick();
    printf("\ntime:%d %d %f\n", t1, pcount, (t1 + 0.1) / pcount);
    if (best > 0) {
        rect = rects[0];
        detected = true;
//        itr_vision::Draw::Rectangle(imginput, rect, 0);
//        itr_vision::IOpnm::WritePGMFile("at.pgm", imginput);
    }
//    F32 dis1, dis2, mind = 99999;
//    S32 index = -1;
//    printf("size:%d\ndis:", imglist.size());
//    for (int i = 0; i < imglist.size(); i++) {
//        itr_vision::Scale::Bilinear(imglist[i], imgnorm);
//        Vector imgvec(patchsize * patchsize, imgnorm.GetData());
//        nnc.Classify(imgvec, dis1, dis2);
//
//        if (dis1 < dis2 && mind > dis1) {
//            mind = dis1;
//            index = i;
//        }
//    }
//    if (index > 0) {
//        rect.X = rects[index].X;
//        rect.Y = rects[index].Y;
//    }
//    else
//    {
//        rect.X=-1;
//        rect.Y=-1;
//    }
//    t2 = clock.Tick();
//    printf("\n");
//    detected = false;
//    if (imglist.size() > 0) {
//
//        for (int i = 0; i < imglist.size(); i++)
//            itr_vision::Draw::Rectangle(imginput, rects[i], 255);

//        detected = true;
//    }

    printf("Count:%d,t1:%d,t2: %d ms\n", imglist.size(), t1, t2);
    std::cout << std::endl;
    return detected;
}
