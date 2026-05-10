#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include "Geometry.h"
using namespace std;

// Visual states for the shader
enum class NodeState : uint8_t {
    UNVISITED = 0,
    FRONTIER  = 1,
    REACHED   = 2,
    EXPANDING = 3,
    CLOSED    = 4,
    PATH      = 5
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
    std::vector<std::vector<EdgeData>> adjacencyList;

    // Updated to accept std::vector<Vertex>
    void buildFromTopology(const std::vector<Vertex>& vertices, const std::vector<int>& edges, int vertexCount) {
        adjacencyList.clear();
        adjacencyList.resize(vertexCount);

        for (size_t i = 0; i < edges.size(); i += 2) {
            int u = edges[i];
            int v = edges[i + 1];

            // Precompute Euclidean distance
            float dx = vertices[u].x - vertices[v].x;
            float dy = vertices[u].y - vertices[v].y;
            float weight = std::sqrt(dx * dx + dy * dy);

            adjacencyList[u].push_back({v, weight});
            adjacencyList[v].push_back({u, weight}); // Undirected
        }
    }
};