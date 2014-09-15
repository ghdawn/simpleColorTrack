#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork>

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
    sender=new QUdpSocket(this);
    receiver=new QUdpSocket(this);
    receiver->bind(45454,QUdpSocket::ShareAddress);
    connect(receiver,SIGNAL(readyRead()),this,SLOT(processPendingDatagram()));
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::processPendingDatagram()
{
    while(receiver->hasPendingDatagrams())
    {   ui->radarWidget->update();
        model->removeRows(0,model->rowCount());
        model->setRowCount(row);
        model->setColumnCount(column);
        QByteArray datagram;
        datagram.resize(receiver->pendingDatagramSize());
        receiver->readDatagram(datagram.data(),datagram.size());
        if(!datagram.isEmpty()){
                QByteArray buf;
                ui->textBrowser->setTextColor(Qt::black);
                    for(int i = 0; i < datagram.count(); i++){
                        QString s;
                        s.sprintf("0x%02x, ", (unsigned char)datagram.at(i));
                        buf += s;
                        }
                    ui->textBrowser->setText(ui->textBrowser->document()->toPlainText() + buf);
                    number=datagram.at(0);
                    zl=U8toU16(datagram.at(2),datagram.at(1));
                    x_plot=U8toU16(datagram.at(4),datagram.at(3));
                    y_plot=U8toU16(datagram.at(6),datagram.at(5));
                    d_plot=U8toU16(datagram.at(8),datagram.at(7));
                    Zl=zl/100.0;
                    X_plot=x_plot/100.0;
                    Y_plot=y_plot/100.0;
                    D_plot=d_plot/100.0;
                    for(int ti=0;ti<1;ti++)
                    {   QString str;
                        QStandardItem *itemTarget=new QStandardItem(QString::number(ti+1));
                        model->setItem(ti,0,itemTarget);
                        QStandardItem *itemFloat1=new QStandardItem(str.sprintf("%0.2f",Zl));
                        QStandardItem *itemFloat2=new QStandardItem(str.sprintf("%0.2f",X_plot));
                        QStandardItem *itemFloat3=new QStandardItem(str.sprintf("%0.2f",Y_plot));
                        QStandardItem *itemFloat4=new QStandardItem(str.sprintf("%0.2f",D_plot));
                        model->setItem(ti,1,itemFloat1);
                        model->setItem(ti,2,itemFloat2);
                        model->setItem(ti,3,itemFloat3);
                        model->setItem(ti,4,itemFloat4);
                    }
    }
    ui->tableView->update();
}
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
     if(obj==ui->radarWidget)
     {
         if(e->type()==QEvent::Paint)
         {
             draw();
             ui->label_2->setText(tr("正在画点"));
             return true;
         }
         else
         {
             ui->label_2->setText(tr("等待中"));
         }
     }
     return QMainWindow::eventFilter(obj,e);
}

void MainWindow::draw()
{
    QPainter painter(ui->radarWidget);
    QPixmap pix;
    pix.load("C:/Users/songgang/Desktop/gh.jpg");
    painter.drawPixmap(-100,-100,500,500,pix);
    painter.setBrush(Qt::red);
    painter.drawEllipse(X_plot,Y_plot,D_plot,D_plot);
}




void MainWindow::on_pushButton_clicked()
{
    QByteArray buffer;
    buffer.append(0x41);
    buffer.append((char)0x00);
    sender->writeDatagram(buffer.data(),buffer.size(),QHostAddress::Broadcast,45454);
    buffer.clear();
}

void MainWindow::on_pushButton_2_clicked()
{
    QByteArray buffer;
    buffer.append(0x41);
    buffer.append(0x01);
    sender->writeDatagram(buffer.data(),buffer.size(),QHostAddress::Broadcast,45454);
    buffer.clear();
}

void MainWindow::on_pushButton_3_clicked()
{
    QByteArray buffer;
    buffer.append(0x41);
    buffer.append(0x02);
    sender->writeDatagram(buffer.data(),buffer.size(),QHostAddress::Broadcast,45454);
    buffer.clear();
}

void MainWindow::on_pushButton_4_clicked()
{
    QByteArray buffer;
    buffer.append(0x01);
    JzJl=ui->doubleSpinBox->value();
    jzjl=JzJl*100;
    buffer.append(jzjl);
    buffer.append(jzjl>>8);
    Jzx=ui->doubleSpinBox_2->value();
    jzx=Jzx*100;
    buffer.append(jzx);
    buffer.append(jzx>>8);
    Jzy=ui->doubleSpinBox_3->value();
    jzy=Jzy*100;
    buffer.append(jzy);
    buffer.append(jzy>>8);
    Jzd=ui->doubleSpinBox_4->value();
    jzd=Jzd*100;
    buffer.append(jzd);
    buffer.append(jzd>>8);
    sender->writeDatagram(buffer.data(),buffer.size(),QHostAddress::Broadcast,45454);
    buffer.clear();
}
quint16 MainWindow::U8toU16(quint16 x,quint16 y)
{
    return((x<<8)|y);
}
