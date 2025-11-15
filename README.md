# Tetris (Manual & AI)

This repository contains a small, single-file Tetris implementation using raylib with two playable modes:

- Manual: player-controlled Tetris (controls similar to classic Tetris).
- AI: automatic placement using a heuristic evaluator (single-ply greedy placement).

The merged file `tetris_merged.cpp` provides a menu to choose between modes and retains much of the functionality from the original `tetris.cpp` (AI) and `exp.cpp` (manual).

---

## Features

- 10x20 playfield using a 7-bag tetromino randomizer.
- Rotation, collision detection, line clear, scoring, and level speed-up.
- Manual controls: move, rotate, soft/hard drop, pause.
- AI mode: picks the best placement for the current piece using a heuristic (lines, holes, height, bumpiness).
- Simple UI with sidebar showing next piece and stats.

---

## Important files

- `tetris_merged.cpp`  — Merged game with Menu, Manual and AI modes (main file).
- `tetris.cpp`         — Original AI/heuristic implementation.
- `exp.cpp`            — Original manual, user-controlled implementation.
- `raylib/`            — Raylib source / build directory included in the workspace (if present).

---

## Build Instructions

You need `raylib` and a C++ compiler (g++ recommended).

Linux example (Ubuntu / other distributions):

```bash
# Install dependencies (example for Debian/Ubuntu):
sudo apt update
sudo apt install build-essential libraylib-dev libgl1-mesa-dev libx11-dev libasound2-dev libpulse-dev -y

# Compile
g++ -std=c++17 tetris_merged.cpp -o tetris_merged -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Run
./tetris_merged
```

Windows (MSYS2) example:

```bash
# In MSYS2 / MinGW shell
g++ -std=c++17 tetris_merged.cpp -o tetris_merged.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./tetris_merged.exe
```

If your system does not provide `libraylib` via package manager, install raylib from source or use the recommended build instructions on raylib's site: https://www.raylib.com/

---

## Running

- Start the program, then use the menu to choose `Start Game` -> select `Manual` or `AI` mode.
- In Manual mode, use the controls below.

---

## Controls (Manual Mode)

- Left / Right arrows: move piece
- Down arrow: soft drop
- Up arrow or X: rotate clockwise
- Z: rotate counter-clockwise
- Space: hard drop
- P: pause
- ESC: return to menu / exit

---

## AI Mode (current implementation)

- The AI in this repository is a single-ply greedy heuristic:
  - For every rotation and every horizontal placement the current piece can legally occupy, the AI "drops" the piece and evaluates the resulting board with a linear heuristic.
  - The heuristic uses weighted features: number of lines cleared, number of holes, aggregate column height, and bumpiness (column height differences).
  - The placement that yields the highest heuristic score is chosen and placed immediately.

- Note: The AI does NOT perform multi-piece DFS/lookahead in the current `tetris_merged.cpp`. You can extend the AI to depth-2 or use beam search for improved play.

- Recent fix: The `Game::collidesPiece` function was adjusted so the spawn area (negative Y positions) is treated the same as `Board::collides`, allowing the AI's drop-height search to compute valid landing rows. This resolves a bug where AI placements weren't appearing on the board.

---

## Potential Enhancements

If you want stronger AI behavior consider:

- Implementing depth-2 or more lookahead (DFS) with a copy of the bag/queue.
- Beam search: keep top-K board states at each depth (reduces branching).
- Reinforcement learning or Monte Carlo Tree Search (advanced).

I can implement a depth-2 lookahead with beam pruning if you'd like — tell me preferred defaults (depth=2, beam=8 is a reasonable starting point).

---

## Troubleshooting

- If the window does not open, ensure that `raylib` is installed and the linker flags are correct for your platform.
- Missing symbols during link: verify you have the necessary development packages (GL, X11, pthread, etc.).

---

## Contributing

Contributions and improvements are welcome. Suggested small steps:
- Add configurable AI depth and beam size (menu or compile-time macro).
- Add hold-piece functionality and improved spawning rules.
- Tweak heuristic weights or allow runtime tuning.

Please open a PR or ask me to implement a specific improvement.

---

## License & Credits

This repository is a small learning/demo project. No explicit license file included — add one if you plan to publish publicly.

Original code pieces were merged from local files `tetris.cpp` and `exp.cpp` (user-provided).


---

If you'd like, I can also:
- Add a README section describing how to tweak AI parameters in the code,
- Implement a depth-2 lookahead AI and update the README with results.

Tell me which you'd prefer next.
