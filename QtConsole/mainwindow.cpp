#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork>
#include <math.h>
#include "itrsystem.h"

const int ServerPort=9031;
const int RecPort=9032;
float pos_x,pos_y,Area,fps;
int mode;

class SSPReceivefuc:public itr_protocol::StandSerialProtocol::SSPDataRecFun
{
    void Do(itr_protocol::StandSerialProtocol *SSP, itr_protocol::StandSerialFrameStruct *SSFS,U8 *Package,S32 PackageLength)
    {
            F32 *X,*Y,*A,*FPS;
            switch(Package[0])
                {
                    case 0x40:
                    mode=Package[1];
                    FPS=(F32*)&Package[2];
                    X=(F32*)&Package[6];
                    Y=(F32*)&Package[10];
                    A=(F32*)&Package[14];
                    pos_x=*X;
                    pos_y=*Y;
                    Area=*A;
                    fps=*FPS;
                    /*
                     * printf("%f %f%f %f\n",x,y,Area,fps);
                    FILE* fout=fopen("pos.txt","a");
                    fprintf(fout,"%f %f %f %f\n",x,y,Area,fps);
                    fclose(fout)*/;
                    break;

                }
    }
};

class SSPSend: public itr_protocol::StandSerialProtocol::SSPDataSendFun
{    
public:
    itr_system::Udp udp;
    itr_system::Udp::UdpPackage udppackage;
    SSPSend()
    {
        udppackage.IP="192.168.199.251";
        udppackage.port=ServerPort;
    }
    S32 Do(U8* Buffer,S32 Length)
    {
        udppackage.pbuffer=(char*)Buffer;
        udppackage.len=Length;
        udp.Send(udppackage);
        //sender->writeDatagram((char*)Buffer,Length,QHostAddress::Broadcast,ServerPort);
    }
};

SSPSend sspSend;
SSPReceivefuc sspRec;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->radarWidget->installEventFilter(this);
    setWindowTitle(tr("无人机视觉测试"));
    ui->doubleSpinBox->setRange(0.0,10.0);
    ui->doubleSpinBox_2->setRange(0.0,10.0);
    ui->doubleSpinBox_3->setRange(0.0,10.0);
    ui->doubleSpinBox_4->setRange(0.0,10.0);
    FILE* fin=fopen("para.data","r");
    if(fin>0)
    {
        F32 px,dx,py,dy;
        fscanf(fin,"%f %f %f %f",&px,&dx,&py,&dy);
        fclose(fin);
        ui->doubleSpinBox->setValue(px);
        ui->doubleSpinBox_2->setValue(dx);
        ui->doubleSpinBox_3->setValue(py);
        ui->doubleSpinBox_4->setValue(dy);
    }


    sspUdp.Init(0xA5,0x5A,&sspSend);
    sspUdp.AddDataRecFunc(&sspRec,0);
    sender=new QUdpSocket(this);
    receiver=new QUdpSocket(this);
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
    static int count=0;
    char filename[]="img00000000.pgm";
    while(receiver->hasPendingDatagrams())
    {
        int Length=receiver->pendingDatagramSize();
        Length=receiver->readDatagram((char*)tempbuff,receiver->pendingDatagramSize());
        sspUdp.ProcessRawByte(tempbuff,SSPLength);
        QString str=QString("x:%1,y:%2\nfps:%3\n").arg(pos_x).arg(pos_y).arg(fps);
        ui->label_2->setText(str);
        int got;
        AVPacket pkt;
        pkt.data = tempbuff+SSPLength;
        pkt.size = Length-SSPLength;
        sprintf(filename,"x264/x264%06d.x264",count);
        FILE* fx264=fopen(filename,"w");
        for(int i=0;i<pkt.size;i++)
            fprintf(fx264,"%c",pkt.data[i]);
        fclose(fx264);
        int ret = avcodec_decode_video2(dec, frame, &got, &pkt);
        if(got<=0)
            continue;
        int k=0;
        sprintf(filename,"img/img%06d.pgm",count++);
        FILE* fp=fopen(filename,"w");
        fprintf(fp,"P5\n320 240\n255\n");
        for(int j=0;j<240;j++)
            for(int i=0;i<320;i++)
            {
                fprintf(fp,"%c",frame->data[0][i+j*352]);
                imgbuffer[4*k  ]=frame->data[0][(i+j*352)];
                imgbuffer[4*k+1]=frame->data[0][(i+j*352)];
                imgbuffer[4*k+2]=frame->data[0][(i+j*352)];
                imgbuffer[4*k+3]=frame->data[0][(i+j*352)];
                k++;
            }
        fclose(fp);
        ui->radarWidget->update();

    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    char filename[30];
    static int count2=0;
    const int radius=20;
    const int crosslength=8;
    if(obj==ui->radarWidget)
    {
        if(e->type()==QEvent::Paint)
        {
            QImage img=QImage(imgbuffer,320,240,QImage::Format_RGB32);
            QPainter painter(ui->radarWidget);
            painter.drawImage(QPoint(0,0),img);
            painter.setPen(Qt::red);
            if(mode==2)
            {
                painter.drawRect(pos_x-radius,pos_y-radius,radius+radius,radius+radius);
                painter.drawLine(pos_x-crosslength,pos_y,pos_x+crosslength,pos_y);
                painter.drawLine(pos_x,pos_y-crosslength,pos_x,pos_y+crosslength);
                sprintf(filename,"img%04d.png",count2++);
                img.save(filename);
            }
            else
                painter.drawRect(140,100,40,40);
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
    U8 buffer[2]={0x41,1};
    sspUdp.SSPSendPackage(0,buffer,2);
}

void MainWindow::on_pushButton_3_clicked()
{
    U8 buffer[2]={0x41,2};
    sspUdp.SSPSendPackage(0,buffer,2);
}

void MainWindow::on_pushButton_4_clicked()
{
    U8 buffer[9]={0x42};
    int pos=1;
    F32 data=ui->doubleSpinBox->value();
    U8* pData=(U8*)&data;
    for(int i=0;i<4;i++)
        buffer[pos++]=pData[i];
    data=ui->doubleSpinBox_2->value();
    for(int i=0;i<4;i++)
        buffer[pos++]=pData[i];
    data=ui->doubleSpinBox_3->value();
    for(int i=0;i<4;i++)
        buffer[pos++]=pData[i];
    data=ui->doubleSpinBox_4->value();
    for(int i=0;i<4;i++)
        buffer[pos++]=pData[i];
    sspUdp.SSPSendPackage(0,buffer,pos);
    FILE* fout=fopen("para.data","w");
    fprintf(fout,"%f %f %f %f",ui->doubleSpinBox->value(),
            ui->doubleSpinBox_2->value(),ui->doubleSpinBox_3->value(),ui->doubleSpinBox_4->value());
    fclose(fout);
}


void MainWindow::on_pushButton_5_clicked()
{
    sspSend.udppackage.IP=ui->lineEdit->text().toStdString();
}
