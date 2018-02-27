// Copyright 2018 Victor Yudin. All rights reserved.


/*******************************************************************************
        Created:	17:9:2002
        FileName: 	hdrloader.cpp
        Author:		Igor Kravtchenko

        Info:		Load HDR image and convert to a set of float32 RGB
triplet.
*******************************************************************************/

#include "hdrloader.h"

#include <QFile>
#include <math.h>
#include <memory.h>
#include <stdio.h>

typedef unsigned char RGBE[4];
#define R 0
#define G 1
#define B 2
#define E 3

#define MINELEN 8 // minimum scanline length for encoding
#define MAXELEN 0x7fff // maximum scanline length for encoding

static void workOnRGBE(RGBE* scan, int len, float* cols);
static bool decrunch(RGBE* scanline, int len, QFile& file);
static bool oldDecrunch(RGBE* scanline, int len, QFile& file);

char getChar(QFile& file)
{
    char c;
    file.read(&c, 1);
    return c;
}

HDRLoaderResult::HDRLoaderResult() : width(0), height(0), cols(nullptr)
{}

HDRLoaderResult::~HDRLoaderResult()
{
    delete cols;
}

bool HDRLoader::load(const char* fileName, HDRLoaderResult& res)
{
    int i;
    char str[200];
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    file.read(str, 10);
    if (memcmp(str, "#?RADIANCE", 10))
    {
        return false;
    }

    file.seek(file.pos() + 1);

    char cmd[200];
    i = 0;
    char c = 0, oldc;
    while (true)
    {
        oldc = c;
        c = getChar(file);
        if (c == 0xa && oldc == 0xa)
            break;
        cmd[i++] = c;
    }

    char reso[200];
    i = 0;
    while (true)
    {
        c = getChar(file);
        reso[i++] = c;
        if (c == 0xa)
            break;
    }

    int w, h;
    if (!sscanf(reso, "-Y %ld +X %ld", &h, &w))
    {
        return false;
    }

    res.width = w;
    res.height = h;

    float* cols = new float[w * h * 3];
    res.cols = cols;

    RGBE* scanline = new RGBE[w];

    // convert image
    for (int y = h - 1; y >= 0; y--)
    {
        if (decrunch(scanline, w, file) == false)
            break;
        workOnRGBE(scanline, w, cols);
        cols += w * 3;
    }

    delete[] scanline;

    return true;
}

float convertComponent(int expo, int val)
{
    float v = val / 256.0f;
    float d = (float)pow(2, expo);
    return pow(v * d, 1.0f / 2.2f);
}

void workOnRGBE(RGBE* scan, int len, float* cols)
{
    while (len-- > 0)
    {
        int expo = scan[0][E] - 128;
        cols[0] = convertComponent(expo, scan[0][R]);
        cols[1] = convertComponent(expo, scan[0][G]);
        cols[2] = convertComponent(expo, scan[0][B]);
        cols += 3;
        scan++;
    }
}

bool decrunch(RGBE* scanline, int len, QFile& file)
{
    int i, j;

    if (len < MINELEN || len > MAXELEN)
        return oldDecrunch(scanline, len, file);

    i = getChar(file);
    if (i != 2)
    {
        file.seek(file.pos() - 1);
        return oldDecrunch(scanline, len, file);
    }

    scanline[0][G] = getChar(file);
    scanline[0][B] = getChar(file);
    i = getChar(file);

    if (scanline[0][G] != 2 || scanline[0][B] & 128)
    {
        scanline[0][R] = 2;
        scanline[0][E] = i;
        return oldDecrunch(scanline + 1, len - 1, file);
    }

    // read each component
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < len;)
        {
            unsigned char code = getChar(file);
            if (code > 128)
            { // run
                code &= 127;
                unsigned char val = getChar(file);
                while (code--)
                    scanline[j++][i] = val;
            }
            else
            { // non-run
                while (code--)
                    scanline[j++][i] = getChar(file);
            }
        }
    }

    return !file.atEnd();
}

bool oldDecrunch(RGBE* scanline, int len, QFile& file)
{
    int i;
    int rshift = 0;

    while (len > 0)
    {
        scanline[0][R] = getChar(file);
        scanline[0][G] = getChar(file);
        scanline[0][B] = getChar(file);
        scanline[0][E] = getChar(file);
        if (file.atEnd())
            return false;

        if (scanline[0][R] == 1 && scanline[0][G] == 1 && scanline[0][B] == 1)
        {
            for (i = scanline[0][E] << rshift; i > 0; i--)
            {
                memcpy(&scanline[0][0], &scanline[-1][0], 4);
                scanline++;
                len--;
            }
            rshift += 8;
        }
        else
        {
            scanline++;
            len--;
            rshift = 0;
        }
    }
    return true;
}
