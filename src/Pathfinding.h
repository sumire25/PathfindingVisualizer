#pragma once
#include "Graph.h"
using namespace std;

// UNINFORMED SEARCH ALGORITHMS
PathfindingHistory runBFS(const Graph& graph, int startNode, int endNode);
PathfindingHistory runDFS(const Graph& graph, int startNode, int endNode);
PathfindingHistory runDijkstra(const Graph& graph, int startNode, int endNode);

// INFORMED (HEURISTIC) SEARCH ALGORITHMS
PathfindingHistory runAStar(const Graph& graph, int startNode, int endNode, const vector<float>& h);
PathfindingHistory runGreedy(const Graph& graph, int startNode, int endNode, const vector<float>& h);
PathfindingHistory runHillClimbing(const Graph& graph, int startNode, int endNode, const vector<float>& h);
PathfindingHistory runIDAStar(const Graph& graph, int startNode, int endNode, const vector<float>& h);