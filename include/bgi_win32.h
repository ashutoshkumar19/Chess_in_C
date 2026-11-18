#ifndef BGI_WIN32_H
#define BGI_WIN32_H

#include <stdlib.h>

#define BGI_VERSION "win32-compat 1.0"

/* BGI/Ega color constants */
enum {
    BLACK = 0, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
    DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED,
    LIGHTMAGENTA, YELLOW, WHITE
};

/* fill patterns (only SOLID_FILL is implemented) */
enum { EMPTY_FILL = 0, SOLID_FILL, LINE_FILL, LTSLASH_FILL, SLASH_FILL,
       BKSLASH_FILL, LTBKSLASH_FILL, HATCH_FILL, XHATCH_FILL,
       INTERLEAVE_FILL, WIDE_DOT_FILL, CLOSE_DOT_FILL, USER_FILL };

/* text fonts (all map to the same built-in fixed font) */
enum { DEFAULT_FONT = 0, TRIPLEX_FONT, SMALL_FONT, SANS_SERIF_FONT,
       GOTHIC_FONT };

#define DETECT 0
#define grOk 0

/* graphics */
void initgraph(int *gdriver, int *gmode, const char *pathtodriver);
void closegraph(void);
int graphresult(void);
char *grapherrormsg(int errorcode);
void cleardevice(void);
void clearviewport(void);
void setbkcolor(int color);
int getbkcolor(void);
void setcolor(int color);
int getcolor(void);
void setfillstyle(int pattern, int color);
void putpixel(int x, int y, int color);
int getpixel(int x, int y);
void floodfill(int x, int y, int border);
void line(int x1, int y1, int x2, int y2);
void arc(int x, int y, int stangle, int endangle, int radius);
void circle(int x, int y, int radius);
void outtextxy(int x, int y, const char *text);
void settextstyle(int font, int direction, int charsize);
void gotoxy(int x, int y);
int wherex(void);
int wherey(void);

/* console/keyboard */
int kbhit(void);
int getch(void);
void delay(unsigned milliseconds);

/* printf replacement drawn into the graphics window at the text cursor */
int bgi_printf(const char *format, ...);

#endif