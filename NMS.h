#pragma once
#include "utils.h"
#include <vector>


namespace NMS {
    void NMS(std::vector<BBox>& boxes, const float& threshold);

    float CalcOverlap(const BBox& box1,const BBox& box2);
}