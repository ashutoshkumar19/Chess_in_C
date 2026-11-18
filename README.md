# Chess Game in C

A two-player graphical chess game written in C. It was originally developed for legacy Turbo C++ using the Borland Graphics Interface (BGI), and now also builds and runs with modern GCC through a bundled Win32 compatibility layer. The project implements a graphical chessboard, custom-drawn chess pieces, mouse-based interactions, and piece movement logic.

## Author

**Ashutosh Kumar**

Developed as an academic project exploring Computer Graphics and Data Structures using C.

## Overview

This project was developed as an exploration of computer graphics and data structures in C. It provides an interactive chess environment where two players can play against each other on the same computer.

The chessboard and pieces are rendered using graphics primitives, while a two-dimensional array maintains the board state and tracks piece positions.

The game code is unchanged from its DOS-era design; `include/` and `src/platform/win32/` supply the legacy `graphics.h`, `conio.h` and `dos.h` APIs so it compiles unmodified with a modern C compiler.

## Features

- **Graphical Chessboard:** An 8×8 checkerboard rendered with graphics primitives.
- **Custom Chess Pieces:** All six chess pieces are drawn programmatically using graphics primitives.
- **Two-Player Gameplay:** Supports alternating turns between two players.
- **Mouse-Based Interaction:** Select and move pieces using mouse clicks.
- **Move Highlighting:** Highlights available movement positions and potential captures.
- **Piece Movement Logic:** Implements movement logic for pawns, rooks, knights, bishops, queens, and kings.
- **Game Controls:** Includes options to restart the game, clear piece selection, view instructions, and exit.
- **Native Windows Build:** Runs as a 640×480 GDI window built with standard GCC — no Turbo C required.

## Technologies & Concepts

| Category | Implementation |
|---|---|
| Programming Language | C |
| Original Graphics | Turbo C Graphics (BGI) |
| Portable Graphics | Win32 GDI compatibility layer in `src/platform/win32/` |
| Data Structures | Two-dimensional arrays |
| Line Drawing | DDA Algorithm |
| Circle Drawing | Bresenham's Circle Algorithm |
| Input Handling | Mouse and keyboard (Int 33h emulation) |
| Graphics Operations | Pixel plotting, line drawing, flood fill, bitmap text |

## Project Structure

```text
Chess_in_C/
├── src/
│   ├── chess.c                  # Game: drawing, rules, UI, main loop
│   └── platform/
│       └── win32/
│           └── bgi_win32.c       # Win32 implementation of the BGI subset
├── include/
│   ├── bgi_win32.h              # Compatibility layer API
│   ├── graphics.h               # BGI graphics API
│   ├── conio.h                  # kbhit/getch/gotoxy/delay
│   └── dos.h                    # REGS and int86 (Int 33h mouse)
├── scripts/
│   └── build.bat                # GCC build producing build/chess.exe
├── docs/
│   └── legacy-notes.txt         # Original Turbo C notes
├── build/                       # Generated binaries (git-ignored)
├── README.md
└── .gitignore
```

## Prerequisites

**Modern build (recommended):**
- GCC (tested with MSYS2 UCRT64 GCC 16 on Windows).
- Windows 10 or later; uses only the system GDI/user32 libraries.
- No third-party graphics libraries and no Turbo C installation.

**Legacy build (optional):**
- Turbo C or Turbo C++ compiler.
- Borland Graphics Interface (BGI) graphics library.
- DOS-compatible environment with mouse support (e.g. DOSBox).

**Note:** The game source uses legacy headers and APIs such as `graphics.h`, `conio.h`, `dos.h`, and `int86()`. These are provided by the compatibility layer described below, which is why modern GCC is now supported.

## Build & Run (Modern GCC on Windows)

`include/` and `src/platform/win32/` provide a small self-contained Win32 compatibility layer: it implements the BGI subset (`graphics.h`), `conio.h` (`kbhit`, `getch`, `gotoxy`, `delay`) and the `dos.h` Int 33h mouse calls used by the game. Rendering goes to a 640×480 GDI window with an indexed framebuffer, scanline floodfill and bitmap text.

Build with MSYS2 UCRT64 GCC:

```bat
scripts\build.bat
```

Or manually:

```bat
gcc -O2 -std=gnu89 -Iinclude -o build\chess.exe src\chess.c src\platform\win32\bgi_win32.c -lgdi32 -luser32 -lm
```

Run:

```bat
build\chess.exe
```

`include/` shadows the missing Turbo C headers, so the game source stays close to the original.

## Legacy Setup (Turbo C++ / DOS)

The original build target is still supported for reference:

1. Open the project in Turbo C or Turbo C++.
2. Load `src\chess.c`.
3. Ensure the BGI graphics driver is available.
4. Configure the graphics initialization path in the source code if your BGI installation uses a different directory.

The default graphics driver path configured in the source is:

```c
initgraph(&gdriver, &gmode, "C:\\TC\\BGI");
```

5. Compile and run the program in a DOS-compatible environment.

## Platform Support

| Platform | Status | Notes |
|---|---|---|
| Windows (MSYS2 UCRT64 GCC) | Supported (recommended) | Uses `src/platform/win32/bgi_win32.c` |
| Turbo C++ / DOSBox | Supported (legacy) | Uses the original BGI driver |
| macOS / Linux | Not implemented | Needs a second backend (e.g. SDL2) implementing the same API in `include/bgi_win32.h`; `src/chess.c` stays unchanged |

## Rendering Details

The compatibility layer keeps an 8-bit indexed framebuffer (one BGI color number per pixel) and blits it to a 640×480 window. This matters because the game inspects pixels at runtime: `getpixel()` is used to detect highlight frames (colors 10 and 12) and square background colors.

| Element | Color |
|---|---|
| Window background | Light grey (7) |
| Light squares | White (15) |
| Dark squares | Dark grey (8) |
| Black pieces | Black body (0) with black outlines |
| White pieces | White body (15) with black outlines |
| Legal-move frame | Light green (10) |
| Capture frame | Light red (12) |

`colorboard()` colors the 8×8 grid directly and black piece outlines are drawn in black rather than dark grey; this keeps piece flood-fills correct on both square colors. The player-turn number is drawn inside the window at the text cursor set by `gotoxy(78, 11)`, and its cell is cleared before redraw so `1` and `2` never overlap.

## Known Limitations

- No castling, en passant, or pawn promotion.
- No check/checkmate detection: the game ends when a king is captured.
- Turn order is inferred from piece parity (black pieces move on odd turns, white on even turns).
- Board state is a fixed `int A[10][10]`; there is no move history or undo.
- Legacy structure: movement code relies on `goto` labels and duplicated per-piece blocks.

## How to Play

| Action | Control |
|---|---|
| Select a chess piece | Right-click on the piece |
| Move a selected piece | Left-click on a highlighted position |
| Clear selection | Click the Clear Selection button |
| View instructions | Click the Instructions button |
| Restart the game | Click the Restart button |
| Exit | Click the Exit button |

Two players take turns moving their respective pieces on the same chessboard. White moves first; turns are tracked in the button panel next to the board.

## Learning Outcomes

This project demonstrates practical applications of:

- C programming and procedural programming.
- Two-dimensional arrays for representing board states.
- Fundamental computer graphics algorithms.
- Drawing and rendering objects using graphics primitives.
- Mouse event handling and interactive user interfaces.
- Implementing movement and capture logic for different chess pieces.
