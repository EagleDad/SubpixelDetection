#include "SubPixelDetection.h"

// OpenCV includes
#include <opencv2/imgproc.hpp>

std::vector< std::vector< cv::Point2f > >
edgesSubPix( const cv::Mat& imageIn, int32_t blurSize, int32_t derivativeSize,
             double lowThreshold, double highThreshold )
{
    // First we need to blur the image with a gaussian
    cv::Mat imageBlurred;
    if ( blurSize > 0 )
    {
        cv::GaussianBlur( imageIn,
                          imageBlurred,
                          cv::Size( 2 * blurSize + 1, 2 * blurSize + 1 ),
                          0 );
    }
    else
    {
        imageBlurred = imageIn;
    }

    // Apply canny to get the edges
    cv::Mat imageEdges;
    cv::Canny(
        imageBlurred, imageEdges, lowThreshold, highThreshold, derivativeSize );

    return { };
}
