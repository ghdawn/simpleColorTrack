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
    N = (rect.Height * rect.Width);
    EI = imgInt(rect.Y, rect.X) + imgInt(rect.Y + rect.Height, rect.X + rect.Width) - imgInt(rect.Y + rect.Height, rect.X) - imgInt(rect.Y, rect.X + rect.Width);
    VI = (imgInt2(rect.Y, rect.X) + imgInt2(rect.Y + rect.Height, rect.X + rect.Width) - imgInt2(rect.Y + rect.Height, rect.X) - imgInt2(rect.Y, rect.X + rect.Width)) * N - EI * EI;

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

    F32 e;
    F32 v;
    int x_last = rect.X, y_last = rect.Y;
    bool detected = false;
    int t1, t2;
    F32 cur, best = 0;

    int range = 20;
    for (int y = y_last - range; y < y_last + range; y += 1) {
        for (int x = x_last - range; x < x_last + range; x += 1) {
            if (x < 0 || x >= imginput.GetCol())continue;
            if (y < 0 || y >= imginput.GetRow())continue;
            //Variance filter
            e = imgInt(y, x) + imgInt(y + rect.Height, x + rect.Width) - imgInt(y + rect.Height, x) - imgInt(y, x + rect.Width);
            v = (imgInt2(y, x) + imgInt2(y + rect.Height, x + rect.Width) - imgInt2(y + rect.Height, x) - imgInt2(y, x + rect.Width)) * N - e * e;

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
                best = cur;
            }
        }
    }
    if (best > 0) {
        rect = rects[0];
        detected = true;
    }
    return detected;
}
