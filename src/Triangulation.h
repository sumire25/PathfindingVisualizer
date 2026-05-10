#pragma once
#include "Geometry.h"

// Generates points in U(-0.9, 0.9) to leave a margin
vector<Vertex> generateRandomPoints(int count);

// Performs Bowyer-Watson Delaunay Triangulation
// Returns a flat list of edges suitable for the OpenGL EBO
vector<int> bowyerWatson(vector<Vertex>& vertices);