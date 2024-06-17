#include "Deriche.h"
#include "SubPixelDetection.h"

// OpenCV includes
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

// Variables for source and edge images
cv::Mat source, edges;
// Variables for low and high thresholds
int lowThreshold = 50;
int highThreshold = 100;

// Max trackbar value
int const maxThreshold = 5000;

// Sobel aperture sizes for Canny edge detector
int apertureSizes[] = { 3, 5, 7 };
int maxapertureIndex = 2;
int apertureIndex = 0;

// Gaussian blur size
int blurAmount = 0;
int maxBlurAmount = 20;

// Edge detector
int edgeDetector = 0;
int maxEdgeDetector = 1;
// 0 -> Sobel, 1 -> Deriche

int alphaFactor = 1;
int maxAlphaFactor = 250;

bool drawGradient { false };

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
    const int32_t apertureSize = apertureSizes[ apertureIndex ];

    cv::Mat derivativeX;
    cv::Mat derivativeY;
    const auto alpha = alphaFactor / 100.0;

    if ( edgeDetector == 0 )
    {
        cv::Sobel( blurredSrc, derivativeX, CV_16SC1, 1, 0, apertureSize );
        cv::Sobel( blurredSrc, derivativeY, CV_16SC1, 0, 1, apertureSize );
    }
    else
    {
        const auto omega = alpha / 1000;
        dericheX( blurredSrc, derivativeX, alpha, omega );
        dericheY( blurredSrc, derivativeY, alpha, omega );
        derivativeX.convertTo( derivativeX, CV_16SC1 );
        derivativeY.convertTo( derivativeY, CV_16SC1 );
    }

    // Apply canny to get the edges
    // cv::Canny( blurredSrc, edges, lowThreshold, highThreshold, apertureSize
    // );

    cv::Canny( derivativeX, derivativeY, edges, lowThreshold, highThreshold );

    auto contours = edgesSubPix( source,
                                 blurAmount,
                                 alpha,
                                 edgeDetector,
                                 apertureSizes[ apertureIndex ],
                                 lowThreshold,
                                 highThreshold );

    //
    // To be able to draw contours in color, the images needs to be converted
    //
    cv::Mat srcColor;
    cv::cvtColor( source, srcColor, cv::COLOR_GRAY2BGR );

    cv::Mat edgesColor;
    cv::cvtColor( edges, edgesColor, cv::COLOR_GRAY2BGR );

    cv::Mat resultColor = srcColor.clone( );

    // Draw the contours
    cv::RNG rng( 12345 );
    for ( const auto& contour : contours )
    {
        // Only integral points can be drawn
        std::vector< cv::Point > polyLine;
        polyLine.reserve( contour.subPixContour.size( ) );

        float responseMax = std::numeric_limits< float >::min( );

        for ( size_t i = 0; i < contour.subPixContour.size( ); i++ )
        {
            const auto& pt = contour.subPixContour[ i ];

            responseMax = std::max( responseMax, contour.response[ i ] );

            polyLine.emplace_back(
                static_cast< int32_t >( std::round( pt.x ) ),
                static_cast< int32_t >( std::round( pt.y ) ) );
        }

        cv::Scalar color = cv::Scalar( rng.uniform( 0, 255 ),
                                       rng.uniform( 0, 255 ),
                                       rng.uniform( 0, 255 ) );

        cv::polylines( resultColor, polyLine, false, color, 1 );

        if ( drawGradient )
        {
            // Draw the direction of the contour point
            // The line length of the max response should be 10
            for ( size_t i = 0; i < contour.subPixContour.size( ); i++ )
            {
                const auto& pt = contour.subPixContour[ i ];
                const auto dir = contour.direction[ i ];
                /*const auto length = static_cast< int32_t >(
                    std::round( contour.response[ i ] / responseMax * 10.0f )
                   );*/
                const auto length = 10.0f;

                const auto p2 = pt + dir * length;

                const auto pl1 =
                    cv::Point( static_cast< int32_t >( std::round( pt.x ) ),
                               static_cast< int32_t >( std::round( pt.y ) ) );
                const auto pl2 =
                    cv::Point( static_cast< int32_t >( std::round( p2.x ) ),
                               static_cast< int32_t >( std::round( p2.y ) ) );

                cv::line( resultColor, pl1, pl2, color );
            }
        }
    }

    cv::Mat matShow;
    cv::hconcat( srcColor, edgesColor, matShow );
    cv::hconcat( matShow, resultColor, matShow );

    // Display images
    cv::imshow( windowName, matShow );
}

int main( [[maybe_unused]] int argc, [[maybe_unused]] char** argv )
{
    //
    // Read the test image
    //
    source = cv::imread( "TestImage.bmp", cv::IMREAD_GRAYSCALE );

    //
    // If the test image is not available, create a dummy one
    //
    if ( source.cols == 0 )
    {
        source = cv::Mat( 512, 512, CV_8UC1, cv::Scalar::all( 0 ) );

        // Draw a circle to the center
        // cv::circle( source, { 256, 256 }, 100, cv::Scalar::all( 128 ), -1 );

        cv::ellipse( source,
                     { 256, 256 },
                     { 200, 100 },
                     0,
                     0,
                     360,
                     cv::Scalar::all( 128 ),
                     -1 );

        // const auto rect = cv::Rect( 64, 64, 256, 256 );
        // cv::rectangle( source, rect, cv::Scalar::all( 128 ), -1 );

        cv::GaussianBlur( source, source, cv::Size( 15, 15 ), 0 );
    }

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

    // Trackbar to control the edge detector
    cv::createTrackbar( "Edge Detector",
                        windowName,
                        &edgeDetector,
                        maxEdgeDetector,
                        applyCanny );

    // Trackbar to control the alpha factor
    cv::createTrackbar(
        "Alpha", windowName, &alphaFactor, maxAlphaFactor, applyCanny );

    // Trackbar to control the blur
    cv::createTrackbar(
        "Blur", windowName, &blurAmount, maxBlurAmount, applyCanny );

    int key { };

    while ( key != 27 )
    {
        if ( key == 'g' )
        {
            drawGradient = ! drawGradient;
            applyCanny( 0, nullptr );
        }

        key = cv::waitKey( 20 ) & 0xFF;
    }

    cv::destroyAllWindows( );

    return 0;
}