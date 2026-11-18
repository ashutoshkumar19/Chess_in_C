/*
 * bgi_win32.c - minimal Borland BGI / Turbo C compatibility layer for Win32.
 *
 * Implements the subset of graphics.h, conio.h and dos.h used by CHESS.C:
 * an indexed 640x480 framebuffer rendered with GDI, a byte-per-pixel index
 * buffer (so getpixel() returns BGI color numbers), scanline floodfill,
 * bitmap text, and Int 33h style mouse calls.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bgi_win32.h"
#include "dos.h"

#define SCRW 640
#define SCRH 480
#define TEXT_CELL_W 8
#define TEXT_CELL_H 19
#define PI 3.14159265358979323846

static const unsigned int bgi_palette[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500,
    0xAAAAAA, 0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF,
    0xFFFF55, 0xFFFFFF
};

static HWND g_hwnd;
static HDC g_memdc;
static HBITMAP g_dib;
static unsigned int *g_dibbits;
static HDC g_tmpdc;
static HBITMAP g_tmpdib;
static unsigned int *g_tmpbits;

static unsigned char g_ib[SCRW * SCRH];
static int g_inited;
static int g_quit;
static int g_dirty;
static DWORD g_last_render;

static int g_color = 15;
static int g_bkcolor = 0;
static int g_fillcolor = 15;
static int g_textsize = 1;
static int g_cursorx = 1;
static int g_cursory = 1;

static int g_mx, g_my;
static int g_btn_phys;
static int g_btn_latch;
static DWORD g_latch_tick;
static int g_mouse_visible = 1;

static int g_keyqueue[64];
static int g_kqhead, g_kqtail;

static HFONT g_font;
static int g_fontsize;

static void pump(void);
static void render(void);
static void draw_text(int x, int y, const char *s);

/* ------------------------------------------------------------------ */
/* drawing primitives                                                  */

static void set_idx(int x, int y, unsigned char c)
{
    if (x < 0 || y < 0 || x >= SCRW || y >= SCRH)
        return;
    g_ib[y * SCRW + x] = c;
    g_dirty = 1;
}

void putpixel(int x, int y, int color)
{
    set_idx(x, y, (unsigned char)(color & 15));
}

int getpixel(int x, int y)
{
    if (x < 0 || y < 0 || x >= SCRW || y >= SCRH)
        return 0;
    return g_ib[y * SCRW + x];
}

void line(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        putpixel(x1, y1, g_color);
        if (x1 == x2 && y1 == y2)
            break;
        {
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x1 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y1 += sy;
            }
        }
    }
}

void circle(int x, int y, int r)
{
    int px = r, py = 0, err = 1 - r;
    while (px >= py) {
        putpixel(x + px, y + py, g_color);
        putpixel(x + py, y + px, g_color);
        putpixel(x - py, y + px, g_color);
        putpixel(x - px, y + py, g_color);
        putpixel(x - px, y - py, g_color);
        putpixel(x - py, y - px, g_color);
        putpixel(x + py, y - px, g_color);
        putpixel(x + px, y - py, g_color);
        py++;
        if (err < 0)
            err += 2 * py + 1;
        else {
            px--;
            err += 2 * (py - px) + 1;
        }
    }
}

void arc(int x, int y, int stangle, int endangle, int radius)
{
    int a0 = stangle, a1 = endangle;
    double step = 1.0;
    double a;
    if (a1 < a0)
        a1 += 360;
    if (a1 - a0 > 360)
        a1 = a0 + 360;
    for (a = a0; a <= a1; a += step) {
        int px = x + (int)floor(radius * cos(a * PI / 180.0) + 0.5);
        int py = y - (int)floor(radius * sin(a * PI / 180.0) + 0.5);
        putpixel(px, py, g_color);
    }
}

/* Scanline floodfill matching BGI semantics: fill every pixel reachable
 * from the seed that is neither the border color nor already the fill
 * color. */
void floodfill(int x, int y, int border)
{
    static int stack[1 << 19];
    int sp = 0;
    unsigned char b = (unsigned char)(border & 15);
    unsigned char nf = (unsigned char)(g_fillcolor & 15);
    if (x < 0 || y < 0 || x >= SCRW || y >= SCRH)
        return;
    if (g_ib[y * SCRW + x] == b)
        return;
    stack[sp++] = y * SCRW + x;
    while (sp > 0) {
        int p = stack[--sp];
        int px, py;
        if (g_ib[p] == b || g_ib[p] == nf)
            continue;
        g_ib[p] = nf;
        g_dirty = 1;
        px = p % SCRW;
        py = p / SCRW;
        if (sp < (1 << 19) - 4) {
            if (px > 0)
                stack[sp++] = p - 1;
            if (px < SCRW - 1)
                stack[sp++] = p + 1;
            if (py > 0)
                stack[sp++] = p - SCRW;
            if (py < SCRH - 1)
                stack[sp++] = p + SCRW;
        }
    }
}

/* ------------------------------------------------------------------ */
/* state                                                               */

void setcolor(int color) { g_color = color & 15; }
int getcolor(void) { return g_color; }
void setbkcolor(int color) { g_bkcolor = color & 15; }
int getbkcolor(void) { return g_bkcolor; }
void setfillstyle(int pattern, int color)
{
    (void)pattern;
    g_fillcolor = color & 15;
}
void settextstyle(int font, int direction, int charsize)
{
    (void)font;
    (void)direction;
    g_textsize = charsize < 1 ? 1 : charsize;
}

void cleardevice(void)
{
    int i;
    for (i = 0; i < SCRW * SCRH; i++)
        g_ib[i] = (unsigned char)g_bkcolor;
    g_cursorx = 1;
    g_cursory = 1;
    g_dirty = 1;
}

void clearviewport(void)
{
    cleardevice();
}

void gotoxy(int x, int y)
{
    g_cursorx = x;
    g_cursory = y;
}

int wherex(void) { return g_cursorx; }
int wherey(void) { return g_cursory; }

/* ------------------------------------------------------------------ */
/* bitmap text                                                         */

static HFONT get_font(int size)
{
    int height = 8 * size + 4;
    if (g_font && g_fontsize == size)
        return g_font;
    if (g_font)
        DeleteObject(g_font);
    g_font = CreateFontA(-height, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                         ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         NONANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN,
                         "Consolas");
    if (!g_font)
        g_font = CreateFontA(-height, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                             FIXED_PITCH | FF_MODERN, "Courier New");
    g_fontsize = size;
    return g_font;
}

/* Render text into the temp bitmap, then stamp only the glyph pixels so
 * that text never disturbs the index buffer's background. */
static void draw_text(int x, int y, const char *s)
{
    HGDIOBJ old;
    SIZE sz;
    int len, i, j;
    if (!g_inited || !s || !*s)
        return;
    len = (int)strlen(s);
    old = SelectObject(g_tmpdc, get_font(g_textsize));
    GetTextExtentPoint32A(g_tmpdc, s, len, &sz);
    if (sz.cx > SCRW)
        sz.cx = SCRW;
    if (sz.cy > SCRH)
        sz.cy = SCRH;
    SetBkColor(g_tmpdc, RGB(0, 0, 0));
    SetTextColor(g_tmpdc, RGB(255, 255, 255));
    SetBkMode(g_tmpdc, OPAQUE);
    TextOutA(g_tmpdc, 0, 0, s, len);
    SelectObject(g_tmpdc, old);

    for (j = 0; j < sz.cy; j++) {
        int py = y + j;
        if (py < 0 || py >= SCRH)
            continue;
        for (i = 0; i < sz.cx; i++) {
            int px = x + i;
            if (px < 0 || px >= SCRW)
                continue;
            if (g_tmpbits[j * SCRW + i])
                g_ib[py * SCRW + px] = (unsigned char)g_color;
        }
    }
    g_dirty = 1;
}

void outtextxy(int x, int y, const char *text)
{
    draw_text(x, y, text);
}

int bgi_printf(const char *format, ...)
{
    char buf[1024];
    va_list ap;
    int n;
    const char *p;

    va_start(ap, format);
    n = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    if (!g_inited) {
        fputs(buf, stdout);
        fflush(stdout);
        return n;
    }
    for (p = buf; *p; p++) {
        if (*p == '\n') {
            g_cursorx = 1;
            g_cursory++;
        } else if (*p == '\r') {
            g_cursorx = 1;
        } else {
            char tmp[2];
            int px = (g_cursorx - 1) * TEXT_CELL_W;
            int py = (g_cursory - 1) * TEXT_CELL_H;
            int i, j;
            unsigned char bg = (unsigned char)g_bkcolor;
            if (px >= 0 && px < SCRW && py >= 0 && py < SCRH)
                bg = g_ib[py * SCRW + px];
            for (j = py; j < py + TEXT_CELL_H; j++)
                for (i = px; i < px + TEXT_CELL_W; i++)
                    set_idx(i, j, bg);
            tmp[0] = *p;
            tmp[1] = 0;
            draw_text(px, py, tmp);
            g_cursorx++;
        }
    }
    pump();
    return n;
}

/* ------------------------------------------------------------------ */
/* window, input, mouse                                                */

static void push_key(int ch)
{
    int next = (g_kqtail + 1) % 64;
    if (next != g_kqhead) {
        g_keyqueue[g_kqtail] = ch;
        g_kqtail = next;
    }
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CLOSE:
        g_quit = 1;
        return 0;
    case WM_DESTROY:
        g_quit = 1;
        PostQuitMessage(0);
        return 0;
    case WM_CHAR:
        if (wp < 256)
            push_key((int)wp);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE)
            push_key(27);
        return 0;
    case WM_MOUSEMOVE:
        g_mx = (short)LOWORD(lp);
        g_my = (short)HIWORD(lp);
        return 0;
    case WM_LBUTTONDOWN:
        g_mx = (short)LOWORD(lp);
        g_my = (short)HIWORD(lp);
        g_btn_phys |= 1;
        g_btn_latch |= 1;
        g_latch_tick = GetTickCount();
        SetCapture(hwnd);
        return 0;
    case WM_LBUTTONUP:
        g_btn_phys &= ~1;
        ReleaseCapture();
        g_latch_tick = GetTickCount();
        return 0;
    case WM_RBUTTONDOWN:
        g_mx = (short)LOWORD(lp);
        g_my = (short)HIWORD(lp);
        g_btn_phys |= 2;
        g_btn_latch |= 2;
        g_latch_tick = GetTickCount();
        SetCapture(hwnd);
        return 0;
    case WM_RBUTTONUP:
        g_btn_phys &= ~2;
        ReleaseCapture();
        g_latch_tick = GetTickCount();
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        render();
        BitBlt(dc, 0, 0, SCRW, SCRH, g_memdc, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void draw_mouse_cursor(void)
{
    int x = g_mx, y = g_my, i;
    int pts[9][2] = {
        {0, 0}, {0, 13}, {3, 10}, {5, 15}, {8, 14}, {6, 9}, {10, 9}, {0, 0}
    };
    for (i = 0; i < 8; i++) {
        int x1 = x + pts[i][0], y1 = y + pts[i][1];
        int x2 = x + pts[i + 1][0], y2 = y + pts[i + 1][1];
        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            if (x1 >= 0 && y1 >= 0 && x1 < SCRW && y1 < SCRH)
                g_dibbits[y1 * SCRW + x1] = 0xFFFFFF;
            if (x1 == x2 && y1 == y2)
                break;
            {
                int e2 = 2 * err;
                if (e2 >= dy) {
                    err += dy;
                    x1 += sx;
                }
                if (e2 <= dx) {
                    err += dx;
                    y1 += sy;
                }
            }
        }
    }
}

static void render(void)
{
    int i;
    if (!g_inited)
        return;
    for (i = 0; i < SCRW * SCRH; i++)
        g_dibbits[i] = bgi_palette[g_ib[i]];
    if (g_mouse_visible)
        draw_mouse_cursor();
    g_dirty = 0;
    g_last_render = GetTickCount();
}

static void pump(void)
{
    MSG msg;
    DWORD now;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            g_quit = 1;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    now = GetTickCount();
    g_btn_latch &= g_btn_phys;
    if (g_btn_latch && now - g_latch_tick > 250)
        g_btn_latch = 0;
    if (g_dirty && now - g_last_render >= 16) {
        render();
        if (g_hwnd) {
            HDC dc = GetDC(g_hwnd);
            BitBlt(dc, 0, 0, SCRW, SCRH, g_memdc, 0, 0, SRCCOPY);
            ReleaseDC(g_hwnd, dc);
        }
    }
}

int kbhit(void)
{
    pump();
    return g_quit || g_kqhead != g_kqtail;
}

int getch(void)
{
    while (!g_quit && g_kqhead == g_kqtail) {
        pump();
        Sleep(5);
    }
    if (g_kqhead != g_kqtail) {
        int c = g_keyqueue[g_kqhead];
        g_kqhead = (g_kqhead + 1) % 64;
        return c;
    }
    return 27;
}

void delay(unsigned milliseconds)
{
    DWORD start = GetTickCount();
    while (GetTickCount() - start < milliseconds) {
        pump();
        Sleep(1);
    }
    pump();
}

int int86(int intno, union REGS *inregs, union REGS *outregs)
{
    if (outregs)
        memset(outregs, 0, sizeof(*outregs));
    if (intno != 0x33 || !inregs)
        return 0;
    switch (inregs->x.ax) {
    case 0x00: /* reset / detect driver */
        outregs->x.ax = 0xFFFF;
        outregs->x.bx = 3;
        break;
    case 0x01: /* show pointer */
        g_mouse_visible = 1;
        pump();
        render();
        g_dirty = 1;
        pump();
        break;
    case 0x02: /* hide pointer */
        g_mouse_visible = 0;
        g_dirty = 1;
        pump();
        break;
    case 0x03: /* read position + buttons */
        pump();
        outregs->x.ax = 0xFFFF;
        outregs->x.bx = (unsigned)(g_btn_phys | g_btn_latch);
        outregs->x.cx = (unsigned)g_mx;
        outregs->x.dx = (unsigned)g_my;
        break;
    default:
        break;
    }
    return outregs ? (int)outregs->x.ax : 0;
}

/* ------------------------------------------------------------------ */
/* init / shutdown                                                     */

void initgraph(int *gdriver, int *gmode, const char *pathtodriver)
{
    WNDCLASSEXA wc;
    RECT rc;
    BITMAPINFO bmi;
    HINSTANCE inst = GetModuleHandleA(NULL);
    (void)pathtodriver;

    if (g_inited)
        return;

    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.lpszClassName = "BgiWin32Chess";
    RegisterClassExA(&wc);

    rc.left = 0;
    rc.top = 0;
    rc.right = SCRW;
    rc.bottom = SCRH;
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                                WS_MINIMIZEBOX, FALSE, 0);
    g_hwnd = CreateWindowExA(0, "BgiWin32Chess",
                             "CHESS - Turbo C BGI on Win32 (GCC build)",
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                                 WS_MINIMIZEBOX,
                             60, 40, rc.right - rc.left, rc.bottom - rc.top,
                             NULL, NULL, inst, NULL);
    if (!g_hwnd) {
        if (gdriver) *gdriver = 0;
        return;
    }

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SCRW;
    bmi.bmiHeader.biHeight = -SCRH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_memdc = CreateCompatibleDC(NULL);
    g_dib = CreateDIBSection(g_memdc, &bmi, DIB_RGB_COLORS,
                             (void **)&g_dibbits, NULL, 0);
    SelectObject(g_memdc, g_dib);

    g_tmpdc = CreateCompatibleDC(NULL);
    g_tmpdib = CreateDIBSection(g_tmpdc, &bmi, DIB_RGB_COLORS,
                                (void **)&g_tmpbits, NULL, 0);
    SelectObject(g_tmpdc, g_tmpdib);

    g_inited = 1;
    g_bkcolor = BLACK;
    cleardevice();
    render();

    ShowWindow(g_hwnd, SW_SHOW);
    SetForegroundWindow(g_hwnd);
    SetFocus(g_hwnd);
    if (gdriver) *gdriver = 0;
    if (gmode) *gmode = 0;
}

int graphresult(void)
{
    return grOk;
}

char *grapherrormsg(int errorcode)
{
    (void)errorcode;
    return "No error";
}

void closegraph(void)
{
    if (!g_inited)
        return;
    g_inited = 0;
    if (g_font)
        DeleteObject(g_font);
    g_font = NULL;
    if (g_tmpdib)
        DeleteObject(g_tmpdib);
    if (g_tmpdc)
        DeleteDC(g_tmpdc);
    if (g_dib)
        DeleteObject(g_dib);
    if (g_memdc)
        DeleteDC(g_memdc);
    if (g_hwnd)
        DestroyWindow(g_hwnd);
}