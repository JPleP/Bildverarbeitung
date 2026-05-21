#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <stdio.h>
#include <opencv2/opencv.hpp>


#include <opencv2/videoio.hpp> //For video input
#include <vector>


#include "benchmark.h"
#include "NCNN.h"
#include "mat.h"

#include <chrono>

#include "utils.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include "Camera.h"

 
static int draw_fps(cv::Mat &rgb, double value)
{
    char text[32];
    sprintf(text, "FPS=%.2f", value);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
} 

static int draw_inf_time(cv::Mat &rgb, double value)
{
    char text[32];
    sprintf(text, "INF=%.2f ms", value*1000);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = label_size.height;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
} 


void DrawKeypoint(cv::Mat& image, std::vector<BBox>& relPos){
    cv::Point2f factor{(float)image.cols, (float)image.rows};
    cv::InputOutputArray arr(image);

    for(auto& e: relPos){
        cv::rectangle(arr, cv::Point2f{e._lt.x * factor.x,e._lt.y * factor.y},cv::Point2f{e._br.x * factor.x,e._br.y * factor.y}, cv::Scalar{0,100,200,255});
    }
}

void DrawKeypoint(cv::Mat& image, std::vector<Keypoint>& relPos){
    cv::Point2f factor{(float)image.cols, (float)image.rows};
    cv::InputOutputArray arr(image);

    for(int i = 0; i < Body2DPoseEst::KEY_LABELS::right_hip; ++i){
        auto& e = relPos[i];
        cv::circle(arr, cv::Point2f{e.x * factor.x,e.y * factor.y}, 10, cv::Scalar{0,0,250,255});
    }
}





int main(int argc, char** argv )
{
    
    Camera camera;

    cv::Mat frame;
    //cv::Mat frame;
    const char* WIN_VS = "Video Stream";

    cv::namedWindow(WIN_VS, cv::WINDOW_KEEPRATIO);
    cv::resizeWindow(WIN_VS, 800, 600);


    cv::Size detectorSize(320,320);
    NCNNDet detector("../../../models/WholeBody/Det_Body.param","../../../models/WholeBody/Det_Body.bin", detectorSize.width,detectorSize.height);
    
    cv::Size estimatorSize(192,256);
    NCNNPose estimator("../../../models/WholeBody/Est_Body.param","../../../models/WholeBody/Est_Body.bin", estimatorSize.width,estimatorSize.height);

    auto t_prev = std::chrono::system_clock::now();
    float movAvgFPS[10] = {}; int t_idx = 0;
    
    for(;;){
        //Get the current frame
        camera >> frame;

    
        auto inf_prev = std::chrono::system_clock::now();
        
        //Get all the boxes of the system
        std::vector<BBox> boxes = detector.GetData(detector.PrepareInput(frame));
        NMS::NMS(boxes, 0.1);

        //Ensure all boxes are constrained to the image
        for(auto& box : boxes){
            box._br.x = std::min(box._br.x, 0.999f);
            box._br.y = std::min(box._br.y, 0.999f);
            box._lt.x = std::max(box._lt.x, 0.0f);
            box._lt.y = std::max(box._lt.y, 0.0f);
        }

        std::vector<Keypoint> keypoints;
        if(boxes.size() > 0){
            //For now we assume, that it's a body detector. Therefore, only the first and most probable is the important one
            cv::Rect roi(boxes[0]._lt.x * (float)frame.cols,boxes[0]._lt.y* (float)frame.rows,(boxes[0]._br.x - boxes[0]._lt.x)* (float)frame.cols,(boxes[0]._br.y - boxes[0]._lt.y)* (float)frame.rows);

            //TODO: Maybe Expand?

            //TODO: Does not pass a frame for some reason.
            cv::Mat roi_frame = cv::Mat(frame, roi).clone();
            keypoints = estimator.GetData(estimator.PrepareInput(roi_frame));
            //Change the keypoints in normalized coordinates
            for(auto& keyp : keypoints){
                keyp.x = (roi.x + keyp.x * roi.width)/ (float)frame.cols;
                keyp.y = (roi.y +  keyp.y * roi.height)/ (float)frame.rows;
            }
        }

        auto inf_stop = std::chrono::system_clock::now();
        
        
        
        DrawKeypoint(frame, boxes);
        if(boxes.size() > 0)
            DrawKeypoint(frame, keypoints);

        auto t_curr = std::chrono::system_clock::now(); movAvgFPS[t_idx++] = std::chrono::duration<double>(t_curr-t_prev).count(); t_idx %= 10; t_prev = t_curr;
        double t_frame = 0; for(int i = 0; i < 10; ++i) t_frame += movAvgFPS[i]; t_frame /= 10.0;
        draw_fps(frame, 1.0f / t_frame);
        draw_inf_time(frame, std::chrono::duration<double>(inf_stop-inf_prev).count());
        cv::imshow(WIN_VS, frame);
        char c = (char)cv::waitKey(1);
        //cv::waitKey(0);
        if(c == 27)
            break;
    }
    
 
    return 0;
}




