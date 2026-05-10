#include "Triangulation.h"
#include <random>
#include <algorithm>

vector<Vertex> generateRandomPoints(int count) {
    vector<Vertex> points;
    points.reserve(count);
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> dis(-0.9f, 0.9f);
    
    for (int i = 0; i < count; ++i) {
        points.push_back({dis(gen), dis(gen), 0.0f});
    }
    return points;
}

vector<int> bowyerWatson(vector<Vertex>& vertices) {
    vector<Triangle> triangles;
    if (vertices.empty()) return {};

    // 1. Add Super-Triangle enclosing all points (range is -1 to 1)
    int p1 = vertices.size();
    int p2 = p1 + 1;
    int p3 = p1 + 2;
    vertices.push_back({-10.0f, -10.0f, 0.0f});
    vertices.push_back({ 10.0f, -10.0f, 0.0f});
    vertices.push_back({  0.0f,  10.0f, 0.0f});
    triangles.push_back({p1, p2, p3});

    // 2. Iterate through all original points
    for (int i = 0; i < vertices.size() - 3; ++i) {
        vector<Triangle> badTriangles;
        for (const auto& t : triangles) {
            if (t.circumcircleContains(vertices, i)) {
                badTriangles.push_back(t);
            }
        }

        vector<Edge> polygon;
        for (size_t j = 0; j < badTriangles.size(); ++j) {
            Edge edges[3] = {
                {badTriangles[j].a, badTriangles[j].b},
                {badTriangles[j].b, badTriangles[j].c},
                {badTriangles[j].c, badTriangles[j].a}
            };
            
            for (int eIdx = 0; eIdx < 3; ++eIdx) {
                bool shared = false;
                for (size_t k = 0; k < badTriangles.size(); ++k) {
                    if (j == k) continue;
                    Edge otherEdges[3] = {
                        {badTriangles[k].a, badTriangles[k].b},
                        {badTriangles[k].b, badTriangles[k].c},
                        {badTriangles[k].c, badTriangles[k].a}
                    };
                    if (edges[eIdx] == otherEdges[0] || edges[eIdx] == otherEdges[1] || edges[eIdx] == otherEdges[2]) {
                        shared = true;
                        break;
                    }
                }
                if (!shared) polygon.push_back(edges[eIdx]);
            }
        }

        // Remove bad triangles
        triangles.erase(remove_if(triangles.begin(), triangles.end(), [&](const Triangle& t) {
            for (const auto& bt : badTriangles) {
                if (t.a == bt.a && t.b == bt.b && t.c == bt.c) return true;
            }
            return false;
        }), triangles.end());

        // Re-triangulate the polygonal hole
        for (const auto& edge : polygon) {
            triangles.push_back({edge.v1, edge.v2, i});
        }
    }

    // 3. Clean up Super-Triangle
    triangles.erase(remove_if(triangles.begin(), triangles.end(), [&](const Triangle& t) {
        return t.a >= p1 || t.b >= p1 || t.c >= p1;
    }), triangles.end());
    
    // Remove super-triangle vertices to restore original vector state
    vertices.pop_back(); vertices.pop_back(); vertices.pop_back();

    // 4. Flatten to EBO index array format (pairs of integers)
    vector<int> indices;
    for (const auto& t : triangles) {
        indices.push_back(t.a); indices.push_back(t.b);
        indices.push_back(t.b); indices.push_back(t.c);
        indices.push_back(t.c); indices.push_back(t.a);
    }
    return indices;
}