#include "SubPixelDetection.h"
#include "Graph.h"

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

///
///
///

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

    // To b able to get sub pixel contours, connected components needs to be
    // labeled. Why not using cv::findContours? The contours returned by
    // cv::findContours are always closed. Means a 1 Pixel line is represented
    // as a rectangular, having the points twice in the contour.
    const auto contoursPix = labelContours( imageCanny );

    // Now that we've found the pixel precise contour points, we can calculate
    // the subpixel position for each point.
    std::vector< std::vector< cv::Point2f > > subPixelContours;
    subPixelContours.reserve( contoursPix.size( ) );

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
            std::ignore = sortedContour;
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