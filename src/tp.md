# Pathfinding visualizer

## Opengl
- say that i want to draw a graph using opengl. If the verteces are stored on points = vector<float> and the edges = vector<pair<int>> which maps the vertex with coordinates (points[2*pair.first], points[2*pair.first + 1]), with the vertex (points[2*pair.second], points[2*pair.second + 1])
- how would be the shader that let those lines draw?
- only sharing the edges vetor?

## Pathfinding algorithm
- select starts and end point
- generate execution history for:
  - node to expand: light green
  - nodes reached result of expansion: light blue
  - frontier nodes: dark green
  - return path: node and its parents, cost