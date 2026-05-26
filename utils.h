#pragma once
#include <opencv2/core/types.hpp>
#include <algorithm>
#include <vector>



#define LOGI(tag, str) std::cout<< "(I) " << tag << " : " << str << std::endl


struct Vec2i{
    int x;
    int y;
};


struct Recti {
    int x;
    int y;
    int width;
    int height;
};

struct RGB {
    uint8_t R,G,B;
};


struct BBox{
    cv::Point2f _lt;
    cv::Point2f _br;
    float _prob = 0;
};


// Struct to hold final coordinate results
struct Keypoint {
    float x; float prob_x;
    float y; float prob_y;
};





namespace NMS{
    static inline float CalcOverlap(const BBox &box1, const BBox &box2){
        float l = std::max(box1._lt.x, box2._lt.x);
        float r = std::min(box1._br.x, box2._br.x);

        if(r <= l){
            //No overlap or just barely
            return 0.0f;
        }

        float t = std::max(box1._lt.y, box2._lt.y);
        float b = std::min(box1._br.y, box2._br.y);

        if(b <= t){
            //No overlap or just barely
            return 0.0f;
        }



        return std::max((r-l)*(b-t) / ((box2._br.x - box2._lt.x) * (box2._br.y - box2._lt.y)), (r-l)*(b-t) / ((box1._br.x - box1._lt.x) * (box1._br.y - box1._lt.y)));
    }

    static inline void NMS(std::vector<BBox>& boxes, const float &threshold){ 
        //Sort by probability
        std::sort(boxes.begin(), boxes.end(), [](const BBox& A, const BBox& B){return A._prob > B._prob;});


        std::vector<std::vector<BBox>> boxGroups;

        for(auto&box: boxes){
            bool isOwnGroup = true;
            for(auto& boxGroup : boxGroups){
                if(CalcOverlap(boxGroup[0], box) > threshold){
                    //Overlap above threshold, means same group
                    boxGroup.push_back(box);
                    isOwnGroup = false;
                    //Continue with next box
                    break;
                }
            }
            if(isOwnGroup){
                boxGroups.push_back({box});
            }
        }

        //Clear the previous boxes
        boxes.clear();

        //For each box group, create the master box
        for(auto& boxGroup : boxGroups){
            BBox master = boxGroup[0];
            for(auto& box : boxGroup){
                if(master._lt.x > box._lt.x) master._lt.x = box._lt.x;
                if(master._lt.y > box._lt.y) master._lt.y = box._lt.y;
                if(master._br.x < box._br.x) master._br.x = box._br.x;
                if(master._br.y < box._br.y) master._br.y = box._br.y;
            }
            boxes.push_back(master);
        }
    }
}
