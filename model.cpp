// Copyright 2018 Victor Yudin. All rights reserved.

#include "model.h"

#include <QDebug>
#include <QFile>
#include <QStandardPaths>

Model::Model(const char* iFile)
{
    // In this simple case the points and the indexes already well formed. So
    // it's only necessary to read them from the text files.
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
