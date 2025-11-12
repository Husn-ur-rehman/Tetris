// tetris_raylib.cpp
// Single-file Tetris-like game using raylib (C++).
// Features: 10x20 board, 7-bag randomizer, rotation, collision, line clear, next-piece preview, scoring, level speed.
// Controls:
//  - Left / Right arrows: move piece
//  - Down arrow: soft drop
//  - Up arrow or X: rotate clockwise
//  - Z: rotate counter-clockwise
//  - Space: hard drop
//  - P: pause
// Build (Linux):
//   g++ tetris_raylib.cpp -o tetris -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
// Build (Windows - MSYS2):
//   g++ tetris_raylib.cpp -o tetris.exe -lraylib -lopengl32 -lgdi32 -lwinmm

#include <raylib.h>
#include <vector>
#include <array>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>

using namespace std;

// Board dimensions
const int BOARD_W = 10;
const int BOARD_H = 20;
const int CELL = 24;
const int WINDOW_W = 640;
const int WINDOW_H = 720;

// Tetromino definitions (4x4 matrices encoded as 16 chars)
// We'll use 7 standard tetrominoes: I O T J L S Z
static const array<string, 7> TETROMINO_RAW = {
    // I
    string("..X...X...X...X."),
    // O
    string(".... .XX. .XX. ...."), // we'll normalize eliminate spaces
    // T
    string(".... .XXX ..X. ...."),
    // J
    string("..X. ..X. .XX. ...."),
    // L
    string("...X ...X .XX. ...."),
    // S
    string(".... .XX. .XX. ...."),
    // Z
    string(".... .XX. .XX. ....")
};

// Instead of the above mixed strings we'll define clear 4x4 manually per shape (0/1)
static const array<array<int,16>,7> TETROMINO = {
    // I
    array<int,16>{0,0,0,0,
                  1,1,1,1,
                  0,0,0,0,
                  0,0,0,0},
    // O
    array<int,16>{0,1,1,0,
                  0,1,1,0,
                  0,0,0,0,
                  0,0,0,0},
    // T
    array<int,16>{0,1,0,0,
                  1,1,1,0,
                  0,0,0,0,
                  0,0,0,0},
    // J
    array<int,16>{1,0,0,0,
                  1,1,1,0,
                  0,0,0,0,
                  0,0,0,0},
    // L
    array<int,16>{0,0,1,0,
                  1,1,1,0,
                  0,0,0,0,
                  0,0,0,0},
    // S
    array<int,16>{0,1,1,0,
                  1,1,0,0,
                  0,0,0,0,
                  0,0,0,0},
    // Z
    array<int,16>{1,1,0,0,
                  0,1,1,0,
                  0,0,0,0,
                  0,0,0,0}
};

static const array<Color,8> PALETTE = {
    BLACK, SKYBLUE, YELLOW, MAGENTA, BLUE, ORANGE, GREEN, RED
};

struct Piece {
    int type; // 0..6
    int x, y; // position refers to top-left of 4x4 area relative to board
    int rotation; // 0..3
};

// Board: 0 empty, 1..7 filled with tetromino id +1
struct Game {
    array<array<int, BOARD_W>, BOARD_H> board{};
    Piece cur;
    vector<int> bag; // upcoming queue
    int score = 0;
    int lines = 0;
    int level = 1;
    bool gameOver = false;
    bool paused = false;
    std::mt19937 rng;
    Game(){
        rng.seed((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
        refillBag();
        spawnPiece();
    }

    void refillBag(){
        bag.clear();
        for(int i=0;i<7;i++) bag.push_back(i);
        shuffle(bag.begin(), bag.end(), rng);
    }

    int nextFromBag(){
        if(bag.empty()) refillBag();
        int v = bag.back(); bag.pop_back();
        return v;
    }

    void spawnPiece(){
        cur.type = nextFromBag();
        cur.rotation = 0;
        cur.x = (BOARD_W/2) - 2;
        cur.y = 0;
        if(collides(cur.x, cur.y, cur.type, cur.rotation)){
            gameOver = true;
        }
    }

    // returns value at 4x4 for piece type, rotation, i,j
    int pieceCell(int type, int rotation, int i, int j) const {
        // i row 0..3, j col 0..3
        int idx = 0;
        switch(rotation % 4){
            case 0: idx = i*4 + j; break;
            case 1: idx = (3 - j)*4 + i; break; // rotate 90
            case 2: idx = (3 - i)*4 + (3 - j); break; // 180
            case 3: idx = j*4 + (3 - i); break; // 270
        }
        return TETROMINO[type][idx];
    }

    bool collides(int px, int py, int type, int rotation) const {
        for(int i=0;i<4;i++){
            for(int j=0;j<4;j++){
                if(pieceCell(type, rotation, i, j)){
                    int bx = px + j;
                    int by = py + i;
                    if(bx < 0 || bx >= BOARD_W || by < 0 || by >= BOARD_H) return true;
                    if(board[by][bx] != 0) return true;
                }
            }
        }
        return false;
    }

    void lockPiece(){
        for(int i=0;i<4;i++){
            for(int j=0;j<4;j++){
                if(pieceCell(cur.type, cur.rotation, i, j)){
                    int bx = cur.x + j;
                    int by = cur.y + i;
                    if(by >= 0 && by < BOARD_H && bx >=0 && bx < BOARD_W)
                        board[by][bx] = cur.type + 1;
                }
            }
        }
        clearLines();
        spawnPiece();
    }

    void clearLines(){
        int cleared = 0;
        for(int r = BOARD_H - 1; r >= 0; r--){
            bool full = true;
            for(int c=0;c<BOARD_W;c++) if(board[r][c] == 0){ full = false; break; }
            if(full){
                cleared++;
                // move everything above down
                for(int rr = r; rr > 0; rr--){
                    board[rr] = board[rr-1];
                }
                board[0].fill(0);
                r++; // recheck same row index since rows shifted
            }
        }
        if(cleared > 0){
            lines += cleared;
            // scoring (classic-ish)
            static const int pointsPer[5] = {0,40,100,300,1200};
            score += pointsPer[cleared] * level;
            level = 1 + lines / 10;
        }
    }

    void hardDrop(){
        while(!collides(cur.x, cur.y+1, cur.type, cur.rotation)) cur.y++;
        lockPiece();
    }
};

int main(){
    InitWindow(WINDOW_W, WINDOW_H, "Tetris - raylib");
    SetTargetFPS(60);

    Game game;

    float gravityTimer = 0.0f;
    float gravityDelay = 0.8f; // seconds between automatic drop (will scale with level)
    float inputDelay = 0.08f; // horizontal key repeat
    float inputTimer = 0.0f;

    bool leftHeld = false, rightHeld = false, downHeld = false;

    while(!WindowShouldClose()){
        // Update
        if(IsKeyPressed(KEY_P)) game.paused = !game.paused;
        if(game.gameOver){
            if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)){
                game = Game(); // restart
                gravityTimer = 0; inputTimer = 0;
            }
        }

        if(!game.gameOver && !game.paused){
            float dt = GetFrameTime();
            gravityDelay = std::max(0.05f, 0.8f - (game.level-1)*0.05f);
            gravityTimer += dt;
            inputTimer += dt;

            // Input: left/right movement with repeat
            if(IsKeyDown(KEY_LEFT)){
                if(!leftHeld || inputTimer >= inputDelay){
                    if(!game.collides(game.cur.x - 1, game.cur.y, game.cur.type, game.cur.rotation)){
                        game.cur.x -= 1;
                    }
                    leftHeld = true; inputTimer = 0;
                }
            } else leftHeld = false;

            if(IsKeyDown(KEY_RIGHT)){
                if(!rightHeld || inputTimer >= inputDelay){
                    if(!game.collides(game.cur.x + 1, game.cur.y, game.cur.type, game.cur.rotation)){
                        game.cur.x += 1;
                    }
                    rightHeld = true; inputTimer = 0;
                }
            } else rightHeld = false;

            // Soft drop
            if(IsKeyDown(KEY_DOWN)){
                if(!downHeld || inputTimer >= inputDelay){
                    if(!game.collides(game.cur.x, game.cur.y+1, game.cur.type, game.cur.rotation)){
                        game.cur.y += 1;
                    }
                    downHeld = true; inputTimer = 0;
                }
            } else downHeld = false;

            // Rotation
            if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_X)){
                int newRot = (game.cur.rotation + 1) % 4;
                if(!game.collides(game.cur.x, game.cur.y, game.cur.type, newRot)){
                    game.cur.rotation = newRot;
                } else {
                    // simple wall kicks: try left/right
                    if(!game.collides(game.cur.x-1, game.cur.y, game.cur.type, newRot)) game.cur.x-- , game.cur.rotation = newRot;
                    else if(!game.collides(game.cur.x+1, game.cur.y, game.cur.type, newRot)) game.cur.x++ , game.cur.rotation = newRot;
                }
            }
            if(IsKeyPressed(KEY_Z)){
                int newRot = (game.cur.rotation + 3) % 4;
                if(!game.collides(game.cur.x, game.cur.y, game.cur.type, newRot)){
                    game.cur.rotation = newRot;
                } else {
                    if(!game.collides(game.cur.x-1, game.cur.y, game.cur.type, newRot)) game.cur.x-- , game.cur.rotation = newRot;
                    else if(!game.collides(game.cur.x+1, game.cur.y, game.cur.type, newRot)) game.cur.x++ , game.cur.rotation = newRot;
                }
            }

            // Hard drop
            if(IsKeyPressed(KEY_SPACE)){
                game.hardDrop();
                gravityTimer = 0;
            }

            // Gravity tick
            if(gravityTimer >= gravityDelay){
                gravityTimer = 0;
                if(!game.collides(game.cur.x, game.cur.y+1, game.cur.type, game.cur.rotation)){
                    game.cur.y += 1;
                } else {
                    // lock
                    game.lockPiece();
                }
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw board background
        int boardX = 20, boardY = 20;
        DrawRectangle(boardX-4, boardY-4, BOARD_W*CELL+8, BOARD_H*CELL+8, DARKGRAY);
        DrawRectangle(boardX, boardY, BOARD_W*CELL, BOARD_H*CELL, LIGHTGRAY);

        // Draw placed blocks
        for(int r=0;r<BOARD_H;r++){
            for(int c=0;c<BOARD_W;c++){
                int v = game.board[r][c];
                if(v){
                    DrawRectangle(boardX + c*CELL, boardY + r*CELL, CELL-2, CELL-2, PALETTE[v]);
                }
            }
        }

        // Draw current piece
        if(!game.gameOver){
            for(int i=0;i<4;i++){
                for(int j=0;j<4;j++){
                    if(game.pieceCell(game.cur.type, game.cur.rotation, i, j)){
                        int bx = game.cur.x + j;
                        int by = game.cur.y + i;
                        if(by >= 0){
                            DrawRectangle(boardX + bx*CELL, boardY + by*CELL, CELL-2, CELL-2, PALETTE[game.cur.type+1]);
                        }
                    }
                }
            }
        }

        // Draw grid lines
        for(int i=0;i<=BOARD_W;i++) DrawLine(boardX + i*CELL, boardY, boardX + i*CELL, boardY + BOARD_H*CELL, Fade(BLACK,0.12f));
        for(int i=0;i<=BOARD_H;i++) DrawLine(boardX, boardY + i*CELL, boardX + BOARD_W*CELL, boardY + i*CELL, Fade(BLACK,0.12f));

        // Sidebar - next pieces
        int sidebarX = boardX + BOARD_W*CELL + 20;
        int sidebarY = boardY;
        DrawText(TextFormat("Score: %d", game.score), sidebarX, sidebarY, 20, BLACK);
        DrawText(TextFormat("Lines: %d", game.lines), sidebarX, sidebarY + 28, 18, BLACK);
        DrawText(TextFormat("Level: %d", game.level), sidebarX, sidebarY + 52, 18, BLACK);

        DrawText("Next:", sidebarX, sidebarY + 90, 18, BLACK);
        int nx = sidebarX; int ny = sidebarY + 120;
        // Show next 5 (peek into bag + random)
        // We'll simulate by copying bag state but not modifying game.bag permanently
        vector<int> preview;
        // take current bag content reversed plus random refill sample if needed
        // simple approach: reproduce bag vector in order of popping: last element is next
        // we copy and if needed refill with a shuffled bag
        auto tmpBag = game.bag;
        std::mt19937 tmpRng = game.rng;
        while(preview.size() < 5){
            if(tmpBag.empty()){
                array<int,7> seeds = {0,1,2,3,4,5,6};
                std::shuffle(seeds.begin(), seeds.end(), tmpRng);
                for(int k=0;k<7;k++) tmpBag.push_back(seeds[k]);
            }
            preview.push_back(tmpBag.back()); tmpBag.pop_back();
        }
        for(size_t pi=0; pi<preview.size(); pi++){
            int t = preview[pi];
            for(int i=0;i<4;i++){
                for(int j=0;j<4;j++){
                    if(game.pieceCell(t,0,i,j)){
                        DrawRectangle(nx + j*12 + 40, ny + i*12 + pi*60, 10, 10, PALETTE[t+1]);
                    }
                }
            }
        }

        // Instructions
        DrawText("Arrows: Move/Drop", sidebarX, sidebarY + 420, 12, DARKGRAY);
        DrawText("Up/X: Rotate  Z: CCW", sidebarX, sidebarY + 440, 12, DARKGRAY);
        DrawText("Space: Hard Drop", sidebarX, sidebarY + 460, 12, DARKGRAY);
        DrawText("P: Pause  Enter/R: Restart", sidebarX, sidebarY + 480, 12, DARKGRAY);

        if(game.paused){
            DrawRectangle(0, WINDOW_H/2 - 40, WINDOW_W, 80, Fade(BLACK, 0.5f));
            DrawText("Paused", WINDOW_W/2 - MeasureText("Paused", 40)/2, WINDOW_H/2 - 20, 40, WHITE);
        }

        if(game.gameOver){
            DrawRectangle(0, WINDOW_H/2 - 60, WINDOW_W, 120, Fade(BLACK, 0.6f));
            DrawText("Game Over", WINDOW_W/2 - MeasureText("Game Over", 40)/2, WINDOW_H/2 - 36, 40, RED);
            DrawText(TextFormat("Score: %d  Lines: %d", game.score, game.lines), WINDOW_W/2 - MeasureText("Score", 20)/2, WINDOW_H/2 + 6, 20, WHITE);
            DrawText("Enter/R to Restart", WINDOW_W/2 - MeasureText("Enter/R to Restart", 20)/2, WINDOW_H/2 + 36, 20, LIGHTGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
