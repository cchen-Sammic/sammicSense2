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
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("yyyy_MMMdd hh_mm");
    ui->logFile->setText(dateTimeString);

    reduccionGyro = 131; //±250dps
    reduccionAccel = 16384; //±2g
    timeZero=0.0;
    playOn= false;
    saveLogOn = false;
    graphONag = false;
    flag_calculoGrafica_fin = true;
    ui->led->turnOff();

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

    serial = new QSerialPort(this);
//    timer = new QTimer(this);
//    timer->setInterval(5);

    setWindowTitle(tr("Sammic Sense"));
    qDebug()<<"empezamos!";
    qDebug()<<vectorTemp.size()<<vectorTemp.at(1);
    vectorTemp[1] =1.2;
    qDebug()<<vectorTemp.at(1);

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


    connect(ui->openCloseButton, SIGNAL(clicked()), this, SLOT(openSerialPort()));
    connect(serial, SIGNAL(readyRead()),this, SLOT(onSerialRead()));
    connect(ui->sendButton, SIGNAL(clicked()), SLOT(onSerialWrite()));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(clickedConnect()));
    connect(this, SIGNAL(logSignal()),this,SLOT(onLogReady()));


    setupRealtimeData(ui->customPlot,ui->customPlot2);

}



//! OPEN SERIAL PORT
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

//! SERIAL WRITE
void Dialog::onSerialWrite()
{
    if (serial->isOpen() && !ui->sendEdit->text().isEmpty())
        serial->write(ui->sendEdit->text().toLatin1());
    ui->sendEdit->clear();
}

//! SERIAL READ
void Dialog::onSerialRead()
{
    if (serial->bytesAvailable()) {
        ui->recvEdit->moveCursor(QTextCursor::End);
        datosPort = QString::fromLatin1(serial->readLine());
//        qDebug()<<datosPort;
//        onLogReady();

        if(playOn==false){
            ui->recvEdit->insertPlainText(datosPort);
        }
        else if(playOn==true){
//            datosPort = QString::fromLatin1(serial->readLine());
            onLogReady();
//            if(flag_calculoGrafica_fin==true){
//                emit logSignal();
//                flag_calculoGrafica_fin=false;
//            }

        }
    }

}

//![1] LETURA, CLASIFICACIÓN Y GUARDAR  PARÁMETROS
void Dialog::onLogReady()
{

    qDebug()<<listDatos;
    if(!datosPort.compare(datosPortTEMP)==0) // comparación de qstring i con i-1
    {
        listDatos = datosPort.split(",", QString::SkipEmptyParts);
//        ui->recvEditLog->moveCursor(QTextCursor::End);
        static QString datosLog;

        if (listDatos[0]=="$a/g" && listDatos.size()==8){
            graphONag=true;

            QStringList listDatosCopy = listDatos;
            listDatosCopy[0]=QString("%1, ").arg(listDatos[1]);
            listDatosCopy[1]=QString("%1, ").arg(listDatos[2]);
            listDatosCopy[2]=QString("%1, ").arg(listDatos[3]);
            listDatosCopy[3]=QString("%1, ").arg(listDatos[4]);
            listDatosCopy[4]=QString("%1, ").arg(listDatos[5]);
            listDatosCopy[5]=QString("%1, ").arg(listDatos[6]);
            listDatosCopy[6]=QString("%1").arg(key);
            //            listDatosCopy.removeLast();
            datosLog = listDatosCopy.join(" ");
//            qDebug()<<datosPort;
//            qDebug()<<datosLog;
//                        ui->recvEditLog->insertPlainText(listDatosCopy.join("   "));
        }
        else graphONag= false;

        //guardar en archivo

        if(saveLogOn)
        {
            QString directory = "../log/"+ui->logFile->text()+".txt";
            QFile file(directory);
            if(file.open(QIODevice::WriteOnly | QIODevice::Text| QIODevice::Append)){
                QTextStream out(&file);
                 qDebug()<<datosLog;
                out<<datosLog<<"\n";//datosPort
                file.close();
                datosPortTEMP = datosPort;
            }
        }

    }

}

//![2] ON/OFF para REPRESENTACIÓN GRÁFICA
void Dialog::clickedConnect()
{
    if (serial->isOpen())
    {
        if(playOn==false){
            serial->write("0");
            ui->playButton->setText("PLAY");
            timeZero = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0-4;
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

//![3] FILTRO DE LA MEDIANA
float Dialog::filterMedian(float valor, QList<float> vector)
{
    vector.prepend(valor);
    int vectorSize = vector.size();
    int posMedian;
    float returnValue;

    QVector<float> vectorTemp = vector.toVector();


    //ordenación de mayor a menor
    qStableSort(vectorTemp.begin(),vectorTemp.end());
    if(vectorSize%2==0){
        posMedian = vectorSize/2 ;
        returnValue= (vectorTemp.at(posMedian-1)+vectorTemp.at(posMedian))/2;
    }
    else
    {
        posMedian = static_cast<int>((vectorSize+1)/2);
        returnValue= vectorTemp.at(posMedian-1);
    }
    int boxValue = 20;
    if(vectorSize>=boxValue){
        vector.removeLast();
    }

    //condición umbral
    if(abs(valor-returnValue)> 1)
    {
        returnValue= valor;
    }


    //    vector->replace(0,returnValue);
    return returnValue;
}

//![4] FILTRO ESTIMACIÓN ALGEBRAICO [ Y ] DE M. FLIESS
float Dialog::filterAlgebraic_Estimation(float valor, QList<float> vector)
{
    vector.prepend(valor);
    int vectorSize = vector.size();
    float suma = 0;
    float tempSuma;
    float tempSumaPeso;
    int windowValue = 20;
    float stimation =0;


    for(int i=0;i<vectorSize;i++)
    {
        tempSumaPeso =float(2*vectorSize - 3*i);
        tempSuma = vector.at(i)*tempSumaPeso;

        /*Suma*/
        suma = tempSuma + tempSumaPeso+ suma;
    }

    if(vectorSize>=windowValue){
        stimation = 2*suma/(vectorSize*vectorSize);
        vector.removeLast();
    }
    return stimation;
}

//![5] FILTRO ESTIMACIÓN ALGEBRAICO [ Y' ] DE M. FLIESS
float Dialog::filterAlgebraic_DerivateEstimation(float valor, QList<float> vector)
{
    vector.prepend(valor);
    int vectorSize = vector.size();
    float suma = 0;
    float tempSuma;
    float tempSumaPeso;
    int windowValue = 20;
    float derivada = 0;

    /*Derivada M Fliess */
    for(int i=0;i<vectorSize;i++)
    {
        tempSumaPeso =float(vectorSize - 2*i);
        tempSuma = vector.at(i)*tempSumaPeso;

        /*Suma*/
        suma = tempSuma + tempSumaPeso+ suma;
    }



    if(vectorSize>=windowValue){
        derivada = -6*suma/(vectorSize*vectorSize*vectorSize*0.05);
        vector.removeLast();
    }
    return derivada;
}



//![6] DEFINICIÓN DE LOS PARAMETROS DE LAS GRÁFICAS
void Dialog::setupRealtimeData(QCustomPlot *customPlot,QCustomPlot *customPlot2)
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
    QMessageBox::critical(this, "", "You're using Qt < 4.7, the realtime data demo needs functions that are available with Qt 4.7 to work properly");
#endif
    demoName = "Real Time Data";
    qDebug()<<"dentro de setupRealTimeData()";
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


    customPlot->addGraph(); // blue dot
    customPlot->graph(3)->setPen(QPen(Qt::blue));
    customPlot->graph(3)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot->addGraph(); // red dot
    customPlot->graph(4)->setPen(QPen(Qt::red));
    customPlot->graph(4)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(4)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot->addGraph(); // green dot
    customPlot->graph(5)->setPen(QPen(Qt::green));
    customPlot->graph(5)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(5)->setScatterStyle(QCPScatterStyle::ssDisc);

    customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot->xAxis->setDateTimeFormat("mm:ss");
    customPlot->xAxis->setAutoTickStep(false);
    customPlot->xAxis->setTickStep(2);
    customPlot->xAxis2->setLabel("g-force      scale range ±2g");
    customPlot->yAxis->setRange(0.0,4.0, Qt::AlignCenter);
    customPlot->axisRect()->setupFullAxesBox();

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));


    customPlot2->addGraph(); // blue line
    customPlot2->graph(0)->setPen(QPen(Qt::blue));
    //  customPlot2->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
    customPlot2->graph(0)->setAntialiasedFill(false);
    customPlot2->addGraph(); // red line
    customPlot2->graph(1)->setPen(QPen(Qt::red));
    //  customPlot2->graph(0)->setChannelFillGraph(customPlot->graph(1));
    customPlot2->addGraph(); //green line
    customPlot2->graph(2)->setPen(QPen(Qt::green));
    //  customPlot2->graph(1)->setChannelFillGraph(customPlot->graph(2));

    customPlot2->addGraph(); // blue dot
    customPlot2->graph(3)->setPen(QPen(Qt::blue));
    customPlot2->graph(3)->setLineStyle(QCPGraph::lsNone);
    customPlot2->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot2->addGraph(); // red dot
    customPlot2->graph(4)->setPen(QPen(Qt::red));
    customPlot2->graph(4)->setLineStyle(QCPGraph::lsNone);
    customPlot2->graph(4)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot2->addGraph(); // green dot
    customPlot2->graph(5)->setPen(QPen(Qt::green));
    customPlot2->graph(5)->setLineStyle(QCPGraph::lsNone);
    customPlot2->graph(5)->setScatterStyle(QCPScatterStyle::ssDisc);

    customPlot2->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot2->xAxis->setDateTimeFormat("mm:ss");
    customPlot2->xAxis->setAutoTickStep(false);
    customPlot2->xAxis->setTickStep(2);
    customPlot2->xAxis2->setLabel("Angular rate º/s       scale range ±250dps");
    customPlot2->yAxis->setRange(0.0,720, Qt::AlignCenter);
    customPlot2->axisRect()->setupFullAxesBox();

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(customPlot2->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot2->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot2->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot2->yAxis2, SLOT(setRange(QCPRange)));



    // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    dataTimer.start(0); // Interval 0 means to refresh as fast as possible

}



//! VALORES DE LAS GRÁFICAS EN TIEMPO REAL
void Dialog::realtimeDataSlot()
{

    if (graphONag)  //(graphONag)
    {

        key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeZero;
        //// Gráfica 1
        value1_0 = (listDatos[1].toFloat())/reduccionAccel;
        value1_1 = (listDatos[2].toFloat())/reduccionAccel;
        value1_2 = (listDatos[3].toFloat())/reduccionAccel;

//        value1_0 = filterMedian(value1_0,vector_1);
//        value1_1 = filterMedian(value1_1,vector_2);
//        value1_2 = filterMedian(value1_2,vector_3);


//        value1_0 = filterAlgebraic_Estimation(value1_0,vector2_1);
//        value1_1 = filterAlgebraic_Estimation(value1_1,vector2_2);
//        value1_2 = filterAlgebraic_Estimation(value1_2,vector2_3);

        value1_0 = value1_0 - 1.045;
        value1_1 = value1_1 - 1.045;
        value1_2 = value1_2 - 1.08;

        // add data to lines:
        ui->customPlot->graph(0)->addData(key, value1_0);
        ui->customPlot->graph(1)->addData(key, value1_1);
        ui->customPlot->graph(2)->addData(key, value1_2);
        // set data of dots:
        ui->customPlot->graph(3)->clearData();
        ui->customPlot->graph(3)->addData(key, value1_0);
        ui->customPlot->graph(4)->clearData();
        ui->customPlot->graph(4)->addData(key, value1_1);
        ui->customPlot->graph(5)->clearData();
        ui->customPlot->graph(5)->addData(key, value1_2);
        // remove data of lines that's outside visible range:
        ui->customPlot->graph(0)->removeDataBefore(key-8);
        ui->customPlot->graph(1)->removeDataBefore(key-8);
        ui->customPlot->graph(2)->removeDataBefore(key-8);
        // rescale value (vertical) axis to fit the current data:
        //    ui->customPlot->graph(0)->rescaleValueAxis();
        //    ui->customPlot->graph(1)->rescaleValueAxis();
        //    ui->customPlot->graph(2)->rescaleValueAxis();

        //// Gráfica 2
        value2_0 = (listDatos[4].toFloat())/reduccionGyro;
        value2_1 = (listDatos[5].toFloat())/reduccionGyro;
        value2_2 = (listDatos[6].toFloat())/reduccionGyro;

//        value2_0 = filterAlgebraic_Estimation(value2_0,vector_4);
//        value2_1 = filterAlgebraic_Estimation(value2_1,vector_5);
//        value2_2 = filterAlgebraic_Estimation(value2_2,vector_6);

//        value2_0 = filterMedian(value2_0,vector_4);
//        value2_1 = filterMedian(value2_1,vector_5);
//        value2_2 = filterMedian(value2_2,vector_6);


        // add data to lines:
        ui->customPlot2->graph(0)->addData(key, value2_0);
        ui->customPlot2->graph(1)->addData(key, value2_1);
        ui->customPlot2->graph(2)->addData(key, value2_2);
        // set data of dots:
        ui->customPlot2->graph(3)->clearData();
        ui->customPlot2->graph(3)->addData(key, value2_0);
        ui->customPlot2->graph(4)->clearData();
        ui->customPlot2->graph(4)->addData(key, value2_1);
        ui->customPlot2->graph(5)->clearData();
        ui->customPlot2->graph(5)->addData(key, value2_2);
        // remove data of lines that's outside visible range:
        ui->customPlot2->graph(0)->removeDataBefore(key-8);
        ui->customPlot2->graph(1)->removeDataBefore(key-8);
        ui->customPlot2->graph(2)->removeDataBefore(key-8);
        // rescale value (vertical) axis to fit the current data:
        //    ui->customPlot2->graph(0)->rescaleValueAxis();
        //    ui->customPlot2->graph(1)->rescaleValueAxis();
        //    ui->customPlot2->graph(2)->rescaleValueAxis();


        // make key axis range scroll with the data (at a constant range size of 8):
        ui->customPlot->xAxis->setRange(key+0.15, 8, Qt::AlignRight);
        ui->customPlot->replot();

        // make key axis range scroll with the data (at a constant range size of 8):
        ui->customPlot2->xAxis->setRange(key+0.15, 8, Qt::AlignRight);
        ui->customPlot2->replot();

        ui->graph1name_1->setText(QString("Accel X"));
        ui->graph1name_2->setText(QString("Accel Y"));
        ui->graph1name_3->setText(QString("Accel Z"));
        ui->graph2name_1->setText(QString("Gyro X"));
        ui->graph2name_2->setText(QString("Gyro Y"));
        ui->graph2name_3->setText(QString("Gyro Z"));


        // calculate frames per second:
        static float lastFpsKey;
        static int frameCount;
        ++frameCount;
        float average = key-lastFpsKey;
        if (average > 2) // average fps over 1 seconds
        {
            ui->statusPlot->setReadOnly(true);
            ui->statusPlot->setText(
                        QString("  %1 FPS,  Total Data points: %2")
                        .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                        .arg(ui->customPlot->graph(0)->data()->count()+ui->customPlot->graph(1)->data()->count()+ui->customPlot->graph(2)->data()->count())
                        );
            lastFpsKey = key;
            frameCount = 0;
        }

    }
    flag_calculoGrafica_fin = true;
}

