#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork>
#include <math.h>
#include "itrsystem.h"

const int ServerPort=9031;
const int RecPort=9032;

namespace Ui
{
itr_system::Udp udp;
itr_system::Udp::UdpPackage udppackage;
float x=0,y=0,Area=0,fps=0;
void Init()
{
    udppackage.IP="127.0.0.1";
    udppackage.port=ServerPort;
}

S32 SSPSend(U8* Buffer,S32 Length)
{
    udppackage.pbuffer=(char*)Buffer;
    udppackage.len=Length;
    udp.Send(udppackage);
    //sender->writeDatagram((char*)Buffer,Length,QHostAddress::Broadcast,ServerPort);
}

void SSPReceivefuc(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS,U8 *Package,S32 PackageLength)
{
        F32 *X,*Y,*A,*FPS;
        switch(Package[0])
            {
                case 0x40:
                //mode=Package[1];
                FPS=(F32*)&Package[2];
                X=(F32*)&Package[6];
                Y=(F32*)&Package[10];
                A=(F32*)&Package[14];
                x=*X;
                y=*Y;
                Area=*A;
                fps=*FPS;
                break;

            }
}
}
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->radarWidget->installEventFilter(this);
    setWindowTitle(tr("无人机视觉测试"));
    ui->doubleSpinBox->setRange(0.0,1000.0);
    ui->doubleSpinBox_2->setRange(0.0,1000.0);
    ui->doubleSpinBox_3->setRange(0.0,1000.0);
    ui->doubleSpinBox_4->setRange(0.0,1000.0);
    row=10;
    column=5;
    model=new QStandardItemModel(row,column);
    ui->tableView->setModel(model);
    ui->tableView->verticalHeader()->hide();
    model->setHeaderData(0,Qt::Horizontal,tr("目标"));
    model->setHeaderData(1,Qt::Horizontal,tr("帧率"));
    model->setHeaderData(2,Qt::Horizontal,tr("x"));
    model->setHeaderData(3,Qt::Horizontal,tr("y"));
    model->setHeaderData(4,Qt::Horizontal,tr("area"));
    sspUdp.Init(0xA5,0x5A,Ui::SSPSend);
    sspUdp.ProcessFunction[0]=&Ui::SSPReceivefuc;
    sender=new QUdpSocket(this);
    receiver=new QUdpSocket(this);
    //    receiver->bind(RecPort,QUdpSocket::ShareAddress);
    receiver->bind(RecPort);
    connect(receiver,SIGNAL(readyRead()),this,SLOT(processPendingDatagram()));
    avcodec_register_all();
    codec = avcodec_find_decoder(CODEC_ID_H264);
    dec = avcodec_alloc_context3(codec);
    if (avcodec_open2(dec, codec,NULL) < 0) {
        fprintf(stderr, "ERR: open H264 decoder err\n");
        exit(-1);
    }

    frame = avcodec_alloc_frame();

}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::processPendingDatagram()
{
    const int SSPLength=18+6;
    while(receiver->hasPendingDatagrams())
    {
        int Length=receiver->pendingDatagramSize();
        Length=receiver->readDatagram((char*)tempbuff,receiver->pendingDatagramSize());
        sspUdp.ProcessRawByte(tempbuff,SSPLength);
        int got;
        AVPacket pkt;
        pkt.data = tempbuff+SSPLength;
        pkt.size = Length-SSPLength;
        int ret = avcodec_decode_video2(dec, frame, &got, &pkt);
        if(ret<0)
            continue;
        int k=0;
        for(int j=0;j<240;j++)
        for(int i=0;i<320;i++)
        {
            imgbuffer[4*k  ]=frame->data[0][(i+j*352)];
            imgbuffer[4*k+1]=frame->data[0][(i+j*352)];
            imgbuffer[4*k+2]=frame->data[0][(i+j*352)];
            imgbuffer[4*k+3]=frame->data[0][(i+j*352)];
            k++;
        }
        ui->radarWidget->update();
        QString str=QString("x:%1,y:%2,fps%3\n").arg(Ui::x).arg(Ui::y).arg(Ui::fps);
        ui->label_2->setText(str);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if(obj==ui->radarWidget)
    {
        if(e->type()==QEvent::Paint)
        {
            QImage img=QImage(imgbuffer,320,240,QImage::Format_RGB32);
            QPainter painter(ui->radarWidget);
            painter.drawImage(QPoint(0,0),img);
            painter.setPen(Qt::red);
            painter.drawEllipse(QPoint(Ui::x,Ui::y),sqrt(Ui::Area)/2,sqrt(Ui::Area)/2);
            return true;
        }
    }
}

void MainWindow::draw()
{
}




void MainWindow::on_pushButton_clicked()
{
    U8 buffer[2]={0x41,0};
    sspUdp.SSPSendPackage(0,buffer,2);
}

void MainWindow::on_pushButton_2_clicked()
{
    U8 buffer[2]={0x41,0};
    sspUdp.SSPSendPackage(0,buffer,2);
}

void MainWindow::on_pushButton_3_clicked()
{
    U8 buffer[2]={0x41,0};
    sspUdp.SSPSendPackage(0,buffer,2);
}

void MainWindow::on_pushButton_4_clicked()
{
    //    QByteArray buffer;
    //    buffer.append(0x01);
    //    JzJl=ui->doubleSpinBox->value();
    //    jzjl=JzJl*100;
    //    buffer.append(jzjl);
    //    buffer.append(jzjl>>8);
    //    Jzx=ui->doubleSpinBox_2->value();
    //    jzx=Jzx*100;
    //    buffer.append(jzx);
    //    buffer.append(jzx>>8);
    //    Jzy=ui->doubleSpinBox_3->value();
    //    jzy=Jzy*100;
    //    buffer.append(jzy);
    //    buffer.append(jzy>>8);
    //    Jzd=ui->doubleSpinBox_4->value();
    //    jzd=Jzd*100;
    //    buffer.append(jzd);
    //    buffer.append(jzd>>8);
    //    sender->writeDatagram(buffer.data(),buffer.size(),QHostAddress::Broadcast,45454);
    //    buffer.clear();
}

