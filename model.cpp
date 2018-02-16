#include "model.h"

#include <QDebug>
#include <QFile>
#include <QStandardPaths>

Model::Model(const char* iFile)
{
    QFile modelDataFile(QString(iFile) + ".data.cache");

    if (!modelDataFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug("Can't read file");
        qDebug(iFile);
        return;
    }

    QFile modelIndexFile(QString(iFile) + ".index.cache");

    if (!modelIndexFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug("Can't read file");
        qDebug(iFile);
        return;
    }

    QTextStream modelDataStream(&modelDataFile);
    while (!modelDataStream.atEnd())
    {
        QString line = modelDataStream.readLine();
        mData.push_back(line.toFloat());
    }
    modelDataFile.close();

    QTextStream modelIndexStream(&modelIndexFile);
    while (!modelIndexStream.atEnd())
    {
        QString line = modelIndexStream.readLine();
        mIndexData.push_back(line.toInt());
    }
    modelIndexFile.close();

    return;
}
