// Copyright 2018 Victor Yudin. All rights reserved.

#ifndef __MODEL_H_
#define __MODEL_H_

#include <vector>

class Model
{
public:
    Model(const char* iFile);

    float* data() { return mData.data(); }
    int points() { return mData.size() / 9; }

    unsigned int* indexData() { return mIndexData.data(); }
    int indexes() { return mIndexData.size(); }

private:
    std::vector<float> mData;
    std::vector<unsigned int> mIndexData;
};

#endif
