#ifndef __MODEL_H_
#define __MODEL_H_

#include <vector>

class Model
{
public:
    Model(const char* iFile);

    float* data() { return mData.data(); }
    int points() { return mData.size() / 6; }

private:
    std::vector<float> mData;
};

#endif
