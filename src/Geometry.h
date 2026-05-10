#pragma once
#include <vector>
#include <cmath>
using namespace std;

// 12 bytes per vertex (interleaved for the VBO)
struct Vertex {
    float x, y;
    float state; // 0=Unvisited, 1=Frontier, 2=Expanded, 3=Path
};

struct Edge {
    int v1, v2;
    bool operator==(const Edge& other) const {
        return (v1 == other.v1 && v2 == other.v2) || 
               (v1 == other.v2 && v2 == other.v1);
    }
};

struct Triangle {
    int a, b, c; // Indices into the vertex array
    
    // Checks if point p is inside the circumcircle of this triangle
    bool circumcircleContains(const vector<Vertex>& vertices, int pIndex) const {
        float ax = vertices[a].x, ay = vertices[a].y;
        float bx = vertices[b].x, by = vertices[b].y;
        float cx = vertices[c].x, cy = vertices[c].y;
        float px = vertices[pIndex].x, py = vertices[pIndex].y;

        float ab = ax * ax + ay * ay;
        float cd = bx * bx + by * by;
        float ef = cx * cx + cy * cy;

        // Circumcenter coordinates (Ux, Uy)
        float D = 2.0f * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
        if (abs(D) < 1e-6f) return false; // Collinear fallback

        float Ux = (ab * (by - cy) + cd * (cy - ay) + ef * (ay - by)) / D;
        float Uy = (ab * (cx - bx) + cd * (ax - cx) + ef * (bx - ax)) / D;

        // Squared radius
        float R2 = (ax - Ux) * (ax - Ux) + (ay - Uy) * (ay - Uy);
        
        // Squared distance from point to circumcenter
        float dist2 = (px - Ux) * (px - Ux) + (py - Uy) * (py - Uy);

        return dist2 <= R2;
    }
};