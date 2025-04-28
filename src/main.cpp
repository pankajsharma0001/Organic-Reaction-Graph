#include <raylib.h>
#include <iostream>
#include <cmath>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// Constants
#define MAX_COMPOUNDS 100
#define MAX_NAME_LEN 50    // Increased size for longer compound names.
#define MAX_REACTIONS 100

// -------------------------------
// Utility Function: Trim whitespace
// -------------------------------
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t");
    if(start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

// -------------------------------
// Data Structures
// -------------------------------
struct Reaction {
    char reactant[MAX_NAME_LEN];
    char product[MAX_NAME_LEN];
    char reactionType[MAX_NAME_LEN];
};

struct Graph {
    int num_compounds;
    char compounds[MAX_COMPOUNDS][MAX_NAME_LEN];
    int adjacency[MAX_COMPOUNDS][MAX_COMPOUNDS];
    // reactionType[] here is used when adding a new product.
    char reactionType[MAX_COMPOUNDS][MAX_NAME_LEN];
};

// -------------------------------
// Graph Helper Functions
// -------------------------------
int find_compound_index(Graph *graph, const char *compound) {
    for (int i = 0; i < graph->num_compounds; i++) {
        if (strcmp(graph->compounds[i], compound) == 0) {
            return i;
        }
    }
    return -1;
}

int add_compound(Graph *graph, const char *compound) {
    if (find_compound_index(graph, compound) == -1) {
        strcpy(graph->compounds[graph->num_compounds], compound);
        graph->num_compounds++;
        return 1;
    }
    return 0;
}

void add_reactionType(Graph *graph, const char *reactionTypeStr) {
    // Stores the reaction type for a product when it is first added.
    static int i = 0;
    if(i < MAX_COMPOUNDS) {
        strcpy(graph->reactionType[i], reactionTypeStr);
        i++;
    }
}

void add_reaction(Graph *graph, const char *reactant, const char *product) {
    int reactant_index = find_compound_index(graph, reactant);
    int product_index = find_compound_index(graph, product);
    if (reactant_index != -1 && product_index != -1) {
        graph->adjacency[reactant_index][product_index] = 1;
    }
}

// -------------------------------
// BFS and Conversion Path Reconstruction
// -------------------------------
void bfs(Graph *graph, int start, int end, int parent[]) {
    int visited[MAX_COMPOUNDS] = {0};
    int queue[MAX_COMPOUNDS];
    int front = 0, rear = 0;
    
    queue[rear++] = start;
    visited[start] = 1;
    parent[start] = -1;
    
    while (front < rear) {
        int current = queue[front++];
        if (current == end)
            break;
        for (int i = 0; i < graph->num_compounds; i++) {
            if (graph->adjacency[current][i] && !visited[i]) {
                queue[rear++] = i;
                visited[i] = 1;
                parent[i] = current;
            }
        }
    }
}

// Returns the conversion path as a multi-line string.
string getConversionPath(Graph *graph, int parent[], int start, int end, Reaction reactions[], int num_reactions, vector<int> &pathIndices) {
    pathIndices.clear();
    int current = end;
    while(current != -1) {
        pathIndices.push_back(current);
        current = parent[current];
    }
    reverse(pathIndices.begin(), pathIndices.end());
    
    string pathStr = "";
    // For each consecutive pair, lookup the reaction.
    for (size_t i = 0; i < pathIndices.size() - 1; i++) {
        int fromIndex = pathIndices[i];
        int toIndex = pathIndices[i+1];
        string fromCompound = graph->compounds[fromIndex];
        string toCompound = graph->compounds[toIndex];
        string reactionFound = "";
        for (int j = 0; j < num_reactions; j++) {
            if (strcmp(reactions[j].reactant, fromCompound.c_str()) == 0 &&
                strcmp(reactions[j].product, toCompound.c_str()) == 0) {
                reactionFound = reactions[j].reactionType;
                break;
            }
        }
        if (reactionFound == "")
            reactionFound = "Unknown";
        pathStr += fromCompound + " -> " + reactionFound + " -> " + toCompound + "\n";
    }
    return pathStr;
}

// -------------------------------
// GUI Helper Functions
// -------------------------------
bool IsMouseClickedInRect(Rectangle rect) {
    Vector2 mousePos = GetMousePosition();
    return CheckCollisionPointRec(mousePos, rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void DrawRoundedRectWithBorder(Rectangle rect, float roundness, Color fillColor, Color borderColor, int borderWidth) {
    // Draw shadow
    Rectangle shadow = { rect.x + 4, rect.y + 4, rect.width, rect.height };
    DrawRectangleRounded(shadow, roundness, 8, Fade(BLACK, 0.3f));
    // Draw filled rounded rectangle
    DrawRectangleRounded(rect, roundness, 8, fillColor);
    // Draw border
    DrawRectangleLines(rect.x, rect.y, rect.width, rect.height, borderColor);
}

void DrawBlinkingCursor(Rectangle rect, const string &text, int fontSize, Color cursorColor) {
    int textWidth = MeasureText(text.c_str(), fontSize);
    if (fmod(GetTime(), 1) < 0.5f) {
        DrawLine(rect.x + 10 + textWidth, rect.y + 10, rect.x + 10 + textWidth, rect.y + 10 + fontSize, cursorColor);
    }
}

// -------------------------------
// Draw Conversion Graph (Chain Diagram)
// Only draws the compounds that are part of the conversion path.
// -------------------------------
void DrawConversionGraph(Graph *graph, Reaction reactions[], int num_reactions, const vector<int>& indices, Rectangle rect) {
    int n = indices.size();
    if(n == 0) return;

    // Lay out nodes evenly in a horizontal line.
    float spacing = rect.width / (n + 1);
    float y = rect.y + rect.height / 2;
    vector<Vector2> positions(n);
    for(int i = 0; i < n; i++){
       positions[i].x = rect.x + spacing * (i + 1);
       positions[i].y = y;
    }

    // Draw edges (line between consecutive nodes) with reaction labels.
    for (int i = 0; i < n - 1; i++){
       DrawLineEx(positions[i], positions[i+1], 2.0f, DARKGRAY);
       
       // Determine reaction label between these compounds.
       string reactionLabel = "";
       int fromIndex = indices[i];
       int toIndex = indices[i+1];
       for (int j = 0; j < num_reactions; j++){
          if(strcmp(reactions[j].reactant, graph->compounds[fromIndex]) == 0 &&
             strcmp(reactions[j].product, graph->compounds[toIndex]) == 0){
             reactionLabel = reactions[j].reactionType;
             break;
          }
       }
       // Compute midpoint for label and offset upward.
       Vector2 mid = { (positions[i].x + positions[i+1].x) / 2, (positions[i].y + positions[i+1].y) / 2 - 15 };
       DrawText(reactionLabel.c_str(), mid.x, mid.y, 10, DARKBLUE);
    }

    // Draw nodes (circles) and label them.
    for (int i = 0; i < n; i++){
       DrawCircleV(positions[i], 40, SKYBLUE);
       int textWidth = MeasureText(graph->compounds[indices[i]], 10);
       DrawText(graph->compounds[indices[i]], positions[i].x - textWidth / 2, positions[i].y - 5, 12, DARKBLUE);
    }
}

// -------------------------------
// Main Function with Raylib GUI
// -------------------------------
int main() {
    // Increase screen height to accommodate extra graph output.
    const int screenWidth = 1000;
    const int screenHeight = 950;
    InitWindow(screenWidth, screenHeight, "Reaction Conversion Path Finder");
    SetTargetFPS(60);

    // Initialize graph and clear adjacency matrix.
    Graph graph = {0};
    for (int i = 0; i < MAX_COMPOUNDS; i++) {
        for (int j = 0; j < MAX_COMPOUNDS; j++) {
            graph.adjacency[i][j] = 0;
        }
    }

    Reaction reactions[MAX_REACTIONS];
    int num_reactions = 0;

    // Load reactions from file "reaction.txt"
    ifstream file("reactions.txt");
    if (!file) {
        cout << "Unable to open file reaction.txt, using default reactions.\n";
        // Default reactions.
        strcpy(reactions[0].reactant, "CH4");
        strcpy(reactions[0].reactionType, "Oxidation");
        strcpy(reactions[0].product, "CH3OH");
        strcpy(reactions[1].reactant, "CH3OH");
        strcpy(reactions[1].reactionType, "Oxidation");
        strcpy(reactions[1].product, "HCHO");
        strcpy(reactions[2].reactant, "HCHO");
        strcpy(reactions[2].reactionType, "Oxidation");
        strcpy(reactions[2].product, "HCOOH");
        strcpy(reactions[3].reactant, "HCOOH");
        strcpy(reactions[3].reactionType, "Oxidation");
        strcpy(reactions[3].product, "CO2");
        num_reactions = 4;
    } else {
        string line;
        while(getline(file, line) && num_reactions < MAX_REACTIONS) {
            if(line.empty()) continue;
            size_t pos1 = line.find("->");
            if(pos1 == string::npos) continue;
            string reactantStr = trim(line.substr(0, pos1));
            size_t pos2 = line.find("->", pos1 + 2);
            if(pos2 == string::npos) continue;
            string reactionTypeStr = trim(line.substr(pos1 + 2, pos2 - (pos1 + 2)));
            string productStr = trim(line.substr(pos2 + 2));
            strcpy(reactions[num_reactions].reactant, reactantStr.c_str());
            strcpy(reactions[num_reactions].reactionType, reactionTypeStr.c_str());
            strcpy(reactions[num_reactions].product, productStr.c_str());
            num_reactions++;

            // Add compounds to the graph.
            int added = add_compound(&graph, productStr.c_str());
            if (added) {
                add_reactionType(&graph, reactionTypeStr.c_str());
            }
            add_compound(&graph, reactantStr.c_str());
        }
        file.close();
    }

    // Build the adjacency matrix.
    for (int i = 0; i < num_reactions; i++) {
        add_reaction(&graph, reactions[i].reactant, reactions[i].product);
    }

    // --- GUI State Variables ---
    string startInput = "CH4";
    string endInput = "CO2";
    bool input1Active = false;
    bool input2Active = false;
    string outputText = ""; // Will contain the conversion path.
    // This vector will store the indices of compounds in the conversion path.
    vector<int> conversionPathIndices;

    // Layout rectangles.
    int titleFontSize = 48;
    const char *titleText = "Reaction Conversion Path Finder";
    int titleWidth = MeasureText(titleText, titleFontSize);
    Rectangle titleRect = { (float)(screenWidth - titleWidth) / 2, 20, (float)titleWidth, 60 };

    Rectangle inputRect1 = { screenWidth * 0.25f - 100, 100, 200, 50 };
    Rectangle inputRect2 = { screenWidth * 0.75f - 100, 100, 200, 50 };
    // Button to trigger BFS search.
    Rectangle buttonRect = { (screenWidth - 250) / 2.0f, 180, 250, 50 };
    // Output area for conversion path.
    Rectangle outputRect = { 50, 260, screenWidth - 100, 350 };
    // Area for drawing the conversion graph (chain diagram).
    Rectangle graphRect = { 50, 630, screenWidth - 100, 250 };

    // Colors and styles.
    Color bgColor = LIGHTGRAY;
    Color titleColor = DARKBLUE;
    Color inputColor = RAYWHITE;
    Color inputActiveColor = Fade(RAYWHITE, 1.0f);
    Color borderColor = GRAY;
    Color buttonColor = SKYBLUE;
    Color buttonHoverColor = BLUE;
    Color textColor = DARKGRAY;
    int inputFontSize = 20;

    // Main loop.
    while (!WindowShouldClose()) {
        // --- Update Input Fields ---
        if (IsMouseClickedInRect(inputRect1)) {
            input1Active = true;
            input2Active = false;
        } else if (IsMouseClickedInRect(inputRect2)) {
            input2Active = true;
            input1Active = false;
        }

        // When the button is clicked, perform BFS and display the conversion path.
        if (IsMouseClickedInRect(buttonRect)) {
            if (startInput.empty() || endInput.empty()) {
                outputText = "Please enter both start and end compounds.";
                conversionPathIndices.clear();
            } else {
                int startIndex = find_compound_index(&graph, startInput.c_str());
                int endIndex = find_compound_index(&graph, endInput.c_str());
                if (startIndex == -1 || endIndex == -1) {
                    outputText = "Start or end compound not found in the graph.";
                    conversionPathIndices.clear();
                } else {
                    int parent[MAX_COMPOUNDS];
                    for (int i = 0; i < MAX_COMPOUNDS; i++) {
                        parent[i] = -1;
                    }
                    bfs(&graph, startIndex, endIndex, parent);
                    if (startIndex != endIndex && parent[endIndex] == -1) {
                        outputText = "No conversion path found.";
                        conversionPathIndices.clear();
                    } else {
                        outputText = getConversionPath(&graph, parent, startIndex, endIndex, reactions, num_reactions, conversionPathIndices);
                    }
                }
            }
        }

        // --- Handle Keyboard Input for Active Text Fields ---
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125) {
                if (input1Active)
                    startInput.push_back((char)key);
                else if (input2Active)
                    endInput.push_back((char)key);
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (input1Active && !startInput.empty())
                startInput.pop_back();
            else if (input2Active && !endInput.empty())
                endInput.pop_back();
        }

        // --- Draw ---
        BeginDrawing();
        ClearBackground(bgColor);

        // Draw centered title.
        DrawText(titleText, titleRect.x, titleRect.y, titleFontSize, titleColor);

        // Draw input boxes.
        Color curInputColor1 = input1Active ? inputActiveColor : inputColor;
        Color curInputColor2 = input2Active ? inputActiveColor : inputColor;
        DrawRoundedRectWithBorder(inputRect1, 0.3f, curInputColor1, borderColor, 2);
        DrawRoundedRectWithBorder(inputRect2, 0.3f, curInputColor2, borderColor, 2);
        DrawText("Start:", inputRect1.x + 10, inputRect1.y - 25, inputFontSize, textColor);
        DrawText("End:", inputRect2.x + 10, inputRect2.y - 25, inputFontSize, textColor);
        DrawText(startInput.c_str(), inputRect1.x + 10, inputRect1.y + 15, inputFontSize, textColor);
        DrawText(endInput.c_str(), inputRect2.x + 10, inputRect2.y + 15, inputFontSize, textColor);

        if (input1Active) DrawBlinkingCursor(inputRect1, startInput, inputFontSize, textColor);
        if (input2Active) DrawBlinkingCursor(inputRect2, endInput, inputFontSize, textColor);

        // Draw the "Find Conversion Path" button with hover effect.
        Vector2 mousePos = GetMousePosition();
        Color curButtonColor = CheckCollisionPointRec(mousePos, buttonRect) ? buttonHoverColor : buttonColor;
        DrawRoundedRectWithBorder(buttonRect, 0.3f, curButtonColor, borderColor, 2);
        int btnTextWidth = MeasureText("Find Conversion Path", inputFontSize);
        DrawText("Find Conversion Path", buttonRect.x + (buttonRect.width - btnTextWidth) / 2,
                 buttonRect.y + (buttonRect.height - inputFontSize) / 2, inputFontSize, RAYWHITE);

        // Draw the output area background and border.
        DrawRectangleRec(outputRect, Fade(RAYWHITE, 0.9f));
        DrawRectangleLines(outputRect.x, outputRect.y, outputRect.width, outputRect.height, borderColor);

        // Display the conversion path text.
        istringstream iss(outputText);
        string line;
        int lineHeight = 25;
        int yPos = outputRect.y + 10;
        int xPos = outputRect.x + 10;
        
        DrawText("Conversion path", xPos, yPos , 20, textColor);


        while(getline(iss, line)) {
            DrawText(line.c_str(), xPos, yPos + 20, 20, textColor);
            yPos += lineHeight;
            if (yPos > outputRect.y + outputRect.height - 20)
                break;
        }

        // Draw the conversion graph area.
        DrawRectangleRec(graphRect, Fade(RAYWHITE, 0.9f));
        DrawRectangleLines(graphRect.x, graphRect.y, graphRect.width, graphRect.height, borderColor);
        DrawText("Conversion Graph", graphRect.x + 10, graphRect.y + 10, 20, textColor);
        // Only draw the conversion graph if a valid conversion path was found.
        if (!conversionPathIndices.empty()) {
            DrawConversionGraph(&graph, reactions, num_reactions, conversionPathIndices,
                                  (Rectangle){ graphRect.x, graphRect.y + 40, graphRect.width, graphRect.height - 40 });
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
