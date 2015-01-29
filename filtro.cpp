#include "filtro.h"


filtro::filtro (QObject *parent)
    : QObject(parent)
{

}
filtro::~filtro(){

}

//! FILTRO DE LA MEDIANA
float filtro::fMedian(QList<float> vector)
{
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
    //condición umbral
    if(abs(vector.at(0)-returnValue)> 1)
    {
        returnValue= vector.at(0);
    }
    return returnValue;
}

//! FILTRO ESTIMACIÓN ALGEBRAICO [ Y ] DE M. FLIESS
float filtro::fAlgebraic_Estimation(QList<float> vector)
{
    int vectorSize = vector.size();
    float suma = 0;
    float tempSuma;
    float tempSumaPeso;
    int minWindowValue = 5;
    float stimation =0;

    for(int i=0;i<vectorSize;i++)
    {
        tempSumaPeso =float(2*vectorSize - 3*i);
        tempSuma = vector.at(i)*tempSumaPeso;

        /*Suma*/
        suma = tempSuma + tempSumaPeso+ suma;
    }

    if(vectorSize>=minWindowValue){
        stimation = 2*suma/(vectorSize*vectorSize);
    }
    return stimation;
}

//! FILTRO ESTIMACIÓN ALGEBRAICO [ Y' ] DE M. FLIESS
float filtro::fAlgebraic_DerivateEstimation(QList<float> vector)
{
    int vectorSize = vector.size();
    float suma = 0;
    float tempSuma;
    float tempSumaPeso;
    int minWindowValue = 5;
    float derivada = 0;

    /*Derivada M Fliess */
    for(int i=0;i<vectorSize;i++)
    {
        tempSumaPeso =float(vectorSize - 2*i);
        tempSuma = vector.at(i)*tempSumaPeso;

        /*Suma*/
        suma = tempSuma + tempSumaPeso+ suma;
    }

    if(vectorSize>=minWindowValue){
        derivada = -6*suma/(vectorSize*vectorSize*vectorSize*0.05);
    }
    return derivada;
}
