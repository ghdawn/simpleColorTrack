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
S32 SSPSend(U8* Buffer,S32 Length);
void SSPReceivefuc(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS,U8 *Package,S32 PackageLength);
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    itr_protocol::StandSerialProtocol sspUdp;

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;

    int row;
    int column;
    void draw();

    U8 tempbuff[65535];
    U8 imgbuffer[352*240*4];
    AVCodec *codec;
    AVCodecContext *dec;
    AVFrame *frame;
    QUdpSocket *sender;
    QUdpSocket *receiver;
    S32 SSPSend(U8* Buffer,S32 Length);
    void SSPReceivefuc(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS,U8 *Package,S32 PackageLength);
protected:
    bool eventFilter(QObject *obj, QEvent *e);

private slots:
    void processPendingDatagram();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
};

#endif // MAINWINDOW_H
