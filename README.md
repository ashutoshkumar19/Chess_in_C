# Chess Game in C

A two-player graphical chess game developed in C using Turbo C graphics (BGI). The project implements a graphical chessboard, custom-drawn chess pieces, mouse-based interactions, and piece movement logic.

## Overview

This project was developed as an exploration of computer graphics and data structures in C. It provides an interactive chess environment where two players can play against each other on the same computer.

The chessboard and pieces are rendered using graphics primitives, while a two-dimensional array maintains the board state and tracks piece positions.

## Features

- **Graphical Chessboard:** An 8×8 chessboard rendered using Turbo C graphics.
- **Custom Chess Pieces:** All six chess pieces are drawn programmatically using graphics primitives.
- **Two-Player Gameplay:** Supports alternating turns between two players.
- **Mouse-Based Interaction:** Select and move pieces using mouse clicks.
- **Move Highlighting:** Highlights available movement positions and potential captures.
- **Piece Movement Logic:** Implements movement logic for pawns, rooks, knights, bishops, queens, and kings.
- **Game Controls:** Includes options to restart the game, clear piece selection, view instructions, and exit.

## Technologies & Concepts

| Category | Implementation |
|---|---|
| Programming Language | C |
| Graphics Library | Turbo C Graphics (BGI) |
| Data Structures | Two-dimensional arrays |
| Line Drawing | DDA Algorithm |
| Circle Drawing | Bresenham's Circle Algorithm |
| Input Handling | Mouse and keyboard |
| Graphics Operations | Pixel plotting, line drawing, flood fill |

## Project Structure

```text
Chess_in_C/
├── CHESS.C       # Main game implementation
├── README.md     # Project documentation
└── Readme.txt    # Original project notes
```

## Prerequisites

The project was originally developed for the Turbo C / Turbo C++ environment.

Requirements:
- Turbo C or Turbo C++ compiler.
- Borland Graphics Interface (BGI) graphics library.
- DOS-compatible environment with mouse support.

**Note:** The source code uses legacy headers and APIs such as `graphics.h`, `conio.h`, `dos.h`, and `int86()`. It is not directly compatible with modern standard C compilers such as GCC or Clang.

## Setup & Execution

1. Open the project in Turbo C or Turbo C++.
2. Load `CHESS.C`.
3. Ensure the BGI graphics driver is available.
4. Configure the graphics initialization path in the source code if your BGI installation uses a different directory.

The default graphics driver path configured in the source is:

```c
initgraph(&gdriver, &gmode, "C:\\TC\\BGI");
```

5. Compile and run the program in a compatible environment.

## How to Play

| Action | Control |
|---|---|
| Select a chess piece | Right-click on the piece |
| Move a selected piece | Left-click on a highlighted position |
| Clear selection | Click the Clear Selection button |
| View instructions | Click the Instructions button |
| Restart the game | Click the Restart button |
| Exit | Click the Exit button |

Two players take turns moving their respective pieces on the same chessboard.

## Learning Outcomes

This project demonstrates practical applications of:

- C programming and procedural programming.
- Two-dimensional arrays for representing board states.
- Fundamental computer graphics algorithms.
- Drawing and rendering objects using graphics primitives.
- Mouse event handling and interactive user interfaces.
- Implementing movement and capture logic for different chess pieces.

## Author

**Ashutosh Kumar**

Developed as an academic project exploring Computer Graphics and Data Structures using C.
