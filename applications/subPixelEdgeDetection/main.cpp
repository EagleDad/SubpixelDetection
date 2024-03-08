#include "SubPixelDetection.h"

// OpenCV includes
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

// STD includes
#include <iostream>

const std::string IMAGES_ROOT = "C:/images";
const std::string RESULTS_ROOT = "C:/images/results";

// Variables for source and edge images
cv::Mat source, edges;
// Variables for low and high thresholds
int lowThreshold = 50;
int highThreshold = 100;

// Max trackbar value
int const maxThreshold = 1000;

// Sobel aperture sizes for Canny edge detector
int apertureSizes[] = { 3, 5, 7 };
int maxapertureIndex = 2;
int apertureIndex = 0;

// Gaussian blur size
int blurAmount = 0;
int maxBlurAmount = 20;

std::string windowName = "SubPixel Detector";

// Function for trackbar call
void applyCanny( int, void* )
{
    // Variable to store blurred image
    cv::Mat blurredSrc;

    // Blur the image before edge detection
    if ( blurAmount > 0 )
    {
        cv::GaussianBlur( source,
                          blurredSrc,
                          cv::Size( 2 * blurAmount + 1, 2 * blurAmount + 1 ),
                          0 );
    }
    else
    {
        blurredSrc = source.clone( );
    }

    // Canny requires aperture size to be odd
    int apertureSize = apertureSizes[ apertureIndex ];

    // Apply canny to get the edges
    cv::Canny( blurredSrc, edges, lowThreshold, highThreshold, apertureSize );

    cv::Mat matShow;
    cv::hconcat( source, edges, matShow );

    auto contours = edgesSubPix( source,
                                 blurAmount,
                                 apertureSizes[ apertureIndex ],
                                 lowThreshold,
                                 highThreshold );

    cv::hconcat( matShow, edges, matShow );

    // Display images
    cv::imshow( windowName, matShow );
}

int main( [[maybe_unused]] int argc, [[maybe_unused]] char** argv )
{
    // Read lena image
    // source = cv::imread( IMAGES_ROOT + "/sample.jpg", cv::IMREAD_GRAYSCALE );
    source = cv::imread( IMAGES_ROOT + "/Picture Vision-Ring_001.bmp",
                         cv::IMREAD_GRAYSCALE );

    // Display images
    cv::imshow( windowName, source );

    // Create a window to display output.
    cv::namedWindow( windowName, cv::WINDOW_AUTOSIZE );

    // Trackbar to control the low threshold
    cv::createTrackbar(
        "Low Threshold", windowName, &lowThreshold, maxThreshold, applyCanny );

    // Trackbar to control the high threshold
    cv::createTrackbar( "High Threshold",
                        windowName,
                        &highThreshold,
                        maxThreshold,
                        applyCanny );

    // Trackbar to control the aperture size
    cv::createTrackbar( "aperture Size",
                        windowName,
                        &apertureIndex,
                        maxapertureIndex,
                        applyCanny );

    // Trackbar to control the blur
    cv::createTrackbar(
        "Blur", windowName, &blurAmount, maxBlurAmount, applyCanny );

    cv::waitKey( 0 );

    cv::destroyAllWindows( );

    return 0;
}