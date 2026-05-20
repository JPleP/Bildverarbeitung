#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <stdio.h>
#include <opencv2/opencv.hpp>


#include <opencv2/videoio.hpp> //For video input
#include <vector>


#include "NCNNPose.h"
#include "benchmark.h"
#include "NCNNDet.h"
#include "mat.h"

#include <chrono>

#include "NMS.h"
 
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





int main(int argc, char** argv )
{
    
    cv::VideoCapture videoStream(0);
    cv::Size resolution = cv::Size((int) videoStream.get(cv::CAP_PROP_FRAME_WIDTH),
                    (int) videoStream.get(cv::CAP_PROP_FRAME_HEIGHT));

    cv::Mat frame;
    //cv::Mat frame;
    const char* WIN_VS = "Video Stream";

    cv::namedWindow(WIN_VS, cv::WINDOW_KEEPRATIO);
    cv::resizeWindow(WIN_VS, 800, 600);


    cv::Size detectorSize(320,320);
    NCNNDet detector("../models/WholeBody/Det_Body.param","../models/WholeBody/Det_Body.bin", detectorSize.width,detectorSize.height);
    
    cv::Size estimatorSize(192,256);
    NCNNPose estimator("../models/WholeBody/Est_Body.param","../models/WholeBody/Est_Body.bin", estimatorSize.width,estimatorSize.height);

    auto t_prev = std::chrono::system_clock::now();
    float movAvgFPS[10] = {}; int t_idx = 0;
    
    for(;;){
        //Get the current frame
        videoStream >> frame;

    
        auto inf_prev = std::chrono::system_clock::now();
        
        //Get all the boxes of the system
        std::vector<BBox> boxes = detector.GetBBoxes(detector.GetData(frame));
        NMS::NMS(boxes, 0.5);

        std::vector<Keypoint> keypoints;
        if(boxes.size() > 0){
            cv::Rect roi;
            

            cv::Rect roi(boxes[0]._lt.x,boxes[0]._lt.y,boxes[0]._br.x,boxes[0]._br.y);
            keypoints = estimator.GetData(frame(roi));
        }

        auto inf_stop = std::chrono::system_clock::now();
        
        
        
        DrawKeypoint(frame, boxes);

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




