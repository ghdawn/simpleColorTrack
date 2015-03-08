#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QUdpSocket>
#include <QDebug>
#include <QPainter>
#include "itrbase.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    itr_protocol::StandSerialProtocol sspUdp;
    struct SensorData
    {
        F32 height;
        F32 x, y;
        S32 laserlength;
        S32 laser[780];
    };
private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;

    int row;
    int column;

    U8 tempbuff[65535];
    U8 imgbuffer[352*240*4];
    AVCodec *codec;
    AVCodecContext *dec;
    AVFrame *frame;
    QUdpSocket *ResultRec;
    QUdpSocket *colorRec;
    QString Lon,Lat,Alt;
    MainWindow::SensorData sensordata;
protected:
    bool eventFilter(QObject *obj, QEvent *e);

private slots:
    void processEstimateResultData();
    void processVisionData();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_btExit_clicked();
    void on_btPlay_clicked();
};

#endif // MAINWINDOW_H
