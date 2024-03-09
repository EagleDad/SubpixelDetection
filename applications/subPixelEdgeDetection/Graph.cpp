#include "Graph.h"

// Std includes
#include <queue>

using intPair = std::pair< int32_t, int32_t >;

Graph::Graph( int32_t numberVertices )
    : mNumberVertices( numberVertices )
{
    mAdjacencyMatrix.resize( static_cast< size_t >( numberVertices ) );
    mShortestPaths.resize( static_cast< size_t >( numberVertices ) );
    // Create a parent vector to back track the shortest path
    mParent.resize( static_cast< size_t >( numberVertices ) );
}

void Graph::addEdge( int32_t u, int32_t v, int32_t weight )
{
    mAdjacencyMatrix[ static_cast< size_t >( u ) ].emplace_back( v, weight );

    mAdjacencyMatrix[ static_cast< size_t >( v ) ].emplace_back( u, weight );
}

void Graph::shortestPath( int32_t source )
{
    shortestPathDijkstra( source );
}

void Graph::shortestPathDijkstra( int32_t source )
{
    mSource = static_cast< size_t >( source );

    // Create a priority queue to store vertices that
    // are being preprocessed. This is weird syntax in C++.
    // Refer below link for details of this syntax
    // https://www.geeksforgeeks.org/implement-min-heap-using-stl/
    std::priority_queue< intPair, std::vector< intPair >, std::greater<> >
        pqpriorityQueue;

    // Create a vector for distances and initialize all
    // distances as infinite (INF)
    std::vector< int32_t > distances( static_cast< size_t >( mNumberVertices ),
                                      std::numeric_limits< int32_t >::max( ) );

    std::fill( mParent.begin( ), mParent.end( ), -1 );

    // Insert source itself in priority queue and initialize
    // its distance as 0.
    pqpriorityQueue.emplace( 0, source );
    distances[ static_cast< size_t >( source ) ] = 0;

    /* Looping till priority queue becomes empty (or all
    distances are not finalized) */
    while ( ! pqpriorityQueue.empty( ) )
    {
        // The first vertex in pair is the minimum distance
        // vertex, extract it from priority queue.
        // vertex label is stored in second of pair (it
        // has to be done this way to keep the vertices
        // sorted distance (distance must be first item
        // in pair)
        const int32_t u = pqpriorityQueue.top( ).second;
        pqpriorityQueue.pop( );

        // 'i' is used to get all adjacent vertices of a
        // vertex
        std::vector< intPair >::iterator i;
        for ( i = mAdjacencyMatrix[ static_cast< size_t >( u ) ].begin( );
              i != mAdjacencyMatrix[ static_cast< size_t >( u ) ].end( );
              ++i )
        {
            // Get vertex label and weight of current
            // adjacent of u.
            int32_t v = i->first;
            const int32_t weight = i->second;

            // If there is shorted path to v through u.
            if ( distances[ static_cast< size_t >( v ) ] >
                 distances[ static_cast< size_t >( u ) ] + weight )
            {
                // Updating distance of v
                distances[ static_cast< size_t >( v ) ] =
                    distances[ static_cast< size_t >( u ) ] + weight;
                pqpriorityQueue.emplace(
                    distances[ static_cast< size_t >( v ) ], v );

                mParent[ static_cast< size_t >( v ) ] = u;
            }
        }
    }
}

void Graph::getShortestMaxPathRecursive( size_t destination,
                                         std::vector< size_t >& shortestPath )
{
    if ( mParent[ destination ] == -1 )
    {
        return;
    }

    getShortestMaxPathRecursive(
        static_cast< size_t >( mParent[ destination ] ), shortestPath );

    shortestPath.push_back( destination );
}

const std::vector< size_t >& Graph::getShortestPath( size_t destination )
{
    if ( ! mShortestPaths[ destination ].empty( ) )
    {
        return mShortestPaths[ destination ];
    }

    mShortestPaths[ destination ].push_back( mSource );

    getShortestMaxPathRecursive( destination, mShortestPaths[ destination ] );

    return mShortestPaths[ destination ];
}
