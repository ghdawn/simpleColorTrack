#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QUdpSocket>
#include <QDebug>
#include <QPainter>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    quint16 U8toU16(quint16 x, quint16 y);
private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    QUdpSocket *sender;
    QUdpSocket *receiver;
    int row;
    int column;
    void draw();
    quint16 zl;
    quint16 x_plot;
    quint16 y_plot;
    quint16 d_plot;
    double Zl;
    double X_plot;
    double Y_plot;
    double D_plot;
    quint16 jzjl;
    quint16 jzx;
    quint16 jzy;
    quint16 jzd;
    double JzJl;
    double Jzx;
    double Jzy;
    double Jzd;
    int number;
protected:
    bool eventFilter(QObject *obj, QEvent *e);
private slots:
    void processPendingDatagram();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
};

#endif // MAINWINDOW_H
