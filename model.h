// Copyright 2018 Victor Yudin. All rights reserved.

#ifndef __MODEL_H_
#define __MODEL_H_

#include <vector>

/** @brief The object to read the external file and generate the data to create
 * a mesh. */
class Model
{
public:
    Model(const char* iFile);

    /**
     * @brief Returns the vertex data that is acceptable by OpenGL.
     *
     * @return The poiner to the data that contains point, normal and color for
     * each vertex.
     */
    float* data() { return mData.data(); }
    /**
     * @brief Returns the number of the points in the mesh.
     */
    int points() { return mData.size() / 9; }

    /**
     * @brief Returns the index data that is acceptable by OpenGL.
     *
     * @return The pointer to the indexes.
     */
    unsigned int* indexData() { return mIndexData.data(); }
    /**
     * @brief The number of the indexes.
     */
    int indexes() { return mIndexData.size(); }

private:
    std::vector<float> mData;
    std::vector<unsigned int> mIndexData;
};

#endif
