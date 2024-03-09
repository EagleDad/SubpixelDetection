#pragma once

// Std includes
#include <vector>

class Graph
{
public:
    Graph( int32_t numberVertices );

    Graph( ) = delete;
    Graph( const Graph& ) = delete;
    Graph& operator=( const Graph& ) = delete;
    Graph( Graph&& ) = delete;
    Graph& operator=( Graph&& ) = delete;
    virtual ~Graph( ) = default;

    void addEdge( int32_t u, int32_t v, int32_t weight );
    void shortestPath( int32_t source );

    const std::vector< size_t >& getShortestPath( size_t destination );

private:
    void shortestPathDijkstra( int32_t source );

    void getShortestMaxPathRecursive( size_t destination,
                                      std::vector< size_t >& shortestPath );

    int32_t mNumberVertices { };

    // In a weighted graph, we need to store vertex
    // and weight pair for every edge
    std::vector< std::vector< std::pair< int32_t, int32_t > > >
        mAdjacencyMatrix;

    // A vector collecting all calculated shortest path configurations
    std::vector< std::vector< size_t > > mShortestPaths;

    // A vector keeping information about it parent to back track the shortest
    // path
    std::vector< int32_t > mParent;

    // The current source
    size_t mSource { };
};
