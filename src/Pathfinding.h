#pragma once
#include "Graph.h"
using namespace std;

// Executes Dijkstra's Algorithm and returns the execution tape
PathfindingHistory runDijkstra(const Graph& graph, int startNode, int endNode);