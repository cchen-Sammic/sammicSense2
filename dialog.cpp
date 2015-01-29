#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QtSerialPort>
#include "dialog.h"
#include "ui_dialog.h"
#include <QtCore>
#include <qthread.h>

#include <QFile>
#include <QDebug>



Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    filtroSenial = new filtro();
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("yyyy_MMMdd hh_mm");
    ui->logFile->setText(dateTimeString);

    sensiGyro = 131; //±250 º/s veloscidad angular: sensiGyro[LBS/(º/s)]
    sensiAccel = 16384; //±2g aceleración: sensiAccel[LSB/g]
    timeZero=0.0;
    numReg =  0.0;
    playOn= false;
    saveLogOn = false;
    graphONag = false;
    comparacionLog_OK = false;
    ui->led->turnOff();
    serial = new QSerialPort(this);

    QVector<float> setInitialValue;
    setInitialValue<<0.0<<0.0<<0.0<<0.0<<0.0<<0.0<<0.0<<0.0<<0.0<<0.0;
    vectorTemp = QList<float>::fromVector(setInitialValue);
    vector_1= QList<float>::fromVector(setInitialValue);;
    vector_2= QList<float>::fromVector(setInitialValue);;
    vector_3= QList<float>::fromVector(setInitialValue);;
    vector_4= QList<float>::fromVector(setInitialValue);;
    vector_5= QList<float>::fromVector(setInitialValue);;
    vector_6= QList<float>::fromVector(setInitialValue);;
    vector2_1= QList<float>::fromVector(setInitialValue);;
    vector2_2= QList<float>::fromVector(setInitialValue);;
    vector2_3= QList<float>::fromVector(setInitialValue);;


    setWindowTitle(tr("Sammic Sense"));
    qDebug()<<"empezamos!";
    initActionsConnections();

}

Dialog::~Dialog()
{
    if (serial->isOpen())
    {
        serial->write("9");
//        QThread::msleep(20);
    }
    delete ui;
    delete serial;
}

//! CONNECTIONs
void Dialog::initActionsConnections(){

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
        ui->portBox->addItem(info.portName());
        //        qDebug()<<info.portName();
    }
    ui->portBox->setCurrentIndex(ui->portBox->count()-1); //ui->portBox->count()-1
    ui->baudRateBox->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->baudRateBox->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->baudRateBox->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->baudRateBox->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->baudRateBox->setCurrentIndex(3);

    setupRealtimeData(ui->customPlot,ui->customPlot2);

    connect(ui->openCloseButton, SIGNAL(clicked()), this, SLOT(openSerialPort()));
    connect(serial, SIGNAL(readyRead()),this, SLOT(onSerialRead()));
    connect(ui->sendButton, SIGNAL(clicked()), SLOT(onSerialWrite()));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(clickedConnect()));
    connect(this, SIGNAL(signalSaveLog(QString)),this,SLOT(onSaveLogTXT(QString)));
}



//![0.0] OPEN SERIAL PORT
void Dialog::openSerialPort()
{
    if(!serial->isOpen()){
        serial->setPortName(ui->portBox->currentText());
        serial->setBaudRate(ui->baudRateBox->currentData().toInt());
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity( QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl( QSerialPort::NoFlowControl);
        serial->open(QIODevice::ReadWrite);
        qDebug()<<"comunicacion serial Ok";
    }
    else {
        qDebug()<<"Sin comunicacion serial";
        serial->close();
        ui->led->turnOff();
    }

    //update led's status
    ui->led->turnOn(serial->isOpen());
}

//![0.1] SERIAL WRITE
void Dialog::onSerialWrite()
{
    if (serial->isOpen() && !ui->sendEdit->text().isEmpty())
        serial->write(ui->sendEdit->text().toLatin1());
    ui->sendEdit->clear();
}

//![0.2] SERIAL READ
void Dialog::onSerialRead()
{
    if (serial->bytesAvailable()) {
        ui->recvEdit->moveCursor(QTextCursor::End);
        datosPort = QString::fromLatin1(serial->readLine());

        if(playOn==false){
            ui->recvEdit->insertPlainText(datosPort);
        }
        else if(playOn==true){
            onLogReady();
        }
    }
}

//![1.1] LETURA Y CLASIFICACIÓN
void Dialog::onLogReady()
{
    if(!datosPort.compare(datosPortTEMP)==0) // comparación de qstring i con i-1
    {
        comparacionLog_OK = false;
        listDatos = datosPort.split(",", QString::SkipEmptyParts);
//        ui->recvEditLog->moveCursor(QTextCursor::End);
        static QString datosLog;

        if (listDatos[0]=="$a/g" && listDatos.size()==8){
            graphONag=true;

            QStringList listDatosCopy = listDatos;
            listDatosCopy[0]=QString("%1,").arg(listDatos[1]);
            listDatosCopy[1]=QString("%1,").arg(listDatos[2]);
            listDatosCopy[2]=QString("%1,").arg(listDatos[3]);
            listDatosCopy[3]=QString("%1,").arg(listDatos[4]);
            listDatosCopy[4]=QString("%1,").arg(listDatos[5]);
            listDatosCopy[5]=QString("%1,").arg(listDatos[6]);
            listDatosCopy[6]=QString("%1").arg(timeSecsCoordinate);
            //            listDatosCopy.removeLast();
            datosLog = listDatosCopy.join(" ");
            if (datosLog!=datosLog_OK)
            {
                datosLog_OK = datosLog;
                comparacionLog_OK = true;
            }           
        }
        else graphONag= false;

        //GUARDAR en FICHERO TXT
        if(saveLogOn && comparacionLog_OK)
        {
            emit signalSaveLog(datosLog);
        }
    }
}

//![1.2] GUARDAR LOG EN FICHERO TXT
void Dialog::onSaveLogTXT(QString log){
    QString directory = "../log/raw "+ui->logFile->text()+".txt";
    QFile file(directory);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text| QIODevice::Append)){
        QTextStream out(&file);
        out<<numReg<<","<<log<<"\n";
        file.close();
        datosPortTEMP = datosPort;
        numReg = numReg +1;
    }
}

//![1.3] CARGAR LOG DE FICHERO TXT


//![2] ON/OFF para REPRESENTACIÓN GRÁFICA
void Dialog::clickedConnect()
{
    if (serial->isOpen())
    {
        if(playOn==false){
            serial->write("0");
            ui->playButton->setText("PLAY");
            timeZero = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 -60;
            playOn= true;
            if(ui->saveBox->isChecked())
            {
                saveLogOn= true;
            }
        }
        else
        {
            serial->write("1");
            ui->playButton->setText("PAUSE");
            playOn= false;
        }
    }
}


//![3.1] DEFINICIÓN DE LOS PARAMETROS DE LAS GRÁFICAS
void Dialog::setupRealtimeData(QCustomPlot *customPlot,QCustomPlot *customPlot2)
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
    QMessageBox::critical(this, "", "You're using Qt < 4.7, the realtime data demo needs functions that are available with Qt 4.7 to work properly");
#endif
    demoName = "Real Time Data";
//    qDebug()<<"dentro de setupRealTimeData()";

    if(true)/*GRAFICA ACELERACION*/
    {
        customPlot->addGraph(); // blue line
        customPlot->graph(0)->setPen(QPen(Qt::blue));
        //  customPlot->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
        customPlot->graph(0)->setAntialiasedFill(false);
        customPlot->addGraph(); // red line
        customPlot->graph(1)->setPen(QPen(Qt::red));
        //  customPlot->graph(0)->setChannelFillGraph(customPlot->graph(1));
        customPlot->addGraph(); //green line
        customPlot->graph(2)->setPen(QPen(Qt::green));
        //  customPlot->graph(1)->setChannelFillGraph(customPlot->graph(2));
        customPlot->addGraph(); // BLUE dot
        customPlot->graph(3)->setPen(QPen(Qt::blue));
        customPlot->graph(3)->setLineStyle(QCPGraph::lsNone);
        customPlot->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);
        customPlot->addGraph(); // RED dot
        customPlot->graph(4)->setPen(QPen(Qt::red));
        customPlot->graph(4)->setLineStyle(QCPGraph::lsNone);
        customPlot->graph(4)->setScatterStyle(QCPScatterStyle::ssDisc);
        customPlot->addGraph(); // GREEN dot
        customPlot->graph(5)->setPen(QPen(Qt::green));
        customPlot->graph(5)->setLineStyle(QCPGraph::lsNone);
        customPlot->graph(5)->setScatterStyle(QCPScatterStyle::ssDisc);
        customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
        customPlot->xAxis->setDateTimeFormat("mm:ss");
        customPlot->xAxis->setAutoTickStep(false);
        customPlot->xAxis->setTickStep(2);
        customPlot->xAxis2->setLabel("g-force      scale range ±2g");
        customPlot->yAxis->setRange(0.0,10.0, Qt::AlignCenter);
        customPlot->axisRect()->setupFullAxesBox();
        // make left and bottom axes transfer their ranges to right and top axes:
        connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
        connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
    }

    if(true) /*GRAFICA VELOCIDAD ANGULAR*/
    {
        customPlot2->addGraph(); // BLUE line
        customPlot2->graph(0)->setPen(QPen(Qt::blue));
        //  customPlot2->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
        customPlot2->graph(0)->setAntialiasedFill(false);
        customPlot2->addGraph(); // RED line
        customPlot2->graph(1)->setPen(QPen(Qt::red));
        //  customPlot2->graph(0)->setChannelFillGraph(customPlot->graph(1));
        customPlot2->addGraph(); //GREEN line
        customPlot2->graph(2)->setPen(QPen(Qt::green));
        //  customPlot2->graph(1)->setChannelFillGraph(customPlot->graph(2));
        customPlot2->addGraph(); // BLUE dot
        customPlot2->graph(3)->setPen(QPen(Qt::blue));
        customPlot2->graph(3)->setLineStyle(QCPGraph::lsNone);
        customPlot2->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);
        customPlot2->addGraph(); // RED dot
        customPlot2->graph(4)->setPen(QPen(Qt::red));
        customPlot2->graph(4)->setLineStyle(QCPGraph::lsNone);
        customPlot2->graph(4)->setScatterStyle(QCPScatterStyle::ssDisc);
        customPlot2->addGraph(); // GREEN dot
        customPlot2->graph(5)->setPen(QPen(Qt::green));
        customPlot2->graph(5)->setLineStyle(QCPGraph::lsNone);
        customPlot2->graph(5)->setScatterStyle(QCPScatterStyle::ssDisc);
        customPlot2->xAxis->setTickLabelType(QCPAxis::ltDateTime);
        customPlot2->xAxis->setDateTimeFormat("mm:ss");
        customPlot2->xAxis->setAutoTickStep(false);
        customPlot2->xAxis->setTickStep(2);
        customPlot2->xAxis2->setLabel("          Angular rate º/s       scale range ±250dps");
        customPlot2->yAxis->setRange(0.0,1800, Qt::AlignCenter);
        customPlot2->axisRect()->setupFullAxesBox();
        // make left and bottom axes transfer their ranges to right and top axes:
        connect(customPlot2->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot2->xAxis2, SLOT(setRange(QCPRange)));
        connect(customPlot2->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot2->yAxis2, SLOT(setRange(QCPRange)));
    }

    ui->graph1name_1->setText(QString("Accel X"));
    ui->graph1name_2->setText(QString("Accel Y"));
    ui->graph1name_3->setText(QString("Accel Z"));
    ui->graph2name_1->setText(QString("Gyro X"));
    ui->graph2name_2->setText(QString("Gyro Y"));
    ui->graph2name_3->setText(QString("Gyro Z"));

    // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    dataTimer.start(0); // Interval 0 means to refresh as fast as possible
}

//![3.2] VALORES DE LAS GRÁFICAS EN TIEMPO REAL
void Dialog::realtimeDataSlot()
{
    if (graphONag && comparacionLog_OK)
    {
        timeSecsCoordinate = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeZero;
        //// Gráfica 1
        if(true) //VECTOR ACELERACIÓN MUESTREO
        {
            value1_0 = (listDatos[1].toFloat());
            value1_1 = (listDatos[2].toFloat());
            value1_2 = (listDatos[3].toFloat());
            vector_1.prepend(value1_0);
            vector_2.prepend(value1_1);
            vector_3.prepend(value1_2);
            if (vector_1.size()>15)
            {
                vector_1.removeLast();
                vector_2.removeLast();
                vector_3.removeLast();
            }
        }
        if(true){//FILTRO SEÑAL MEDIANA
            value1_0 = filtroSenial->fMedian(vector_1);
            value1_1 = filtroSenial->fMedian(vector_2);
            value1_2 = filtroSenial->fMedian(vector_3);
            vector_1[0]=value1_0;
            vector_2[0]=value1_1;
            vector_3[0]=value1_2;
        }
        if(true){//FILTRO SEÑAL ALGEBRAICA ESTIMACION
            value1_0 = filtroSenial->fAlgebraic_Estimation(vector_1);
            value1_1 = filtroSenial->fAlgebraic_Estimation(vector_2);
            value1_2 = filtroSenial->fAlgebraic_Estimation(vector_3);
            vector_1[0]=value1_0;
            vector_2[0]=value1_1;
            vector_3[0]=value1_2;
        }
        value1_0 = value1_0/sensiAccel;
        value1_1 = value1_1/sensiAccel;
        value1_2 = value1_2/sensiAccel;
        if(true)//DIBUJAR GRAFICA 1: ACELERACION
        {
            // add data to lines:
            ui->customPlot->graph(0)->addData(timeSecsCoordinate, value1_0);
            ui->customPlot->graph(1)->addData(timeSecsCoordinate, value1_1);
            ui->customPlot->graph(2)->addData(timeSecsCoordinate, value1_2);
            // set data of dots:
            ui->customPlot->graph(3)->clearData();
            ui->customPlot->graph(3)->addData(timeSecsCoordinate, value1_0);
            ui->customPlot->graph(4)->clearData();
            ui->customPlot->graph(4)->addData(timeSecsCoordinate, value1_1);
            ui->customPlot->graph(5)->clearData();
            ui->customPlot->graph(5)->addData(timeSecsCoordinate, value1_2);
            // remove data of lines that's outside visible range:
            ui->customPlot->graph(0)->removeDataBefore(timeSecsCoordinate-8);
            ui->customPlot->graph(1)->removeDataBefore(timeSecsCoordinate-8);
            ui->customPlot->graph(2)->removeDataBefore(timeSecsCoordinate-8);
            // rescale value (vertical) axis to fit the current data:
            //    ui->customPlot->graph(0)->rescaleValueAxis();
            //    ui->customPlot->graph(1)->rescaleValueAxis();
            //    ui->customPlot->graph(2)->rescaleValueAxis();
        }

        //// Gráfica 2
        if(true) //VECTOR VELOCIDAD ANGULAR MUESTREO
        {
            value2_0 = (listDatos[4].toFloat());
            value2_1 = (listDatos[5].toFloat());
            value2_2 = (listDatos[6].toFloat());
            vector_4.prepend(value2_0);
            vector_5.prepend(value2_1);
            vector_6.prepend(value2_2);
            if (vector_4.size()>15)
            {
                vector_4.removeLast();
                vector_5.removeLast();
                vector_6.removeLast();
            }
        }
        if(true) //FILTRO SEÑAL DE LA MEDIANA
        {
            value2_0 = filtroSenial->fMedian(vector_4);
            value2_1 = filtroSenial->fMedian(vector_5);
            value2_2 = filtroSenial->fMedian(vector_6);
            vector_4[0] = value2_0;
            vector_5[0] = value2_1;
            vector_6[0] = value2_2;
        }
        if(true)// FILTRO SEÑAL ALGEBRAICA ESTIMACIÓN
        {
            value2_0 = filtroSenial->fAlgebraic_Estimation(vector_4);
            value2_1 = filtroSenial->fAlgebraic_Estimation(vector_5);
            value2_2 = filtroSenial->fAlgebraic_Estimation(vector_6);
            vector_4[0] = value2_0;
            vector_5[0] = value2_1;
            vector_6[0] = value2_2;
        }
        value2_0 = value2_0/sensiGyro;
        value2_1 = value2_1/sensiGyro;
        value2_2 = value2_2/sensiGyro;
        if(true)//DIBUJAR GRAFICA 2: VELOCIDAD ANGULAR
        {
            // add data to lines:
            ui->customPlot2->graph(0)->addData(timeSecsCoordinate, value2_0);
            ui->customPlot2->graph(1)->addData(timeSecsCoordinate, value2_1);
            ui->customPlot2->graph(2)->addData(timeSecsCoordinate, value2_2);
            // set data of dots:
            ui->customPlot2->graph(3)->clearData();
            ui->customPlot2->graph(3)->addData(timeSecsCoordinate, value2_0);
            ui->customPlot2->graph(4)->clearData();
            ui->customPlot2->graph(4)->addData(timeSecsCoordinate, value2_1);
            ui->customPlot2->graph(5)->clearData();
            ui->customPlot2->graph(5)->addData(timeSecsCoordinate, value2_2);
            // remove data of lines that's outside visible range:
            ui->customPlot2->graph(0)->removeDataBefore(timeSecsCoordinate-8);
            ui->customPlot2->graph(1)->removeDataBefore(timeSecsCoordinate-8);
            ui->customPlot2->graph(2)->removeDataBefore(timeSecsCoordinate-8);
            // rescale value (vertical) axis to fit the current data:
            //    ui->customPlot2->graph(0)->rescaleValueAxis();
            //    ui->customPlot2->graph(1)->rescaleValueAxis();
            //    ui->customPlot2->graph(2)->rescaleValueAxis();
        }

        //// Desplazamiento de eje X: desplazamiento en tiempo real
        // make timeSecsCoordinate axis range scroll with the data (at a constant range size of 8):
        ui->customPlot->xAxis->setRange(timeSecsCoordinate+0.15, 8, Qt::AlignRight);
        ui->customPlot->replot();
        // make timeSecsCoordinate axis range scroll with the data (at a constant range size of 8):
        ui->customPlot2->xAxis->setRange(timeSecsCoordinate+0.15, 8, Qt::AlignRight);
        ui->customPlot2->replot();

        //// Calculo de FPS y datos por gráfica
        if(true) // CALCULATE FRAMES PER SECOND:
        { static float lastFpsKey;
            static int frameCount;
            ++frameCount;
            float average = timeSecsCoordinate-lastFpsKey;
            if (average > 2) // average fps over 1 seconds
            {
                ui->statusPlot->setReadOnly(true);
                ui->statusPlot->setText(
                            QString("  %1 FPS,   %2 puntos/gráfica")
                            .arg(frameCount/(timeSecsCoordinate-lastFpsKey), 0, 'f', 0)
                            .arg(ui->customPlot->graph(0)->data()->count()+ui->customPlot->graph(1)->data()->count()+ui->customPlot->graph(2)->data()->count())
                            );
                lastFpsKey = timeSecsCoordinate;
                frameCount = 0;
            }
        }
    }
}

