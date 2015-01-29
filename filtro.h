#ifndef FILTRO
#define FILTRO
#include <QList>
#include <QVector>
#include <QObject>

class filtro: public QObject
{
public:
      filtro(QObject *parent = 0);
     ~filtro();
public:
    float fMedian(QList<float> vector);
    float fAlgebraic_Estimation(QList<float> vector);
    float fAlgebraic_DerivateEstimation(QList<float> vector);

};
#endif // FILTRO

