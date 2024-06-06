#include "SubPixelDetection.h"
#include "Graph.h"

// Std includes
#include <iostream>

// OpenCV includes
#include <opencv2/imgproc.hpp>

enum class SearchDirection
{
    Right = 1,
    Bottom = 2,
    Left = 3,
    Top = 4
};

enum class Neighbourhood
{
    FourConnected,
    EightConnected
};

//
// isEqual - Any arithmetic type
//
template < typename T >
std::enable_if_t< std::is_arithmetic_v< T >, bool >
isEqual( T a, T b, T epsilon = std::numeric_limits< T >::epsilon( ) )
{
    if constexpr ( ! std::is_floating_point_v< T > )
    {
        return a == b;
    }
    else
    {
        return std::fabs( a - b ) < epsilon;
    }
}

//
// isZero
//
template < typename T >
std::enable_if_t< std::is_arithmetic_v< T >, bool >
isZero( T val, T epsilon = std::numeric_limits< T >::epsilon( ) )
{
    if constexpr ( ! std::is_floating_point_v< T > )
    {
        return val == T( );
    }
    else
    {
        return std::fabs( val ) <= epsilon;
    }
}

std::vector< std::vector< cv::Point2i > >
labelContours( const cv::Mat& imageIn );

void checkNeighbourhood( int32_t x, int32_t y, const cv::Mat& refTable,
                         std::vector< int32_t >& neighbourhood );

int32_t smallestNeighbour( const std::vector< int32_t >& neighbourhood,
                           const std::vector< int32_t >& activeLabels );

void createElement(
    int32_t x, int32_t y,
    std::vector< std::shared_ptr< std::vector< cv::Point2i > > >& objects,
    std::vector< int32_t >& activeLabels, int32_t labelNumber );

void addElement(
    int32_t x, int32_t y, int32_t labelNumber,
    const std::vector< std::shared_ptr< std::vector< cv::Point2i > > >&
        objects );

void mergeElement(
    const std::vector< int32_t >& neighbourhood, int32_t newLabelNumber,
    std::vector< std::shared_ptr< std::vector< cv::Point2i > > >& objects,
    std::vector< int32_t >& activeLabels );

std::vector< std::vector< cv::Point2i > > calculateShortestPathsDijkstra(
    const std::vector< cv::Point2i >& unorderedContourPoints,
    const cv::Mat& imageCanny );

int32_t countNeighbours( const cv::Point2i& pos, const cv::Mat& imageIn,
                         const SearchDirection direction );

int32_t countNeighbours( const cv::Point2i& pos, const cv::Mat& imageIn,
                         const Neighbourhood neighbourhood );

uint8_t getPixelValue( const cv::Mat& mat, const cv::Point2i& point );

std::vector< size_t >
findPossibleStartPoints( const std::vector< cv::Point2i >& contourPoints,
                         const cv::Mat& imageIn );

std::vector< cv::Point2i >
traceContourPavlidis( const cv::Mat& imageIn, const cv::Point2i& startPoint );

std::unique_ptr< Graph >
calculateAdjacencyMatrix( const std::vector< cv::Point2i >& contourPoints );

cv::Mat thinning( const cv::Mat& imageIn );

int32_t thinningIteration( cv::Mat& imageA, cv::Mat& imageB,
                           const int32_t iteration );

void magnitudeNeighbourhood( const cv::Mat& derivationX,
                             const cv::Mat& derivationY,
                             const cv::Point2i& position,
                             const Neighbourhood& neighbourhood,
                             std::vector< float >& magnitudes );

float amplitude( const cv::Mat& derivationX, const cv::Mat& derivationY,
                 const cv::Point2i& position );

void secondFacetModel( const std::vector< float >& magnitudes,
                       std::vector< float >& secondFacetModel );

void extractSubPixelPosition( const cv::Mat& image, const cv::Point& pos,
                              const cv::Mat& derivativeX,
                              const cv::Mat& derivativeY,
                              cv::Point2f& subPixelPoint, float& response,
                              cv::Point2f& direction );

void extractSubPixelPositionSecondFacet(
    const cv::Mat& image, const cv::Point& pos, const cv::Mat& derivativeX,
    const cv::Mat& derivativeY, cv::Point2f& subPixelPoint, float& response,
    cv::Point2f& direction );

void extractSubPixelPositionInterpolation(
    const cv::Mat& image, const cv::Point& pos, const cv::Mat& derivativeX,
    const cv::Mat& derivativeY, cv::Point2f& subPixelPoint, float& response,
    cv::Point2f& direction );

void calculateEigenValuesVectors( const cv::Mat& src, float& eigenValue1,
                                  float& eigenValue2, cv::Point2f& eigenVector1,
                                  cv::Point2f& eigenVector2 );

void calculateEigenValuesVectorsCv( const cv::Mat& src, float& eigenValue1,
                                    float& eigenValue2,
                                    cv::Point2f& eigenVector1,
                                    cv::Point2f& eigenVector2 );

void calculateEigenValuesVectorsSelf( const cv::Mat& src, float& eigenValue1,
                                      float& eigenValue2,
                                      cv::Point2f& eigenVector1,
                                      cv::Point2f& eigenVector2 );

void imageNeighbourhood( const cv::Mat& image, const cv::Point2i& position,
                         const Neighbourhood& neighbourhood,
                         std::vector< float >& magnitudes );

float pixelValue( const cv::Mat& image, const cv::Point2i& position );

///
///
///

std::vector< Contour > edgesSubPix( const cv::Mat& imageIn, int32_t blurSize,
                                    int32_t derivativeSize, double lowThreshold,
                                    double highThreshold )
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

    // Since we want to calculate subpixel edges, the derivatives are required.
    // We calculated them here and pass them to the canny operator that they are
    // not calculated twice.
    cv::Mat imageSobelX;
    cv::Mat imageSobelY;

    cv::Sobel( imageBlurred, imageSobelX, CV_16S, 1, 0, derivativeSize );
    cv::Sobel( imageBlurred, imageSobelY, CV_16S, 0, 1, derivativeSize );

    // Calculate canny edges bases on the derivatives
    // Apply canny to get the edges
    cv::Mat imageCanny;
    cv::Canny(
        imageSobelX, imageSobelY, imageCanny, lowThreshold, highThreshold );

    // Note: The Canny image is not everywhere 1 pixel, we might run a thinning
    // on the edge image.
    imageCanny = thinning( imageCanny );

    // To b able to get sub pixel contours, connected components needs to be
    // labeled. Why not using cv::findContours? The contours returned by
    // cv::findContours are always closed. Means a 1 Pixel line is represented
    // as a rectangular, having the points twice in the contour.
    const auto contoursPix = labelContours( imageCanny );

    // Now that we've found the pixel precise contour points, we can calculate
    // the subpixel position for each point.
    std::vector< Contour > subPixelContours;

    for ( auto& currentContour : contoursPix )
    {
        // Note: the pixel precise contours are not ordered from start to end
        // right now. This is something that a caller would expect. A contour
        // should not consist out of unordered scattered points.
        const auto sortedContours =
            calculateShortestPathsDijkstra( currentContour, imageCanny );

        // Since the contour could have multiple start points due to junctions,
        // we might get multiple results for one contour. Calculate the subpixel
        // position for each contour now.
        for ( const auto& sortedContour : sortedContours )
        {
            Contour current;
            current.subPixContour.reserve( sortedContour.size( ) );
            current.response.reserve( sortedContour.size( ) );
            current.direction.reserve( sortedContour.size( ) );

            float resp;
            cv::Point2f dir;
            cv::Point2f subPixelPoint;

            for ( const auto& point : sortedContour )
            {
                extractSubPixelPosition( imageBlurred,
                                         point,
                                         imageSobelX,
                                         imageSobelY,
                                         subPixelPoint,
                                         resp,
                                         dir );

                current.subPixContour.emplace_back( subPixelPoint );

                current.response.emplace_back( resp );

                current.direction.emplace_back( dir );
            }

            subPixelContours.push_back( current );
        }
    }

    return subPixelContours;
}

/*
 * Function finds connected contours in a canny image using connected component
 *  analysis in 8 connected neighbourhood.
 *
 *
 * @param [in]  imageIn The input canny image
 *
 * @return Returns vector of contour points.
 */
std::vector< std::vector< cv::Point2i > >
labelContours( const cv::Mat& imageIn )
{
    std::vector< std::shared_ptr< std::vector< cv::Point2i > > > objects;
    std::vector< int32_t > activeLabels;

    int32_t labelNumber = 0;
    std::vector< int32_t > neighbourhood( 4 );

    cv::Mat imgLabels(
        imageIn.rows, imageIn.cols, CV_32SC1, cv::Scalar::all( 0 ) );

    for ( auto y = 0; y < imageIn.rows; y++ )
    {
        const auto rowPtrSrc = imageIn.ptr< uint8_t >( y );
        const auto rowPtrLbl = imgLabels.ptr< int32_t >( y );

        for ( auto x = 0; x < imageIn.cols; x++ )
        {
            if ( rowPtrSrc[ x ] != 0 )
            {
                int32_t currentLabelNumber;
                checkNeighbourhood( x, y, imgLabels, neighbourhood );

                const auto minLabel =
                    smallestNeighbour( neighbourhood, activeLabels );

                if ( minLabel == 0 )
                {
                    currentLabelNumber = ++labelNumber;
                    rowPtrLbl[ x ] = currentLabelNumber;
                    createElement(
                        x, y, objects, activeLabels, currentLabelNumber );
                }
                else
                {
                    currentLabelNumber = minLabel;
                    rowPtrLbl[ x ] = currentLabelNumber;
                    addElement( x, y, minLabel, objects );

                    mergeElement(
                        neighbourhood, minLabel, objects, activeLabels );
                }
            }
        }
    }

    std::vector< std::vector< cv::Point2i > > contours;

    for ( size_t i = 0; i < activeLabels.size( ); i++ )
    {
        if ( activeLabels[ i ] != static_cast< int32_t >( i ) + 1 )
            continue;

        contours.push_back( *objects[ i ] );
    }

    return contours;
}

/*
 * Function that check the 8 connected neighbourhood for labeling
 *
 * |x|x|x|
 * |x|c|0|
 * |0|0|0|
 *
 * @param [in]       x              The current x position
 * @param [in]       y              The current y position
 * @param [in]       referenceTable The reference table containing already
 *                                  applied label numbers.
 * @param [in out]   neighbourhood  The vector receiving the neighbours.
 */
void checkNeighbourhood( int32_t x, int32_t y, const cv::Mat& referenceTable,
                         std::vector< int32_t >& neighbourhood )
{
    const auto top = y - 1 >= 0 ? y - 1 : y;
    const auto left = x - 1 >= 0 ? x - 1 : x;
    const auto right = x + 1 < referenceTable.cols ? x + 1 : x;

    const auto yPtr = referenceTable.ptr< int32_t >( y );
    const auto topPtr = referenceTable.ptr< int32_t >( top );

    neighbourhood[ 0 ] = yPtr[ left ];
    neighbourhood[ 1 ] = topPtr[ left ];
    neighbourhood[ 2 ] = topPtr[ x ];
    neighbourhood[ 3 ] = topPtr[ right ];
}

/*
 * Function that returns the smallest element that is nonzero
 *
 * |x|x|x|
 * |x|c|0|
 * |0|0|0|
 *
 * @param [in]   neighbourhood   The vector receiving the neighbours.
 *
 * @return Returns the smallest nonzero element if exists, else zero.
 */
int32_t smallestNeighbour( const std::vector< int32_t >& neighbourhood,
                           const std::vector< int32_t >& activeLabels )
{
    int32_t smallest = std::numeric_limits< int32_t >::max( );

    for ( const auto& elem : neighbourhood )
    {
        if ( elem > 0 )
        {
            const auto label =
                activeLabels[ static_cast< size_t >( elem - 1 ) ];

            if ( label < smallest )
            {
                smallest = label;
            }
        }
    }

    return smallest == std::numeric_limits< int32_t >::max( ) ? 0 : smallest;
}

/*
 * Function that creates a new object list and updates the reference table
 *
 *
 * @param [in]       x              The x position of the current pixel
 * @param [in]       y              The y position of the current pixel
 * @param [in]       objects        The table of already created objects
 * @param [in out]   referenceTable The reference table.
 */
void createElement(
    int32_t x, int32_t y,
    std::vector< std::shared_ptr< std::vector< cv::Point2i > > >& objects,
    std::vector< int32_t >& activeLabels, int32_t labelNumber )
{
    objects.push_back( std::make_shared< std::vector< cv::Point2i > >( ) );

    objects.back( )->emplace_back( x, y );

    activeLabels.push_back( labelNumber );
}

/*
 * Function that adds a point to an existing object. NOTE that 0 is not a valid
 * label number. 0 means background.
 * The object is added to object at index labelNumber - 1
 *

 * @param [in]       x              The x position of the current pixel
 * @param [in]       y              The y position of the current pixel
 * @param [in]       objects        The table of already created objects
 *
 */
void addElement(
    int32_t x, int32_t y, int32_t labelNumber,
    const std::vector< std::shared_ptr< std::vector< cv::Point2i > > >&
        objects )
{
    objects[ static_cast< size_t >( labelNumber - 1 ) ]->emplace_back( x, y );
}

/*
 * Function that merges elements together, if elements in the neighbourhood
 * have a different label number.
 *
 * @param [in]    neighbourhood  The vector receiving the neighbours.
 * @param [in]    newLabelNumber The label number to merge to.
 * @param [in]    objects        The table of already created objects.
 * @param [in]    referenceTable The reference table.
 *
 */
void mergeElement(
    const std::vector< int32_t >& neighbourhood, int32_t newLabelNumber,
    std::vector< std::shared_ptr< std::vector< cv::Point2i > > >& objects,
    std::vector< int32_t >& activeLabels )
{
    for ( const auto& elem : neighbourhood )
    {
        if ( elem > 0 )
        {
            const auto old_l = static_cast< size_t >( elem - 1 );
            const auto new_l = static_cast< size_t >( newLabelNumber - 1 );
            // Get the real label number of the current object, it could already
            // be merged
            const auto realElemNum = activeLabels[ old_l ];

            if ( realElemNum != newLabelNumber )
            {
                if ( objects[ old_l ] == objects[ new_l ] )
                    continue;

                objects[ new_l ]->insert( objects[ new_l ]->end( ),
                                          objects[ old_l ]->begin( ),
                                          objects[ old_l ]->end( ) );

                // All active labels needs to be changed to the merged one
                for ( size_t i = 0; i < activeLabels.size( ); i++ )
                {
                    if ( activeLabels[ i ] == realElemNum )
                    {
                        activeLabels[ i ] = newLabelNumber;
                        objects[ i ] = objects[ new_l ];
                    }
                }
            }
        }
    }
}

/*
 * Function that calculates the shortest path through a set of 2D points using
 * Dijkstra search.
 *
 * @param [in]    neighbourhood  The vector receiving the neighbours.
 * @param [in]    newLabelNumber The label number to merge to.
 * @param [in]    objects        The table of already created objects.
 * @param [in]    referenceTable The reference table.
 *
 * @returns The ordered contour points
 */
std::vector< std::vector< cv::Point2i > > calculateShortestPathsDijkstra(
    const std::vector< cv::Point2i >& unorderedContourPoints,
    const cv::Mat& imageCanny )
{
    const auto startIndices =
        findPossibleStartPoints( unorderedContourPoints, imageCanny );

    // Set distance at start position to 0
    if ( ! startIndices.empty( ) )
    {
        // startIndex = startIndices[ 0 ];

        const auto adjacencyGraph =
            calculateAdjacencyMatrix( unorderedContourPoints );

        std::vector< std::vector< cv::Point2i > > contours(
            startIndices.size( ) - 1 );

        for ( size_t i = 0; i < startIndices.size( ) - 1; i++ )
        {
            const auto startIndex = startIndices[ i ];
            const auto destIndex = startIndices[ i + 1 ];

            adjacencyGraph->shortestPath(
                static_cast< int32_t >( startIndex ) );

            // Get all results from start point to all possible end points
            for ( const auto& orderedIdx :
                  adjacencyGraph->getShortestPath( destIndex ) )
            {
                contours.at( i ).push_back(
                    unorderedContourPoints.at( orderedIdx ) );
            }
        }

        return contours;
    }

    // The object is closed. Start point is first point.
    // Run contour tracing algorithm to sort contour points.
    return { traceContourPavlidis( imageCanny, unorderedContourPoints[ 0 ] ) };
}

/*
 * Function searches possible start points in a set of connected contour points.
 *
 * @param [in]    neighbourhood  The vector receiving the neighbours.
 * @param [in]    newLabelNumber The label number to merge to.
 * @param [in]    objects        The table of already created objects.
 * @param [in]    referenceTable The reference table.
 *
 * @returns The indices of the start points
 *
 */
std::vector< size_t >
findPossibleStartPoints( const std::vector< cv::Point2i >& contourPoints,
                         const cv::Mat& imageIn )
{
    // Check for multiple start points
    // 0 start points -> Contour is closed. Choose one
    // 2 start points, contour is a profile with given start and end
    // more than 2 start points, contour is scattered. try to find all
    // endpoints and return multiple profiles

    std::vector< size_t > startIndices;

    for ( size_t i = 0; i < contourPoints.size( ); i++ )
    {
        const auto& point = contourPoints[ i ];

        const auto neighboursTop =
            countNeighbours( point, imageIn, SearchDirection::Top );
        const auto neighboursRight =
            countNeighbours( point, imageIn, SearchDirection::Right );
        const auto neighboursBottom =
            countNeighbours( point, imageIn, SearchDirection::Bottom );
        const auto neighboursLeft =
            countNeighbours( point, imageIn, SearchDirection::Left );

        // If a point does not have neighbours in two connected directions, it
        // is considered to be a endpoint

        if ( ( neighboursTop == 0 && neighboursRight == 0 ) ||
             ( neighboursRight == 0 && neighboursBottom == 0 ) ||
             ( neighboursBottom == 0 && neighboursLeft == 0 ) ||
             ( neighboursLeft == 0 && neighboursTop == 0 ) )
        {
            // It could be only a start point, if we have max 1 neighbour is the
            // 4 connected neighborhood
            if ( countNeighbours(
                     point, imageIn, Neighbourhood::FourConnected ) <= 1 )
            {
                startIndices.push_back( i );
            }
        }

        //     BOTTOM             TOP             LEFT             RIGHT
        // |0|0|x| |0|0|x| |0|x|x|  |x|x|0| |x|0|0|  |0|0|0| |0|0|x|
        // |0|x|0| |0|x|0| |0|x|0|  |0|x|0| |0|x|0|  |x|x|x| |x|x|x| |x|x|x|
        // |x|x|0| |0|x|x| |x|x|0|  |0|x|0| |0|x|0|  |0|0|0| |x|0|0| |0|0|0|
    }

    return startIndices;
}

/*
 * Function counts the neighbours of a current pixel
 *
 * @param [in]    pos       The current position to count for.
 * @param [in]    imageIn   The input image.
 * @param [in]    direction The direction to search for neighbours.
 *
 * @returns The number of neighbours
 *
 */
int32_t countNeighbours( const cv::Point2i& pos, const cv::Mat& imageIn,
                         const SearchDirection direction )
{
    // Check 8 connectivity clockwise
    // |1|2|3|
    // |0|c|4|
    // |7|6|5|

    const auto x = pos.x;
    const auto y = pos.y;

    int32_t neighbours = 0;

    // TODO: Get and pass rowPtr

    switch ( direction )
    {
    case SearchDirection::Top:
        neighbours +=
            getPixelValue( imageIn, { x - 1, y - 1 } ) != 0 ? 1 : 0;       // 1
        neighbours += getPixelValue( imageIn, { x, y - 1 } ) != 0 ? 1 : 0; // 2
        neighbours +=
            getPixelValue( imageIn, { x + 1, y - 1 } ) != 0 ? 1 : 0; // 3
        break;
    case SearchDirection::Right:
        neighbours +=
            getPixelValue( imageIn, { x + 1, y - 1 } ) != 0 ? 1 : 0;       // 3
        neighbours += getPixelValue( imageIn, { x + 1, y } ) != 0 ? 1 : 0; // 4
        neighbours +=
            getPixelValue( imageIn, { x + 1, y + 1 } ) != 0 ? 1 : 0; // 5
        break;
    case SearchDirection::Bottom:
        neighbours +=
            getPixelValue( imageIn, { x + 1, y + 1 } ) != 0 ? 1 : 0;       // 5
        neighbours += getPixelValue( imageIn, { x, y + 1 } ) != 0 ? 1 : 0; // 6
        neighbours +=
            getPixelValue( imageIn, { x - 1, y + 1 } ) != 0 ? 1 : 0; // 7
        break;
    case SearchDirection::Left:
        neighbours +=
            getPixelValue( imageIn, { x - 1, y - 1 } ) != 0 ? 1 : 0;       // 1
        neighbours += getPixelValue( imageIn, { x - 1, y } ) != 0 ? 1 : 0; // 0
        neighbours +=
            getPixelValue( imageIn, { x - 1, y + 1 } ) != 0 ? 1 : 0; // 7
        break;
    }

    return neighbours;
}

/*
 * Function counts the neighbours of a current pixel using neighbourhood
 * specification.
 *
 * @param [in]    pos           The current position to count for.
 * @param [in]    imageIn       The input image.
 * @param [in]    neighbourhood The neighbourhood to use.
 *
 * @returns The number of neighbours
 *
 */
int32_t countNeighbours( const cv::Point2i& pos, const cv::Mat& imageIn,
                         const Neighbourhood neighbourhood )
{
    // Check 8 connectivity clockwise
    // |1|2|3|
    // |0|c|4|
    // |7|6|5|

    // Check 4 connectivity clockwise
    // |x|2|x|
    // |0|c|4|
    // |x|6|x|

    const auto x = pos.x;
    const auto y = pos.y;

    int32_t neighbours = 0;

    if ( neighbourhood == Neighbourhood::FourConnected )
    {
        neighbours += getPixelValue( imageIn, { x - 1, y } ) != 0 ? 1 : 0; // 0
        neighbours += getPixelValue( imageIn, { x, y - 1 } ) != 0 ? 1 : 0; // 2
        neighbours += getPixelValue( imageIn, { x + 1, y } ) != 0 ? 1 : 0; // 4
        neighbours += getPixelValue( imageIn, { x, y + 1 } ) != 0 ? 1 : 0; // 6
    }
    else
    {
        neighbours += getPixelValue( imageIn, { x - 1, y } ) != 0 ? 1 : 0; // 0
        neighbours +=
            getPixelValue( imageIn, { x - 1, y - 1 } ) != 0 ? 1 : 0;       // 1
        neighbours += getPixelValue( imageIn, { x, y - 1 } ) != 0 ? 1 : 0; // 2
        neighbours +=
            getPixelValue( imageIn, { x + 1, y - 1 } ) != 0 ? 1 : 0;       // 3
        neighbours += getPixelValue( imageIn, { x + 1, y } ) != 0 ? 1 : 0; // 4
        neighbours +=
            getPixelValue( imageIn, { x + 1, y + 1 } ) != 0 ? 1 : 0;       // 5
        neighbours += getPixelValue( imageIn, { x, y + 1 } ) != 0 ? 1 : 0; // 6
        neighbours +=
            getPixelValue( imageIn, { x - 1, y + 1 } ) != 0 ? 1 : 0; // 7
    }

    return neighbours;
}

/*
 * Function returns the current pixel value, while checking for a valid access.
 *
 * @param [in]    point     The position to get the value for.
 * @param [in]    mat       The input image.
 *
 * @returns The gray value. If not in range returns 0.
 *
 */
uint8_t getPixelValue( const cv::Mat& mat, const cv::Point2i& point )
{
    return ( point.x >= 0 && point.x < mat.cols && point.y >= 0 &&
             point.y < mat.rows )
               //? mat.at< uint8_t >( point )
               ? mat.ptr< uint8_t >( point.y )[ point.x ]
               : static_cast< uint8_t >( 0 );
};

/*
 * Function traces a contour using Pavlidis algorithm.
 *
 * @param [in]    imageIn       The input image containing contours.
 * @param [in]    startPoint    The start point of the contour.
 *
 * @returns The contour of the object.
 *
 */
std::vector< cv::Point2i > traceContourPavlidis( const cv::Mat& imageIn,
                                                 const cv::Point2i& startPoint )
{
    SearchDirection direction = SearchDirection::Top;
    int32_t rightTurnStep = 0;
    int32_t traceTime = 4;

    cv::Point2i currentPoint = { startPoint.x, startPoint.y };
    const cv::Point2i searchStartPoint = { startPoint.x, startPoint.y };

    std::vector< cv::Point2i > contour;

    auto turnLeft = [ & ]( const SearchDirection dir )
    {
        int32_t newDir = static_cast< int32_t >( dir ) - 1;

        if ( newDir == 0 )
        {
            newDir = 4;
        }

        return static_cast< SearchDirection >( newDir );
    };

    auto turnRight = [ & ]( const SearchDirection dir )
    {
        int32_t newDir = static_cast< int32_t >( dir ) + 1;

        if ( newDir == 5 )
        {
            newDir = 1;
        }

        return static_cast< SearchDirection >( newDir );
    };

    do
    {
        cv::Point2i P1, P2, P3;

        if ( searchStartPoint == currentPoint )
        {
            traceTime--;
            if ( traceTime == 0 )
            {
                break;
            }
        }

        switch ( direction )
        {
        case SearchDirection::Top:
            // * * *
            //   o
            P1.x = currentPoint.x - 1;
            P1.y = currentPoint.y - 1;
            P2.x = currentPoint.x;
            P2.y = currentPoint.y - 1;
            P3.x = currentPoint.x + 1;
            P3.y = currentPoint.y - 1;
            break;

        case SearchDirection::Left:
            // *
            // * o
            // *
            P1.x = currentPoint.x - 1;
            P1.y = currentPoint.y + 1;
            P2.x = currentPoint.x - 1;
            P2.y = currentPoint.y;
            P3.x = currentPoint.x - 1;
            P3.y = currentPoint.y - 1;
            break;

        case SearchDirection::Right:
            //   *
            // o *
            //   *
            P1.x = currentPoint.x + 1;
            P1.y = currentPoint.y - 1;
            P2.x = currentPoint.x + 1;
            P2.y = currentPoint.y;
            P3.x = currentPoint.x + 1;
            P3.y = currentPoint.y + 1;
            break;

        case SearchDirection::Bottom:
            //   o
            // * * *
            P1.x = currentPoint.x + 1;
            P1.y = currentPoint.y + 1;
            P2.x = currentPoint.x;
            P2.y = currentPoint.y + 1;
            P3.x = currentPoint.x - 1;
            P3.y = currentPoint.y + 1;
            break;
        }

        const uint8_t p1 = getPixelValue( imageIn, P1 );
        const uint8_t p2 = getPixelValue( imageIn, P2 );
        const uint8_t p3 = getPixelValue( imageIn, P3 );

        if ( p1 )
        {
            contour.emplace_back( currentPoint.x, currentPoint.y );
            currentPoint = P1;
            direction = turnLeft( direction );
            rightTurnStep = 0;
        }
        else if ( p2 )
        {
            contour.emplace_back( currentPoint.x, currentPoint.y );
            currentPoint = P2;
            rightTurnStep = 0;
        }
        else if ( p3 )
        {
            contour.emplace_back( currentPoint.x, currentPoint.y );
            currentPoint = P3;
            rightTurnStep = 0;
        }
        else
        {
            direction = turnRight( direction );
            rightTurnStep++;
            if ( rightTurnStep == 3 )
            {
                break;
            }
        }

    } while ( true );

    return contour;
}

/*
 * Function that calculates an adjacency matrix for a contour.
 *
 * @param [in]    contourPoints The contour point to calculate the matrix for.
 *
 * @returns A graph object.
 *
 */
std::unique_ptr< Graph >
calculateAdjacencyMatrix( const std::vector< cv::Point2i >& contourPoints )
{
    const auto numberVertices = contourPoints.size( );

    auto graph =
        std::make_unique< Graph >( static_cast< int32_t >( numberVertices ) );

    for ( size_t u = 0; u < numberVertices; u++ )
    {
        const auto& lhs = contourPoints[ u ];
        const auto& lhsX = lhs.x;
        const auto& lhsY = lhs.y;

        for ( size_t v = u; v < numberVertices; v++ )
        {
            if ( u == v )
                continue;

            const auto& rhs = contourPoints[ v ];
            const auto dx = std::abs( lhsX - rhs.x );
            const auto dy = std::abs( lhsY - rhs.y );

            if ( ( dx <= 1 && dy <= 1 ) )
            {
                graph->addEdge( static_cast< int32_t >( u ),
                                static_cast< int32_t >( v ),
                                dx + dy );
            }
        }
    }

    return graph;
}

/**
 * @brief This function performs a thinning on a region
 *
 * @param [in]   imageIn            The input single channel 8 bit image
 *
 *  @return The thinned output image
 *
 * The thinning algorithm is base on the paper from T.Y. Zhang and C.Y. Suen
 * form 1984 It describes a parallel approach to thin binary structures im
 * two sub-iterations The algorithm is considering the 8 neighbors tto the
 * current pixel location.
 *
 *         P9              P2              P3
 *   (y - 1, x - 1)    (y - 1, x)    (y - 1, x + 1)
 *
 *         P8              P1              P4
 *     (y, x - 1)        (y, x)        (y, x + 1)
 *
 *         P7              P6              P5
 *    (y + 1, x - 1)   (y + 1, x)    (y + 1, x + 1)
 *
 *    Consider: The algorithm is expecting a binary image [0 : 1]
 *
 *    B(P1) is the sum of nonzero neighbors of P1.
 *    B(P1) = P2 + P3 + P4 + P5 + P6 + P7 + P8 + P9)
 *
 *    A(P1) is the number of transitions from 0 to 1 in the following order
 *    P2 P3 P4 P5 P6 P7 P8 P9 P2
 *
 *    Steps Iteration 1 -> Conditions to satisfy
 *    2 <= B(P1) <= 6
 *    A(P1) == 1
 *    P2 * P4 * P6 == 0
 *    P4 * P6 * P8 == 0
 *
 *    Steps Iteration 2 -> Conditions to satisfy
 *    2 <= B(P1) <= 6
 *    A(P1) == 1
 *    P2 * P4 * P8 == 0
 *    P2 * P6 * P8 == 0
 *
 */
cv::Mat thinning( const cv::Mat& imageIn )
{
    // let border be the same in all directions
    constexpr int32_t border = 1;
    // constructs a larger image to fit both the image and the border
    cv::Mat workImageA( imageIn.rows + border * 2,
                        imageIn.cols + border * 2,
                        imageIn.depth( ) );

    cv::copyMakeBorder( imageIn,
                        workImageA,
                        border,
                        border,
                        border,
                        border,
                        cv::BORDER_CONSTANT,
                        cv::Scalar::all( 0 ) );

    const auto outRect = cv::Rect( 1, 1, imageIn.cols, imageIn.rows );

    int32_t changedPixels;

    cv::Mat workImageB = workImageA.clone( );

    do
    {
        changedPixels = thinningIteration( workImageA, workImageB, 0 );

        workImageB.copyTo( workImageA );

        changedPixels +=
            thinningIteration( // NOLINT(readability-suspicious-call-argument)
                workImageB,
                workImageA,
                1 );

        workImageA.copyTo( workImageB );

    } while ( changedPixels > 0 );

    return { workImageA( outRect ) };
}

int32_t thinningIteration( cv::Mat& imageA, cv::Mat& imageB,
                           const int32_t iteration )
{
    int32_t changedPixel { };

    std::array< uint8_t, 9 > neighbors { };

    auto getNeighbors =
        [ &neighbors, &imageA ]( const int32_t x, const int32_t y )
    {
        const auto rowPtrYP1 = imageA.ptr< uint8_t >( y + 1 );
        const auto rowPtrY = imageA.ptr< uint8_t >( y );
        const auto rowPtrYM1 = imageA.ptr< uint8_t >( y - 1 );

        constexpr auto lbl = uint8_t { 255 };

        neighbors[ 0 ] =
            rowPtrY[ x ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P1

        neighbors[ 1 ] =
            rowPtrYM1[ x ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P2

        neighbors[ 2 ] =
            rowPtrYM1[ x + 1 ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P3

        neighbors[ 3 ] =
            rowPtrY[ x + 1 ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P4

        neighbors[ 4 ] =
            rowPtrYP1[ x + 1 ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P5

        neighbors[ 5 ] =
            rowPtrYP1[ x ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P6

        neighbors[ 6 ] =
            rowPtrYP1[ x - 1 ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P7

        neighbors[ 7 ] =
            rowPtrY[ x - 1 ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P8

        neighbors[ 8 ] =
            rowPtrYM1[ x - 1 ] == lbl ? uint8_t { 1 } : uint8_t { 0 }; // P9
    };

    auto getTransitions = [ & ]( )
    {
        uint8_t transitions { };
        // P2 P3 P4 P5 P6 P7 P8 P9 P2
        transitions += neighbors[ 1 ] == 0 && neighbors[ 2 ] != 0; // P2 - P3
        transitions += neighbors[ 2 ] == 0 && neighbors[ 3 ] != 0; // P3 - P4
        transitions += neighbors[ 3 ] == 0 && neighbors[ 4 ] != 0; // P4 - P5
        transitions += neighbors[ 4 ] == 0 && neighbors[ 5 ] != 0; // P5 - P6
        transitions += neighbors[ 5 ] == 0 && neighbors[ 6 ] != 0; // P6 - P7
        transitions += neighbors[ 6 ] == 0 && neighbors[ 7 ] != 0; // P7 - P8
        transitions += neighbors[ 7 ] == 0 && neighbors[ 8 ] != 0; // P8 - P9
        transitions += neighbors[ 8 ] == 0 && neighbors[ 1 ] != 0; // P9 - P2

        return transitions;
    };

    auto sumNeighbors = [ & ]( )
    {
        uint8_t sum { };
        sum += neighbors[ 1 ];
        sum += neighbors[ 2 ];
        sum += neighbors[ 3 ];
        sum += neighbors[ 4 ];
        sum += neighbors[ 5 ];
        sum += neighbors[ 6 ];
        sum += neighbors[ 7 ];
        sum += neighbors[ 8 ];
        return sum;
    };

    for ( int32_t y = 1; y < imageA.rows - 1; ++y )
    {
        const auto rowPtrSrcA = imageA.ptr< uint8_t >( y );
        const auto rowPtrSrcB = imageB.ptr< uint8_t >( y );

        for ( int32_t x = 1; x < imageA.cols - 1; ++x )
        {
            if ( *( rowPtrSrcA + x ) != 0 )
            {
                //  0  1  2  3  4  5  6  7  8
                //  P1 P2 P3 P4 P5 P6 P7 P8 P9
                getNeighbors( x, y );

                const auto A = getTransitions( );

                const auto B = sumNeighbors( );

                const auto m1 = iteration == 0
                                    ? static_cast< uint8_t >(
                                          neighbors[ 1 ] * neighbors[ 3 ] *
                                          neighbors[ 5 ] ) // P2 * P4 * P6
                                    : static_cast< uint8_t >(
                                          neighbors[ 1 ] * neighbors[ 3 ] *
                                          neighbors[ 7 ] ); // P2 * P4 * P8
                const auto m2 = iteration == 0
                                    ? static_cast< uint8_t >(
                                          neighbors[ 3 ] * neighbors[ 5 ] *
                                          neighbors[ 7 ] ) // P4 * P6 * P8
                                    : static_cast< uint8_t >(
                                          neighbors[ 1 ] * neighbors[ 5 ] *
                                          neighbors[ 7 ] ); // P2 * P6 * P8

                if ( A == uint8_t { 1 } &&
                     ( B >= uint8_t { 2 } && B <= uint8_t { 6 } ) &&
                     m1 == uint8_t { 0 } && m2 == uint8_t { 0 } )
                {
                    changedPixel++;
                    rowPtrSrcB[ x ] = uint8_t { 0 };
                }
            }
        }
    }

    return changedPixel;
}

/*
 * Function that returns the magnitude neighbourhood for a certain pixel
 *
 * @param [in]  derivationX     The derivative in x direction
 * @param [in]  derivationY     The derivative in y direction
 * @param [in]  position        The current position
 * @param [in]  neighbourhood   The neighbourhood to use
 * @param [in]  magnitudes      A vector receiving the results
 *
 */
void magnitudeNeighbourhood( const cv::Mat& derivationX,
                             const cv::Mat& derivationY,
                             const cv::Point2i& position,
                             const Neighbourhood& neighbourhood,
                             std::vector< float >& magnitudes )
{
    const auto imageWidth = derivationX.cols;
    const auto imageHeight = derivationX.rows;

    const auto x = position.x;
    const auto y = position.y;

    const auto top = y - 1 >= 0 ? y - 1 : y;
    const auto down = y + 1 < imageHeight ? y + 1 : y;
    const auto left = x - 1 >= 0 ? x - 1 : x;
    const auto right = x + 1 < imageWidth ? x + 1 : x;

    switch ( neighbourhood )
    {
    case Neighbourhood::FourConnected:
    {
        magnitudes[ 0 ] =
            amplitude( derivationX, derivationY, cv::Point2i( x, top ) );
        magnitudes[ 1 ] =
            amplitude( derivationX, derivationY, cv::Point2i( left, y ) );
        magnitudes[ 2 ] =
            amplitude( derivationX, derivationY, cv::Point2i( x, y ) );
        magnitudes[ 3 ] =
            amplitude( derivationX, derivationY, cv::Point2i( right, y ) );
        magnitudes[ 4 ] =
            amplitude( derivationX, derivationY, cv::Point2i( x, down ) );
        break;
    }

    case Neighbourhood::EightConnected:
    {
        magnitudes[ 0 ] =
            amplitude( derivationX, derivationY, cv::Point2i( left, top ) );
        magnitudes[ 1 ] =
            amplitude( derivationX, derivationY, cv::Point2i( x, top ) );
        magnitudes[ 2 ] =
            amplitude( derivationX, derivationY, cv::Point2i( right, top ) );
        magnitudes[ 3 ] =
            amplitude( derivationX, derivationY, cv::Point2i( left, y ) );
        magnitudes[ 4 ] =
            amplitude( derivationX, derivationY, cv::Point2i( x, y ) );
        magnitudes[ 5 ] =
            amplitude( derivationX, derivationY, cv::Point2i( right, y ) );
        magnitudes[ 6 ] =
            amplitude( derivationX, derivationY, cv::Point2i( left, down ) );
        magnitudes[ 7 ] =
            amplitude( derivationX, derivationY, cv::Point2i( x, down ) );
        magnitudes[ 8 ] =
            amplitude( derivationX, derivationY, cv::Point2i( right, down ) );
        break;
    }
    }
}

/*
 * Function that calculates the amplitude of an image at a certain position
 *
 * @param [in]  derivationX     The derivative in x direction
 * @param [in]  derivationY     The derivative in y direction
 * @param [in]  position        The current position
 *
 */
float amplitude( const cv::Mat& derivationX, const cv::Mat& derivationY,
                 const cv::Point2i& position )
{
    const auto x = position.x;
    const auto y = position.y;

    const auto rowPtrX = derivationX.ptr< int16_t >( y );
    const auto rowPtrY = derivationY.ptr< int16_t >( y );

    return static_cast< float >( std::abs( rowPtrX[ x ] ) +
                                 std::abs( rowPtrY[ x ] ) );
}

/*
 * Function that calculates second facet model for a certain pixel based on the
 * magnitude neighbourhood.
 *
 * @param [in]  magnitudes          The magnitudes for the pixel position
 * @param [in]  secondFacetModel    A vector receiving the second faced model.
 *
 */
void secondFacetModel( const std::vector< float >& magnitudes,
                       std::vector< float >& secondFacetModel )
{
    if ( secondFacetModel.size( ) != 6 && magnitudes.size( ) != 9 )
    {
        throw std::invalid_argument(
            "The size of the incoming vector for the second "
            "facet model or the magnitudes is wrong" );
    }

    secondFacetModel[ 0 ] = static_cast< float >(
        ( magnitudes[ 0 ] + magnitudes[ 1 ] + magnitudes[ 2 ] +
          magnitudes[ 3 ] + magnitudes[ 4 ] + magnitudes[ 5 ] +
          magnitudes[ 6 ] + magnitudes[ 7 ] + magnitudes[ 8 ] ) /
        9.0 );

    secondFacetModel[ 1 ] = static_cast< float >(
        ( -magnitudes[ 0 ] + magnitudes[ 2 ] - magnitudes[ 3 ] +
          magnitudes[ 5 ] - magnitudes[ 6 ] + magnitudes[ 8 ] ) /
        6.0 );

    secondFacetModel[ 2 ] = static_cast< float >(
        ( magnitudes[ 6 ] + magnitudes[ 7 ] + magnitudes[ 8 ] -
          magnitudes[ 0 ] - magnitudes[ 1 ] - magnitudes[ 2 ] ) /
        6.0 );

    secondFacetModel[ 3 ] = static_cast< float >(
        ( magnitudes[ 0 ] - 2.0 * magnitudes[ 1 ] + magnitudes[ 2 ] +
          magnitudes[ 3 ] - 2.0 * magnitudes[ 4 ] + magnitudes[ 5 ] +
          magnitudes[ 6 ] - 2.0 * magnitudes[ 7 ] + magnitudes[ 8 ] ) /
        6.0 );

    secondFacetModel[ 4 ] =
        static_cast< float >( ( -magnitudes[ 0 ] + magnitudes[ 2 ] +
                                magnitudes[ 6 ] - magnitudes[ 8 ] ) /
                              4.0 );

    secondFacetModel[ 5 ] = static_cast< float >(
        ( magnitudes[ 0 ] + magnitudes[ 1 ] + magnitudes[ 2 ] -
          2.0 * ( magnitudes[ 3 ] + magnitudes[ 4 ] + magnitudes[ 5 ] ) +
          magnitudes[ 6 ] + magnitudes[ 7 ] + magnitudes[ 8 ] ) /
        6.0 );
}

/*
 * Function that calculates the sub pixel coordinate for a certain pixel using
 * interpolation along the gradient
 *
 * @param [in]  image           The input image
 * @param [in]  pos             The current position
 * @param [in]  derivativeX     The derivative of the image in x direction
 * @param [in]  derivativeY     The derivative of the image in y direction
 * @param [in]  subPixelPoint   The calculated subpixel point
 * @param [in]  response        The calculated response
 * @param [in]  direction       The calculated direction
 *
 */
void extractSubPixelPosition( const cv::Mat& image, const cv::Point& pos,
                              const cv::Mat& derivativeX,
                              const cv::Mat& derivativeY,
                              cv::Point2f& subPixelPoint, float& response,
                              cv::Point2f& direction )
{
    /*extractSubPixelPositionSecondFacet( image,
                                        pos,
                                        derivativeX,
                                        derivativeY,
                                        subPixelPoint,
                                        response,
                                        direction );*/

    extractSubPixelPositionInterpolation( image,
                                          pos,
                                          derivativeX,
                                          derivativeY,
                                          subPixelPoint,
                                          response,
                                          direction );
}

void extractSubPixelPositionSecondFacet(
    const cv::Mat& image, const cv::Point& pos, const cv::Mat& derivativeX,
    const cv::Mat& derivativeY, cv::Point2f& subPixelPoint, float& response,
    cv::Point2f& direction )
{
    std::vector< float > magnitudes( 9 );
    std::vector< float > facetModel( 6 );

    /*magnitudeNeighbourhood( imageSobelX,
                                        imageSobelY,
                                        point,
                                        Neighbourhood::EightConnected,
                                        magnitudes );*/

    imageNeighbourhood( image, pos, Neighbourhood::EightConnected, magnitudes );

    secondFacetModel( magnitudes, facetModel );

    //     | a     b |
    // H:= |         |
    //     | c     d |

    //     | fxx fxy |
    // H:= |         |
    //     | fyx fyy |

    // derivatives
    // f    = fm[ 0 ]
    // fx   = fm[ 1 ]
    // fy   = fm[ 2 ]
    // fxy  = fm[ 4 ]
    // fxx  = fm[ 3 ]
    // fyy  = fm[ 5 ]

    const auto f = facetModel[ 0 ];
    const auto fx = facetModel[ 1 ];
    const auto fy = facetModel[ 2 ];
    const auto fxy = facetModel[ 4 ];
    const auto fxx = facetModel[ 3 ];
    const auto fyy = facetModel[ 5 ];
    std::ignore = f;

    cv::Point2f eigenVector1;
    cv::Point2f eigenVector2;
    float eigenValue1 { };
    float eigenValue2 { };

    cv::Mat H( 2, 2, CV_32FC1 );
    H.at< float >( 0, 0 ) = fxx;
    H.at< float >( 0, 1 ) = fxy;
    H.at< float >( 1, 0 ) = fxy;
    H.at< float >( 1, 1 ) = fyy;

    calculateEigenValuesVectors(
        H, eigenValue1, eigenValue2, eigenVector1, eigenVector2 );

    if ( eigenValue1 < eigenValue2 )
    {
        std::cout << "Error\n";
    }

    const double nx = eigenVector1.x;
    const double ny = eigenVector1.y;
    /*response = std::abs( eigenValue1 );*/
    /*response = std::abs( f );*/
    response = std::sqrt( fx * fx + fy * fy );

    // Having the highest eigenvalue and corresponding eigenvector defined on
    // position 1 we can calculate the sub pixel offset described in the paper.

    //                    rx * nx + ry * ny
    // t = - -------------------------------------------
    //       rxx * nx^2 + 2 * rxy * nx * ny + ryy * ny^2

    const auto divisor = fxx * nx * nx + 2.0 * fxy * nx * ny + fyy * ny * ny;
    double t { };
    if ( ! isZero( divisor ) )
    {
        t = -( fx * nx + fy * ny ) / divisor;
    }

    // Having t the sub pixel point is defined as:
    // (px, py) = (t * nx, t * ny)
    // And the rule for (px, py) ELEMENT [-0.5, 0.5] X [-0.5, 0.5]
    double px = nx * t;
    double py = ny * t;

    if ( std::fabs( px ) > 0.5 )
    {
        px = 0.0;
    }

    if ( std::fabs( py ) > 0.5 )
    {
        py = 0.0;
    }

    auto x = static_cast< double >( pos.x );
    auto y = static_cast< double >( pos.y );

    x += px;
    y += py;

    subPixelPoint = { static_cast< float >( x ), static_cast< float >( y ) };

    direction = { static_cast< float >( nx ), static_cast< float >( ny ) };
    // const double a = std::atan2( y, x );
    // direction = a >= 0.0 ? a : a + CV_2PI;
}

void extractSubPixelPositionInterpolation(
    const cv::Mat& image, const cv::Point& pos, const cv::Mat& derivativeX,
    const cv::Mat& derivativeY, cv::Point2f& subPixelPoint, float& response,
    cv::Point2f& direction )
{
    const auto nx = static_cast< float >( derivativeX.at< int16_t >( pos ) );
    const auto ny = static_cast< float >( derivativeY.at< int16_t >( pos ) );

    direction = cv::Point2f( nx, ny );
    direction = direction / cv::norm( direction );

    auto dir = std::atan2( ny, nx ) * 180.0f / CV_PI + 180.0f;

    constexpr std::array< int32_t, 4 > posXp { { 1, 1, 0, -1 } };
    constexpr std::array< int32_t, 4 > posXm { { -1, -1, 0, 1 } };

    constexpr std::array< int32_t, 4 > posYp { { 0, -1, -1, -1 } };
    constexpr std::array< int32_t, 4 > posYm { { 0, 1, 1, 1 } };

    auto d = [ &dir ]( )
    {
        if ( ( dir > 157.5 && dir <= 202.5 ) || dir <= 22.5 || dir > 337.5 )
        {
            return 0;
        }

        if ( ( dir > 22.5 && dir <= 67.5 ) || ( dir > 202.5 && dir <= 247.5 ) )
        {
            return 1;
        }

        if ( ( dir > 67.5 && dir <= 112.5 ) || ( dir > 247.5 && dir <= 292.5 ) )
        {
            return 2;
        }

        return 3;
    };

    const auto edgeDir = d( );

    const auto x = pos.x;
    const auto y = pos.y;

    auto xp = x + posXp.at( edgeDir );
    auto xm = x + posXm.at( edgeDir );

    auto yp = y + posYp.at( edgeDir );
    auto ym = y + posYm.at( edgeDir );

    if ( xm < 0 )
    {
        xm = 0;
    }

    if ( xp >= derivativeX.cols )
    {
        xp = derivativeX.cols - 1;
    }

    if ( ym < 0 )
    {
        ym = 0;
    }

    if ( yp >= derivativeX.rows )
    {
        yp = derivativeX.rows - 1;
    }

    const auto Kp = amplitude( derivativeX, derivativeY, { xp, yp } );
    const auto Km = amplitude( derivativeX, derivativeY, { xm, ym } );
    const auto Ko = amplitude( derivativeX, derivativeY, { x, y } );

    response = Ko;

    //            Km - Kp
    // n = ------------------------
    //     2 * ( Km - 2 * Ko + Kp )
    const auto top = Km - Kp;
    const auto bottom = Km - 2.0f * Ko + Kp;

    float n { };
    if ( ! isZero( bottom ) )
    {
        n = 0.5f * top / bottom;
    }

    if ( std::fabs( n ) > 0.5 )
    {
        n = 0.0f;
    }

    subPixelPoint = cv::Point2f( static_cast< float >( x ) + n,
                                 static_cast< float >( y ) + n );
}

void calculateEigenValuesVectors( const cv::Mat& src, float& eigenValue1,
                                  float& eigenValue2, cv::Point2f& eigenVector1,
                                  cv::Point2f& eigenVector2 )
{
    calculateEigenValuesVectorsCv(
        src, eigenValue1, eigenValue2, eigenVector1, eigenVector2 );

    /* calculateEigenValuesVectorsSelf(
         src, eigenValue1, eigenValue2, eigenVector1, eigenVector2 );*/
}

void calculateEigenValuesVectorsCv( const cv::Mat& src, float& eigenValue1,
                                    float& eigenValue2,
                                    cv::Point2f& eigenVector1,
                                    cv::Point2f& eigenVector2 )
{
    cv::Mat eigenValues;
    cv::Mat eigenVectors;

    cv::eigen( src, eigenValues, eigenVectors );
    // const cv::PCA pca( src, cv::Mat( ), cv::PCA::DATA_AS_ROW, 0 );

    eigenValue1 = eigenValues.at< float >( 0, 0 );
    eigenValue2 = eigenValues.at< float >( 1, 0 );
    // std::cout << eigenValues << '\n';

    eigenVector1 = eigenVectors.at< cv::Vec2f >( 0, 0 );
    eigenVector2 = eigenVectors.at< cv::Vec2f >( 1, 0 );
    // std::cout << eigenVectors << '\n';
}

void calculateEigenValuesVectorsSelf( const cv::Mat& src, float& eigenValue1,
                                      float& eigenValue2,
                                      cv::Point2f& eigenVector1,
                                      cv::Point2f& eigenVector2 )
{
    const auto fxx = src.at< float >( 0, 0 );
    const auto fxy = src.at< float >( 0, 1 );
    const auto fyy = src.at< float >( 1, 1 );

    // According to the paper we need to calculate the eigenvalues and
    // eigenvectors of the hessian to determine the subpixel position
    // https://people.math.harvard.edu/~knill/teaching/math21b2004/exhibits/2dmatrices/index.html

    //        (fxx + fyy)       (4 * fxy^2 + (fxx - fyy)^2)^1/2
    // L1,2 = -----------  +/-  -------------------------------
    //             2                            2

    const auto firstTerm = 0.5f * ( fxx + fyy );
    const auto secondTerm =
        0.5f * std::sqrt( 4.0f * fxy * fxy + ( fxx - fyy ) * ( fxx - fyy ) );

    eigenValue1 = firstTerm - secondTerm;

    eigenValue2 = firstTerm + secondTerm;

    if ( eigenValue2 > eigenValue1 )
    {
        std::swap( eigenValue1, eigenValue2 );
    }

    // If element b or c is equal to zero we cannot calculate the eigenvectors.
    // We set the eigenvectors to [0, 1] and [1, 0]
    if ( isZero( fxy ) )
    {
        eigenVector1 = { 1.0, 0.0 };
        eigenVector2 = { 0.0, 1.0 };
    }
    else
    {
        // To get the corresponding eigenvector we need to full fill
        // ( A - lambda * I) * v = 0
        //
        // | (a - lambda)       b       |   | x |   | 0 |
        // |                            | * |   | = |   |
        // |      c        (d - lambda) |   | y |   | 0 |
        //
        // (a - lambda) * x + b * y = 0
        // c * x + (d - lambda) * y = 0
        //
        // y = (lambda - a) * x / b
        // y = c * x / (lambda - d)
        //
        // y = y
        //
        // (lambda - a) * x       c * x
        // ----------------- = -------------
        //         b           (lambda - d)
        //
        // Choosing x to be denominator of each fraction
        //
        // lambda - a        c
        // ----------- = ----------
        //      b        lambda - d
        //
        // e.g x = b leads to
        // x = b and y = lambda - a
        //
        // e.g x = c  leads to
        //
        // reforming
        // x = y * (lambda - d) / c and choosing y = c leads to
        // x = lambda - d and y = c
        //
        //      |     b      |        | lambda - d |
        // v1 = |            | ; v2 = |            |
        //      | lambda - a |        |     c      |

        // Now we need the corresponding eigenvectors
        eigenVector1 = { fxy, eigenValue1 - fxx };
        eigenVector2 = { eigenValue2 - fyy, fxy };

        eigenVector1 /= cv::norm( eigenVector1 );
        eigenVector2 /= cv::norm( eigenVector2 );
    }
}

void imageNeighbourhood( const cv::Mat& image, const cv::Point2i& position,
                         const Neighbourhood& neighbourhood,
                         std::vector< float >& magnitudes )
{
    const auto imageWidth = image.cols;
    const auto imageHeight = image.rows;

    const auto x = position.x;
    const auto y = position.y;

    const auto top = y - 1 >= 0 ? y - 1 : y;
    const auto down = y + 1 < imageHeight ? y + 1 : y;
    const auto left = x - 1 >= 0 ? x - 1 : x;
    const auto right = x + 1 < imageWidth ? x + 1 : x;

    switch ( neighbourhood )
    {
    case Neighbourhood::FourConnected:
    {
        magnitudes[ 0 ] = pixelValue( image, cv::Point2i( x, top ) );
        magnitudes[ 1 ] = pixelValue( image, cv::Point2i( left, y ) );
        magnitudes[ 2 ] = pixelValue( image, cv::Point2i( x, y ) );
        magnitudes[ 3 ] = pixelValue( image, cv::Point2i( right, y ) );
        magnitudes[ 4 ] = pixelValue( image, cv::Point2i( x, down ) );
        break;
    }

    case Neighbourhood::EightConnected:
    {
        magnitudes[ 0 ] = pixelValue( image, cv::Point2i( left, top ) );
        magnitudes[ 1 ] = pixelValue( image, cv::Point2i( x, top ) );
        magnitudes[ 2 ] = pixelValue( image, cv::Point2i( right, top ) );
        magnitudes[ 3 ] = pixelValue( image, cv::Point2i( left, y ) );
        magnitudes[ 4 ] = pixelValue( image, cv::Point2i( x, y ) );
        magnitudes[ 5 ] = pixelValue( image, cv::Point2i( right, y ) );
        magnitudes[ 6 ] = pixelValue( image, cv::Point2i( left, down ) );
        magnitudes[ 7 ] = pixelValue( image, cv::Point2i( x, down ) );
        magnitudes[ 8 ] = pixelValue( image, cv::Point2i( right, down ) );
        break;
    }
    }
}

float pixelValue( const cv::Mat& image, const cv::Point2i& position )
{
    const auto x = position.x;
    const auto y = position.y;

    const auto rowPtr = image.ptr< uint8_t >( y );

    return rowPtr[ x ];
}