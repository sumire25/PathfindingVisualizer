#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <cmath>
#include <vector>

#include "Geometry.h"
#include "Triangulation.h"
#include "Graph.h"
#include "Pathfinding.h"

const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in float aState;

out float vState;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    gl_PointSize = 10.0;
    vState = aState;
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in float vState;
out vec4 FragColor;
uniform int uRenderMode; // 0 = Points, 1 = Lines, 2 = Path Line

void main() {
    if (uRenderMode == 1) {
        FragColor = vec4(0.3f, 0.3f, 0.4f, 0.5f); // Base Edge color
    } else if (uRenderMode == 2) {
        FragColor = vec4(1.0f, 0.8f, 0.0f, 1.0f); // Path Line: Gold
    } else {
        int state = int(vState);
        if (state == 0)      FragColor = vec4(0.6f, 0.6f, 0.6f, 1.0f); // Unvisited: Gray
        else if (state == 1) FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); // Visited: White
        else if (state == 2) FragColor = vec4(0.5f, 0.8f, 1.0f, 1.0f); // Reached: Light Blue
        else if (state == 3) FragColor = vec4(0.0f, 0.4f, 0.0f, 1.0f); // Frontier: Dark Green
        else if (state == 4) FragColor = vec4(0.0f, 0.8f, 0.0f, 1.0f); // Expanding: Green
        else if (state == 5) FragColor = vec4(1.0f, 0.8f, 0.0f, 1.0f); // Path: Gold
        else if (state == 6) FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); // Start Node: Red
        else if (state == 7) FragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f); // End Node: Magenta
    }
}
)glsl";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1080, 1080, "Pathfinding Profiler", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    GLuint shaderProgram = glCreateProgram();
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    int inputPointCount = 100;
    std::vector<Vertex> points = generateRandomPoints(inputPointCount);
    std::vector<int> edges;
    std::vector<int> pathIndices; // Stores the sequence of vertices for the final path line
    Graph graph;

    GLuint VAO, VBO, EBO, pathEBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &pathEBO); // New EBO specifically for the path line

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vertex), points.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glBindVertexArray(0);

    GLint renderModeLoc = glGetUniformLocation(shaderProgram, "uRenderMode");

    int startNode = -1;
    int endNode = -1;
    bool mouseLeftPressed = false;
    bool mouseRightPressed = false;
    
    int currentAlgo = 0;
    const char* algos[] = { 
        "Dijkstra", "BFS", "DFS", "A* (Euclidean)", 
        "Greedy Best-First", "Hill Climbing", "IDA*" 
    };

    PathfindingHistory activeResult;
    bool isPlaying = false;
    int playbackStep = 0;
    int lastExpandedNode = -1;
    float stepDelay = 0.05f;
    float playbackTimer = 0.0f;
    
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        // --- 1. MOUSE PICKING ---
        if (!io.WantCaptureMouse && !isPlaying) {
            bool currentLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool currentRight = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

            if ((currentLeft && !mouseLeftPressed) || (currentRight && !mouseRightPressed)) {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                int width, height;
                glfwGetWindowSize(window, &width, &height);

                float ndcX = (xpos / (float)width) * 2.0f - 1.0f;
                float ndcY = 1.0f - (ypos / (float)height) * 2.0f;

                int closestIndex = -1;
                float minSquareDist = 0.05f * 0.05f;

                for (size_t i = 0; i < points.size(); ++i) {
                    float dx = points[i].x - ndcX;
                    float dy = points[i].y - ndcY;
                    float sqDist = dx * dx + dy * dy;
                    if (sqDist < minSquareDist) {
                        minSquareDist = sqDist;
                        closestIndex = i;
                    }
                }

                if (closestIndex != -1) {
                    if (currentLeft) {
                        if (startNode != -1 && startNode != endNode) points[startNode].state = 0.0f;
                        if (closestIndex == endNode) endNode = -1;
                        startNode = closestIndex;
                        points[startNode].state = 6.0f;
                    } else if (currentRight) {
                        if (endNode != -1 && endNode != startNode) points[endNode].state = 0.0f;
                        if (closestIndex == startNode) startNode = -1;
                        endNode = closestIndex;
                        points[endNode].state = 7.0f;
                    }
                    pathIndices.clear(); // Clear path overlay if endpoints change
                    glBindBuffer(GL_ARRAY_BUFFER, VBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(Vertex), points.data());
                }
            }
            mouseLeftPressed = currentLeft;
            mouseRightPressed = currentRight;
        }

        // --- 2. ASYNCHRONOUS PLAYBACK LOOP ---
        if (isPlaying && playbackStep < activeResult.steps.size()) {
            playbackTimer += deltaTime;
            if (playbackTimer >= stepDelay) {
                playbackTimer = 0.0f;

                if (lastExpandedNode != -1 && lastExpandedNode != startNode && lastExpandedNode != endNode) {
                    points[lastExpandedNode].state = static_cast<float>(NodeState::REACHED);
                }

                const auto& step = activeResult.steps[playbackStep];
                for (const auto& delta : step.deltas) {
                    if (delta.nodeIndex != startNode && delta.nodeIndex != endNode) {
                        points[delta.nodeIndex].state = static_cast<float>(delta.newState);
                    }
                }

                lastExpandedNode = step.expandedNode;
                playbackStep++;

                // Handle completion
                if (playbackStep == activeResult.steps.size()) {
                    isPlaying = false;
                    if (activeResult.pathFound) {
                        // 1. Color the nodes
                        for (int n : activeResult.finalPath) {
                            if (n != startNode && n != endNode) {
                                points[n].state = static_cast<float>(NodeState::PATH);
                            }
                        }
                        
                        // 2. Upload the edge sequence to the secondary EBO for line rendering
                        pathIndices = activeResult.finalPath;
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pathEBO);
                        glBufferData(GL_ELEMENT_ARRAY_BUFFER, pathIndices.size() * sizeof(int), pathIndices.data(), GL_DYNAMIC_DRAW);
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Restore main EBO binding safely
                    }
                }
                
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(Vertex), points.data());
            }
        }

        // --- 3. IMGUI WINDOW ---
        ImGui::Begin("Graph Controls");
        
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Left Click: Start Node");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Right Click: End Node");
        ImGui::Separator();

        ImGui::InputInt("Node Count", &inputPointCount);
        if (ImGui::Button("Generate Points")) {
            points = generateRandomPoints(inputPointCount);
            edges.clear();
            pathIndices.clear();
            startNode = -1; endNode = -1;
            isPlaying = false;
            
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vertex), points.data(), GL_DYNAMIC_DRAW);
        }

        if (ImGui::Button("Generate Triangulation")) {
            for(auto& p : points) p.state = 0.0f;
            edges = bowyerWatson(points);
            pathIndices.clear();
            
            if (startNode != -1) points[startNode].state = 6.0f;
            if (endNode != -1) points[endNode].state = 7.0f;

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(int), edges.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(Vertex), points.data());
        }

        ImGui::Separator();
        ImGui::Combo("Algorithm", &currentAlgo, algos, IM_ARRAYSIZE(algos));
        ImGui::SliderFloat("Speed (s)", &stepDelay, 0.01f, 0.5f);
        
        bool disableRunButton = (startNode == -1 || endNode == -1 || edges.empty() || isPlaying);
        if (disableRunButton) ImGui::BeginDisabled();
        
        if (ImGui::Button("Run Algorithm")) {
            for(size_t i = 0; i < points.size(); ++i) points[i].state = 0.0f;
            points[startNode].state = 6.0f; 
            points[endNode].state = 7.0f;
            pathIndices.clear(); // Clear old path
            
            graph.buildFromTopology(points, edges, points.size());

            std::vector<float> h(points.size(), 0.0f);
            if (currentAlgo >= 3) {
                float targetX = points[endNode].x;
                float targetY = points[endNode].y;
                for (size_t i = 0; i < points.size(); ++i) {
                    float dx = points[i].x - targetX;
                    float dy = points[i].y - targetY;
                    h[i] = std::sqrt(dx * dx + dy * dy);
                }
            }

            switch (currentAlgo) {
                case 0: activeResult = runDijkstra(graph, startNode, endNode); break;
                case 1: activeResult = runBFS(graph, startNode, endNode); break;
                case 2: activeResult = runDFS(graph, startNode, endNode); break;
                case 3: activeResult = runAStar(graph, startNode, endNode, h); break;
                case 4: activeResult = runGreedy(graph, startNode, endNode, h); break;
                case 5: activeResult = runHillClimbing(graph, startNode, endNode, h); break;
                case 6: activeResult = runIDAStar(graph, startNode, endNode, h); break;
            }
            
            isPlaying = true; 
            playbackStep = 0;
            playbackTimer = 0.0f;
            lastExpandedNode = -1;

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(Vertex), points.data());
        }
        
        if (disableRunButton) ImGui::EndDisabled();

        if (!isPlaying && activeResult.executionTimeMicroseconds > 0) {
            ImGui::Separator();
            if (activeResult.pathFound) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Execution Complete");
                ImGui::Text("CPU Time: %lld µs", activeResult.executionTimeMicroseconds);
                ImGui::Text("Total Cost: %.2f", activeResult.totalCost);
                ImGui::Text("Path Length: %zu nodes", activeResult.finalPath.size());
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No Path Found!");
            }
        }

        ImGui::End();

        // --- 4. RENDER CALLS ---
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // A. Draw Full Graph Topology
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Ensure base topology EBO is bound
        if (!edges.empty()) {
            glUniform1i(renderModeLoc, 1);
            glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
        }

        // B. Draw Found Path Line Overlay (If exists)
        if (!isPlaying && !pathIndices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pathEBO); // Switch to specific Path EBO
            glUniform1i(renderModeLoc, 2);
            glLineWidth(4.0f); // Make path line thicker (supported by most drivers)
            glDrawElements(GL_LINE_STRIP, pathIndices.size(), GL_UNSIGNED_INT, 0);
            glLineWidth(1.0f);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Restore binding
        }

        // C. Draw Nodes
        glUniform1i(renderModeLoc, 0);
        glDrawArrays(GL_POINTS, 0, points.size());

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &pathEBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}