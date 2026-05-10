#include "Pathfinding.h"
#include <queue>
#include <limits>
#include <chrono>
#include <algorithm>

PathfindingHistory runDijkstra(const Graph& graph, int startNode, int endNode) {
    PathfindingHistory history;
    int n = graph.adjacencyList.size();
    if (n == 0 || startNode < 0 || endNode >= n) return history;

    // Start benchmark timer
    auto startTime = chrono::high_resolution_clock::now();

    vector<float> dist(n, numeric_limits<float>::infinity());
    vector<int> parent(n, -1);
    vector<bool> closed(n, false);
    
    // Min-heap: {distance, nodeIndex}
    using pdi = pair<float, int>;
    priority_queue<pdi, vector<pdi>, greater<pdi>> pq;

    dist[startNode] = 0.0f;
    pq.push({0.0f, startNode});

    while (!pq.empty()) {
        int u = pq.top().second;
        float d = pq.top().first;
        pq.pop();

        if (closed[u]) continue;
        closed[u] = true;

        ExpansionStep step;
        step.expandedNode = u;
        step.deltas.push_back({u, NodeState::EXPANDING});

        // Early exit if we reached the target
        if (u == endNode) {
            history.pathFound = true;
            history.steps.push_back(step);
            break;
        }

        // Evaluate neighbors
        for (const EdgeData& edge : graph.adjacencyList[u]) {
            int v = edge.targetIndex;
            float weight = edge.weight; // Fast O(1) memory read, no math required

            if (closed[v]) continue;

            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
                pq.push({dist[v], v});
                
                // Visual tracking: Node was reached and distance improved
                step.deltas.push_back({v, NodeState::REACHED});
            } else {
                // Visual tracking: Node was evaluated but not improved
                step.deltas.push_back({v, NodeState::FRONTIER});
            }
        }
        history.steps.push_back(step);
    }

    // Stop benchmark timer
    auto endTime = chrono::high_resolution_clock::now();
    history.executionTimeMicroseconds = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

    // Reconstruct Path
    if (history.pathFound) {
        history.totalCost = dist[endNode];
        int curr = endNode;
        while (curr != -1) {
            history.finalPath.push_back(curr);
            curr = parent[curr];
        }
        reverse(history.finalPath.begin(), history.finalPath.end());
    }

    return history;
}