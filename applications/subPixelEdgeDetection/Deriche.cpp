#include "Deriche.h"

void dericheX( const cv::Mat& imageIn, cv::Mat& imageOut, double alpha,
               double omega )
{
    // Implementation based on the paper from Richard Deriche:
    // Using Canny's Criteria to derive a recursively implemented optimal edge
    // detector

    //
    // Calculate the required coefficients based on the paper
    ////
    /*const auto kDenom =
        1.0 + 2.0 * alpha * std::exp( -alpha ) - std::exp( -2.0 + alpha );
    const auto k =
        std::pow( 1.0 - std::exp( -alpha ), 2 ) * std::pow( alpha, 2 ) / kDenom;

    const auto c = std::pow( 1.0 - std::exp( -alpha ), 2 ) / std::exp( -alpha
    );*/

    const auto kDenom = 2.0 * alpha * std::exp( -alpha ) * std::sin( omega ) +
                        omega - omega * std::exp( -2.0 * alpha );

    const auto k = ( 1.0 - 2.0 * std::exp( -alpha ) * std::cos( omega ) +
                     std::exp( -2.0 * alpha ) ) *
                   ( std::pow( alpha, 2 ) + std::pow( omega, 2 ) ) / kDenom;

    const auto c = ( 1.0 - 2.0 * std::exp( -alpha ) * std::cos( omega ) +
                     std::exp( -2 * alpha ) ) /
                   ( std::exp( -alpha ) * std::sin( omega ) );

    const auto a = -c * std::exp( -alpha ) * std::sin( omega );
    const auto b1 = -2.0 * std::exp( -alpha ) * std::cos( omega );
    const auto b2 = std::exp( -2 * alpha );
    const auto c1 = k * alpha / ( std::pow( alpha, 2 ) + std::pow( omega, 2 ) );
    const auto c2 = k * omega / ( std::pow( alpha, 2 ) + std::pow( omega, 2 ) );
    const auto a0 = c2;
    const auto a1 = ( -c2 * std::cos( omega ) + c1 * std::sin( omega ) ) *
                    std::exp( -alpha );
    const auto a2 = a1 - c2 * b1;
    const auto a3 = -c2 * b2;

    imageOut = cv::Mat( imageIn.size( ), CV_32FC1, cv::Scalar::all( 0.0f ) );
    cv::Mat YP = imageOut.clone( ); // We just need one line
    cv::Mat YM = imageOut.clone( ); // We just need one line
    cv::Mat imageS = imageOut.clone( );

    const auto width = imageIn.size( ).width;
    const auto height = imageIn.size( ).height;

    // X rows -> horizontal IIR filter
    for ( int32_t y = 0; y < height; y++ )
    {
        auto srcPtr = imageIn.ptr< uint8_t >( y );
        auto sPtr = imageS.ptr< float >( y );
        auto ypPtr = YP.ptr< float >( y );
        auto ymPtr = YM.ptr< float >( y );

        // Left to right
        // Y+(x, y) = I(x - 1, y) - b1 * Y+(x - 1, y) - b2 * Y+(x - 2, y)
        int32_t x = 0;
        ypPtr[ x ] = *srcPtr;

        x++;
        srcPtr++;
        ypPtr[ x ] = srcPtr[ x - 1 ] - b1 * ypPtr[ x - 1 ];

        x++;
        srcPtr++;

        for ( ; x < width; x++, srcPtr++ )
        {
            ypPtr[ x ] =
                srcPtr[ -1 ] - b1 * ypPtr[ x - 1 ] - b2 * ypPtr[ x - 2 ];
        }

        // Right to left
        // Y-(x, y) = I(x + 1, y) - b1 * Y-(x + 1, y) - b2 * Y-(x + 2, y)
        srcPtr = imageIn.ptr< uint8_t >( y );
        srcPtr += width - 1;

        x = width - 1;
        ymPtr[ x ] = srcPtr[ x ];

        x--;
        srcPtr--;
        ymPtr[ x ] = srcPtr[ x + 1 ] - b1 * ymPtr[ x + 1 ];

        x--;
        srcPtr--;

        for ( ; x >= 0; x--, srcPtr-- )
        {
            ymPtr[ x ] =
                srcPtr[ 1 ] - b1 * ymPtr[ x + 1 ] - b2 * ymPtr[ x + 2 ];
        }

        for ( x = 0; x < width; x++ )
        {
            sPtr[ x ] = a * ( ypPtr[ x ] - ymPtr[ x ] );
        }
    }

    // X cols
    cv::Mat RP = cv::Mat( 1, height, CV_32FC1, cv::Scalar::all( 0.0f ) );
    cv::Mat RM = cv::Mat( 1, height, CV_32FC1, cv::Scalar::all( 0.0f ) );

    // const auto stride = imageS.step[ 0 ];

    auto rpPtr = RP.ptr< float >( 0 );
    auto rmPtr = RM.ptr< float >( 0 );

    for ( int32_t x = 0; x < width; x++ )
    {
        // Top to bottom
        // R+(x, y) = a0 * S(x, y) + a1 * S(x, y - 1) - b1 * R+(x, y - 1) - b2 *
        // R+(x, y - 2)

        auto sPtr = imageS.ptr< float >( 0 );

        // Move to current column
        sPtr += x;

        int32_t y = 0;
        rpPtr[ y ] = a0 * *sPtr;

        y++;
        sPtr += width;

        rpPtr[ y ] = a0 * *sPtr + a1 * sPtr[ -width ] - b1 * rpPtr[ y - 1 ];

        y++;
        sPtr += width;

        for ( ; y < height; y++, sPtr += width )
        {
            rpPtr[ y ] = a0 * *sPtr + a1 * sPtr[ -width ] -
                         b1 * rpPtr[ y - 1 ] - b2 * rpPtr[ y - 2 ];
        }

        // Bottom to top
        // R-(x, y) = a2 * S(x, y + 1) + a3 * S(x, y + 2) - b1 * R-(x, y + 1) -
        // b2 * R-(x, y + 2)
        sPtr = imageS.ptr< float >( height - 1 );
        sPtr += x;

        y = height - 1;
        rmPtr[ y ] = *sPtr;

        y--;
        sPtr -= width;
        rmPtr[ y ] = a2 * sPtr[ width ] - b1 + rmPtr[ y + 1 ];

        y--;
        sPtr -= width;

        for ( ; y >= 0; y--, sPtr -= width )
        {
            rmPtr[ y ] = a2 * sPtr[ width ] + a3 * sPtr[ 2 * width ] -
                         b1 * rmPtr[ y + 1 ] - b2 * rmPtr[ y + 2 ];
        }

        for ( y = 0; y < height; y++ )
        {
            auto dstPtr = imageOut.ptr< float >( y );
            dstPtr += x;
            *dstPtr = rmPtr[ y ] + rpPtr[ y ];
        }
    }

    // Y rows -> vertical IIR filter
}

void dericheY( const cv::Mat& imageIn, cv::Mat& imageOut, double alpha,
               double omega )
{
    // Implementation based on the paper from Richard Deriche:
    // Using Canny's Criteria to derive a recursively implemented optimal edge
    // detector

    //
    // Calculate the required coefficients based on the paper
    ////
    /*const auto kDenom =
        1.0 + 2.0 * alpha * std::exp( -alpha ) - std::exp( -2.0 + alpha );
    const auto k =
        std::pow( 1.0 - std::exp( -alpha ), 2 ) * std::pow( alpha, 2 ) / kDenom;

    const auto c = std::pow( 1.0 - std::exp( -alpha ), 2 ) / std::exp( -alpha
    );*/

    const auto kDenom = 2.0 * alpha * std::exp( -alpha ) * std::sin( omega ) +
                        omega - omega * std::exp( -2.0 * alpha );

    const auto k = ( 1.0 - 2.0 * std::exp( -alpha ) * std::cos( omega ) +
                     std::exp( -2.0 * alpha ) ) *
                   ( std::pow( alpha, 2 ) + std::pow( omega, 2 ) ) / kDenom;

    const auto c = ( 1.0 - 2.0 * std::exp( -alpha ) * std::cos( omega ) +
                     std::exp( -2 * alpha ) ) /
                   ( std::exp( -alpha ) * std::sin( omega ) );

    const auto a = -c * std::exp( -alpha ) * std::sin( omega );
    const auto b1 = -2.0 * std::exp( -alpha ) * std::cos( omega );
    const auto b2 = std::exp( -2 * alpha );
    const auto c1 = k * alpha / ( std::pow( alpha, 2 ) + std::pow( omega, 2 ) );
    const auto c2 = k * omega / ( std::pow( alpha, 2 ) + std::pow( omega, 2 ) );
    const auto a0 = c2;
    const auto a1 = ( -c2 * std::cos( omega ) + c1 * std::sin( omega ) ) *
                    std::exp( -alpha );
    const auto a2 = a1 - c2 * b1;
    const auto a3 = -c2 * b2;

    const auto width = imageIn.size( ).width;
    const auto height = imageIn.size( ).height;

    imageOut = cv::Mat( imageIn.size( ), CV_32FC1, cv::Scalar::all( 0.0f ) );
    cv::Mat YP = cv::Mat( 1, height, CV_32FC1, cv::Scalar::all( 0.0f ) );
    cv::Mat YM = cv::Mat( 1, height, CV_32FC1, cv::Scalar::all( 0.0f ) );
    cv::Mat imageS = imageOut.clone( );

    auto ypPtr = YP.ptr< float >( 0 );
    auto ymPtr = YM.ptr< float >( 0 );

    // IIR Filter

    // Y+(x, y) = I(x, y - 1) - b1 * Y+(x, y - 1) - b2 * Y+(x, y - 2)
    // for x = 0 ... M - 1; y = 0 ... N - 1

    // Y-(x, y) = I(x, y + 1) - b1 * Y-(x, y + 1) - b2 * Y-(x, y + 2)
    // for x = 0 ... M - 1; y = N - 1 ... 0

    // S(x, y) = a * (Y+(x, y) - Y-(x, y)
    // for x = 0 ... M - 1; y = 0 ... N - 1

    // Y cols -> vertical IIR filter
    for ( int32_t x = 0; x < width; x++ )
    {
        // Top to bottom
        auto srcPtr = imageIn.ptr< uint8_t >( 0 );
        auto sPtr = imageS.ptr< float >( 0 );

        srcPtr += x;
        sPtr += x;

        int32_t y = 0;
        ypPtr[ y ] = *srcPtr;

        y++;
        srcPtr += width;
        ypPtr[ y ] = srcPtr[ -width ] - b1 * ypPtr[ y - 1 ];

        y++;
        srcPtr += width;

        for ( ; y < height; y++, srcPtr += width )
        {
            ypPtr[ y ] =
                srcPtr[ -width ] - b1 * ypPtr[ y - 1 ] - b2 * ypPtr[ y - 2 ];
        }

        // Bottom to top
        srcPtr = imageIn.ptr< uint8_t >( height - 1 );
        srcPtr += x;

        y = height - 1;
        ymPtr[ y ] = *srcPtr;

        y--;
        srcPtr -= width;
        ymPtr[ y ] = srcPtr[ width ] - b1 * ymPtr[ y + 1 ];

        y--;
        srcPtr -= width;

        for ( ; y >= 0; y--, srcPtr -= width )
        {
            ymPtr[ y ] =
                srcPtr[ width ] - b1 * ymPtr[ y + 1 ] - b2 * ymPtr[ y + 2 ];
        }

        for ( y = 0; y < height; y++, sPtr += width )
        {
            *sPtr = a * ( ypPtr[ y ] - ymPtr[ y ] );
        }
    }

    // Y rows

    // R+(x, y) = a0 * S(x, y) + a1 * S(x - 1, y) - b1 * R+(x - 1, y) - b2 *
    // R+(x - 2, y)
    // for x = 0 ... M - 1; y = 0 ... N - 1

    // R-(x, y) = a2 * S(x + 1, y) + a3 * S(x + 2, y) - b1 * R-(x + 1, y) - b2 *
    // R-(x + 2, y)
    // for x = M - 1 ... 0; y = 0 ... N - 1

    // R(x, y) = R-(x, y) + R+(x, y)
    // for x = 0 ... M - 1; y = 0 ... N - 1

    cv::Mat RP = cv::Mat( 1, width, CV_32FC1, cv::Scalar::all( 0.0f ) );
    cv::Mat RM = cv::Mat( 1, width, CV_32FC1, cv::Scalar::all( 0.0f ) );

    auto rpPtr = RP.ptr< float >( 0 );
    auto rmPtr = RM.ptr< float >( 0 );

    for ( int32_t y = 0; y < height; y++ )
    {
        // Left to right
        auto dstPtr = imageOut.ptr< float >( y );
        auto sPtr = imageS.ptr< float >( y );

        int32_t x = 0;
        rpPtr[ x ] = a0 * *sPtr;

        x++;
        sPtr++;
        rpPtr[ x ] = a0 * sPtr[ 0 ] + a1 * sPtr[ -1 ] - b1 * rpPtr[ x - 1 ];

        x++;
        sPtr++;

        for ( ; x < width; x++, sPtr++ )
        {
            rpPtr[ x ] = a0 * sPtr[ 0 ] + a1 * sPtr[ -1 ] -
                         b1 * rpPtr[ x - 1 ] - b2 * rpPtr[ x - 2 ];
        }

        // Right to left
        sPtr = imageS.ptr< float >( y );

        sPtr += width - 1;

        x = width - 1;
        rmPtr[ x ] = *sPtr;

        x--;
        sPtr--;
        rmPtr[ x ] = a2 * sPtr[ 1 ] - b1 * rmPtr[ x + 1 ];

        x--;
        sPtr--;

        for ( ; x >= 0; x--, sPtr-- )
        {
            rmPtr[ x ] = a2 * sPtr[ 1 ] + a3 * sPtr[ 2 ] - b1 * rmPtr[ x + 1 ] -
                         b2 * rmPtr[ x + 2 ];
        }

        for ( x = 0; x < width; x++ )
        {
            dstPtr[ x ] = rmPtr[ x ] + rpPtr[ x ];
        }
    }
}