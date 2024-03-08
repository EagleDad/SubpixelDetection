#pragma once

// Std includes
#include <vector>

// OpenCV includes
#include <opencv2/core.hpp>

std::vector< std::vector< cv::Point2f > >
edgesSubPix( const cv::Mat& imageIn, int32_t blurSize, int32_t derivativeSize,
             double lowThreshold, double highThreshold );