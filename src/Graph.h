#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include "Geometry.h"
using namespace std;

// Visual states for the shader
enum class NodeState : uint8_t {
    UNVISITED = 0,// gray
    VISITED   = 1,// white
    REACHED   = 2,// light blue
    FRONTIER  = 3,// dark green
    EXPANDING = 4,// green
    PATH      = 5// gold
};

// Represents an atomic change to a single vertex
struct StateDelta {
    int nodeIndex;
    NodeState newState;
};

// Represents one pop from the Priority Queue
struct ExpansionStep {
    int expandedNode;
    vector<StateDelta> deltas; // All neighbors affected during this pop
};

// The complete execution trace
struct PathfindingHistory {
    vector<ExpansionStep> steps;
    vector<int> finalPath;
    float totalCost = 0.0f;
    long long executionTimeMicroseconds = 0;
    bool pathFound = false;
};

// --- Optimized Graph Structures ---

struct EdgeData {
    int targetIndex;
    float weight; // Precomputed Euclidean distance
};

struct Graph {
    vector<vector<EdgeData>> adjacencyList;

    // Updated to accept vector<Vertex>
    void buildFromTopology(const vector<Vertex>& vertices, const vector<int>& edges, int vertexCount) {
        adjacencyList.clear();
        adjacencyList.resize(vertexCount);

        for (size_t i = 0; i < edges.size(); i += 2) {
            int u = edges[i];
            int v = edges[i + 1];

            // Precompute Euclidean distance
            float dx = vertices[u].x - vertices[v].x;
            float dy = vertices[u].y - vertices[v].y;
            float weight = sqrt(dx * dx + dy * dy);

            adjacencyList[u].push_back({v, weight});
            adjacencyList[v].push_back({u, weight}); // Undirected
        }
    }
};