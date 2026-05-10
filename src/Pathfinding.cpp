#include "Pathfinding.h"
#include <queue>
#include <limits>
#include <chrono>
#include <algorithm>
#include <stack>

PathfindingHistory runBFS(const Graph& graph, int startNode, int endNode) {
    PathfindingHistory history;
    int n = graph.adjacencyList.size();
    if (n == 0 || startNode < 0 || endNode >= n) return history;

    auto startTime = chrono::high_resolution_clock::now();

    vector<bool> reached(n, false);
    vector<int> parent(n, -1);
    queue<int> q;

    reached[startNode] = true;
    q.push(startNode);

    if (startNode == endNode) {
        history.pathFound = true;
        history.finalPath = {startNode};
        return history;
    }

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        ExpansionStep step;
        step.expandedNode = u;
        step.deltas.push_back({u, NodeState::EXPANDING});

        for (const EdgeData& edge : graph.adjacencyList[u]) {
            int v = edge.targetIndex;
            if (!reached[v]) {
                reached[v] = true;
                parent[v] = u;
                step.deltas.push_back({v, NodeState::FRONTIER});
                
                // Early goal test upon generation
                if (v == endNode) {
                    history.pathFound = true;
                    history.steps.push_back(step);
                    goto reconstruct_path; 
                }
                q.push(v);
            }
        }
        history.steps.push_back(step);
    }

reconstruct_path:
    auto endTime = chrono::high_resolution_clock::now();
    history.executionTimeMicroseconds = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

    if (history.pathFound) {
        int curr = endNode;
        while (curr != -1) {
            history.finalPath.push_back(curr);
            curr = parent[curr];
        }
        reverse(history.finalPath.begin(), history.finalPath.end());

        // --- NEW: Calculate True Total Cost ---
        history.totalCost = 0.0f;
        for (size_t i = 0; i < history.finalPath.size() - 1; ++i) {
            int u = history.finalPath[i];
            int v = history.finalPath[i + 1];
            
            // Find the edge weight between u and v
            for (const EdgeData& edge : graph.adjacencyList[u]) {
                if (edge.targetIndex == v) {
                    history.totalCost += edge.weight;
                    break;
                }
            }
        }
    }
    return history;
}


PathfindingHistory runDFS(const Graph& graph, int startNode, int endNode) {
    PathfindingHistory history;
    int n = graph.adjacencyList.size();
    if (n == 0 || startNode < 0 || endNode >= n) return history;

    auto startTime = chrono::high_resolution_clock::now();

    vector<bool> reached(n, false);
    vector<int> parent(n, -1);
    stack<int> s;

    s.push(startNode);

    while (!s.empty()) {
        int u = s.top();
        s.pop();

        if (reached[u]) continue;
        reached[u] = true;

        ExpansionStep step;
        step.expandedNode = u;
        step.deltas.push_back({u, NodeState::EXPANDING});

        // Goal test upon expansion
        if (u == endNode) {
            history.pathFound = true;
            history.steps.push_back(step);
            break;
        }

        // Reverse iterate to maintain lexicographical exploration order on stack
        for (auto it = graph.adjacencyList[u].rbegin(); it != graph.adjacencyList[u].rend(); ++it) {
            int v = it->targetIndex;
            if (!reached[v]) {
                parent[v] = u; // Overwrites if multiple paths exist, characteristic of DFS
                s.push(v);
                step.deltas.push_back({v, NodeState::FRONTIER});
            }
        }
        history.steps.push_back(step);
    }

    auto endTime = chrono::high_resolution_clock::now();
    history.executionTimeMicroseconds = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

    if (history.pathFound) {
        int curr = endNode;
        while (curr != -1) {
            history.finalPath.push_back(curr);
            curr = parent[curr];
        }
        reverse(history.finalPath.begin(), history.finalPath.end());

        // --- NEW: Calculate True Total Cost ---
        history.totalCost = 0.0f;
        for (size_t i = 0; i < history.finalPath.size() - 1; ++i) {
            int u = history.finalPath[i];
            int v = history.finalPath[i + 1];
            
            // Find the edge weight between u and v
            for (const EdgeData& edge : graph.adjacencyList[u]) {
                if (edge.targetIndex == v) {
                    history.totalCost += edge.weight;
                    break;
                }
            }
        }
    }
    return history;
}


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


PathfindingHistory runAStar(const Graph& graph, int startNode, int endNode, const vector<float>& h) {
    PathfindingHistory history;
    int n = graph.adjacencyList.size();
    if (n == 0 || startNode < 0 || endNode >= n) return history;

    auto startTime = chrono::high_resolution_clock::now();

    vector<float> dist(n, numeric_limits<float>::infinity());
    vector<int> parent(n, -1);
    vector<bool> closed(n, false);
    
    // Min-heap: {f_score, nodeIndex}
    using pdi = pair<float, int>;
    priority_queue<pdi, vector<pdi>, greater<pdi>> pq;

    dist[startNode] = 0.0f;
    pq.push({h[startNode], startNode});

    while (!pq.empty()) {
        int u = pq.top().second;
        pq.pop();

        if (closed[u]) continue;
        closed[u] = true;

        ExpansionStep step;
        step.expandedNode = u;
        step.deltas.push_back({u, NodeState::EXPANDING});

        if (u == endNode) {
            history.pathFound = true;
            history.steps.push_back(step);
            break;
        }

        for (const EdgeData& edge : graph.adjacencyList[u]) {
            int v = edge.targetIndex;
            float weight = edge.weight;

            if (closed[v]) continue;

            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
                float f_score = dist[v] + h[v];
                pq.push({f_score, v});
                
                step.deltas.push_back({v, NodeState::FRONTIER});
            }
        }
        history.steps.push_back(step);
    }

    auto endTime = chrono::high_resolution_clock::now();
    history.executionTimeMicroseconds = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

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


PathfindingHistory runGreedy(const Graph& graph, int startNode, int endNode, const vector<float>& h) {
    PathfindingHistory history;
    int n = graph.adjacencyList.size();
    if (n == 0 || startNode < 0 || endNode >= n) return history;

    auto startTime = chrono::high_resolution_clock::now();

    vector<bool> closed(n, false);
    vector<int> parent(n, -1);
    
    using pdi = pair<float, int>;
    priority_queue<pdi, vector<pdi>, greater<pdi>> pq;

    pq.push({h[startNode], startNode});

    while (!pq.empty()) {
        int u = pq.top().second;
        pq.pop();

        if (closed[u]) continue;
        closed[u] = true;

        ExpansionStep step;
        step.expandedNode = u;
        step.deltas.push_back({u, NodeState::EXPANDING});

        if (u == endNode) {
            history.pathFound = true;
            history.steps.push_back(step);
            break;
        }

        for (const EdgeData& edge : graph.adjacencyList[u]) {
            int v = edge.targetIndex;
            if (!closed[v] && parent[v] == -1) { // Only set parent if unvisited
                parent[v] = u;
                pq.push({h[v], v});
                step.deltas.push_back({v, NodeState::FRONTIER});
            }
        }
        history.steps.push_back(step);
    }

    auto endTime = chrono::high_resolution_clock::now();
    history.executionTimeMicroseconds = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

    if (history.pathFound) {
        int curr = endNode;
        while (curr != -1) {
            history.finalPath.push_back(curr);
            curr = parent[curr];
        }
        reverse(history.finalPath.begin(), history.finalPath.end());

        // --- NEW: Calculate True Total Cost ---
        history.totalCost = 0.0f;
        for (size_t i = 0; i < history.finalPath.size() - 1; ++i) {
            int u = history.finalPath[i];
            int v = history.finalPath[i + 1];
            
            // Find the edge weight between u and v
            for (const EdgeData& edge : graph.adjacencyList[u]) {
                if (edge.targetIndex == v) {
                    history.totalCost += edge.weight;
                    break;
                }
            }
        }
    }
    return history;
}


PathfindingHistory runHillClimbing(const Graph& graph, int startNode, int endNode, const vector<float>& h) {
    PathfindingHistory history;
    int n = graph.adjacencyList.size();
    if (n == 0 || startNode < 0 || endNode >= n) return history;

    auto startTime = chrono::high_resolution_clock::now();

    int current = startNode;
    float current_g = 0.0f;
    vector<int> path = {startNode};

    while (true) {
        ExpansionStep step;
        step.expandedNode = current;
        step.deltas.push_back({current, NodeState::EXPANDING});

        if (current == endNode) {
            history.pathFound = true;
            history.steps.push_back(step);
            break;
        }

        int best_neighbor = -1;
        float best_g = 0.0f;
        float max_eval = -numeric_limits<float>::infinity();
        float current_eval = -(current_g + h[current]);

        for (const EdgeData& edge : graph.adjacencyList[current]) {
            int v = edge.targetIndex;
            float next_g = current_g + edge.weight;
            float eval = -(next_g + h[v]); 

            step.deltas.push_back({v, NodeState::FRONTIER});

            if (eval > max_eval) {
                max_eval = eval;
                best_neighbor = v;
                best_g = next_g;
            }
        }
        history.steps.push_back(step);

        // Terminate at local maximum (no neighbor is strictly better)
        if (best_neighbor == -1 || max_eval <= current_eval) break;

        current = best_neighbor;
        current_g = best_g;
        path.push_back(current);
    }

    auto endTime = chrono::high_resolution_clock::now();
    history.executionTimeMicroseconds = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

    if (history.pathFound) {
        history.finalPath = path;
        history.totalCost = current_g;
    }
    return history;
}


float searchIDAStar(int node, float g, float threshold, int endNode, 
                    const vector<float>& h, const Graph& graph, 
                    vector<int>& path, PathfindingHistory& history) {
    
    float f = g + h[node];
    if (f > threshold) return f;

    ExpansionStep step;
    step.expandedNode = node;
    step.deltas.push_back({node, NodeState::EXPANDING});
    
    if (node == endNode) {
        history.pathFound = true;
        history.steps.push_back(step);
        return -1.0f; // -1 denotes FOUND
    }

    float min_val = numeric_limits<float>::infinity();
    for (const EdgeData& edge : graph.adjacencyList[node]) {
        int v = edge.targetIndex;
        
        // Prevent simple cycles within the current DFS branch
        if (find(path.begin(), path.end(), v) == path.end()) {
            step.deltas.push_back({v, NodeState::FRONTIER});
        }
    }
    history.steps.push_back(step);

    for (const EdgeData& edge : graph.adjacencyList[node]) {
        int v = edge.targetIndex;
        if (find(path.begin(), path.end(), v) == path.end()) {
            path.push_back(v);
            float temp = searchIDAStar(v, g + edge.weight, threshold, endNode, h, graph, path, history);
            if (temp == -1.0f) return -1.0f;
            if (temp < min_val) min_val = temp;
            path.pop_back();
        }
    }
    return min_val;
}

PathfindingHistory runIDAStar(const Graph& graph, int startNode, int endNode, const vector<float>& h) {
    PathfindingHistory history;
    int n = graph.adjacencyList.size();
    if (n == 0 || startNode < 0 || endNode >= n) return history;

    auto startTime = chrono::high_resolution_clock::now();

    float threshold = h[startNode];
    vector<int> path = {startNode};

    while (!history.pathFound) {
        float temp = searchIDAStar(startNode, 0.0f, threshold, endNode, h, graph, path, history);
        
        if (temp == -1.0f) { // Found
            break; 
        }
        if (temp == numeric_limits<float>::infinity()) { // No path exists
            break;
        }
        threshold = temp;
    }

    auto endTime = chrono::high_resolution_clock::now();
    history.executionTimeMicroseconds = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

    if (history.pathFound) {
        history.finalPath = path;
    }
    // --- NEW: Calculate True Total Cost ---
    history.totalCost = 0.0f;
    for (size_t i = 0; i < history.finalPath.size() - 1; ++i) {
        int u = history.finalPath[i];
        int v = history.finalPath[i + 1];
        
        // Find the edge weight between u and v
        for (const EdgeData& edge : graph.adjacencyList[u]) {
            if (edge.targetIndex == v) {
                history.totalCost += edge.weight;
                break;
            }
        }
    }
    return history;
}
