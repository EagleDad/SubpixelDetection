#pragma once

// Std includes
#include <vector>

// OpenCV includes
#include <opencv2/core.hpp>

struct Contour
{
    std::vector< cv::Point2f > subPixContour;
    std::vector< float > response;
    std::vector< cv::Point2f > direction;
};

std::vector< Contour > edgesSubPix( const cv::Mat& imageIn, int32_t blurSize,
                                    int32_t derivativeSize, double lowThreshold,
                                    double highThreshold );