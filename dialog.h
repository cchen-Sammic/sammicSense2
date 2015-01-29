#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include<QFile>
#include <QTimer>
#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include "qcustomplot.h"
#include "filtro.h"
#include <QList>

namespace Ui {
    class Dialog;
}
//class QTimer;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;
    QSerialPort *serial;
    filtro *filtroSenial;
    QString demoName;
    QTimer dataTimer;
    bool graphONag;
    bool playOn;
    bool saveLogOn;
    bool comparacionLog_OK;
    QStringList listDatos;
    QString datosPort;
    QString datosPortTEMP;
    QString datosLog_OK;

    QFile file;
    QList<float> vectorTemp;
    QList<float> vector_1;
    QList<float> vector_2;
    QList<float> vector_3;
    QList<float> vector_4;
    QList<float> vector_5;
    QList<float> vector_6;
    QList<float> vector2_1;
    QList<float> vector2_2;
    QList<float> vector2_3;
    int sensiGyro;
    int sensiAccel;
    float value1_0;
    float value1_1;
    float value1_2;
    float value2_0;
    float value2_1;
    float value2_2;
    float timeZero;
    float timeSecsCoordinate;
    float numReg;


private Q_SLOTS:
    void openSerialPort();
    void onSerialRead();
    void onSerialWrite();
    void onLogReady(); //lectura y clafisicación de tados
    void onSaveLogTXT(QString log);
    void realtimeDataSlot(); //gráfica
    void clickedConnect(); //botón conexión arduino

private:
        void initActionsConnections();
        void setupRealtimeData(QCustomPlot *customPlot, QCustomPlot *customPlot2);
        float filterAlgebraic_DerivateEstimation(QList<float>vector);
        float filterAlgebraic_Estimation(QList<float>vector);
        float filterMedian(QList<float> vector);

signals:
        void signalSaveLog(QString log);

};

#endif // DIALOG_H
