#pragma once

// OpenCV includes
#include <opencv2/core.hpp>

void dericheX( const cv::Mat& imageIn, cv::Mat& imageOut, double alpha,
               double omega );

void dericheY( const cv::Mat& imageIn, cv::Mat& imageOut, double alpha,
               double omega );