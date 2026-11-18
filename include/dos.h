#ifndef DOS_H_PORT
#define DOS_H_PORT
#include "bgi_win32.h"

#ifdef __cplusplus
extern "C" {
#endif

union REGS {
    struct {
        unsigned int ax, bx, cx, dx, si, di, cflag;
    } x;
    struct {
        unsigned short ax, bx, cx, dx;
    } w;
    struct {
        unsigned char al, ah, bl, bh, cl, ch, dl, dh;
    } h;
};

int int86(int intno, union REGS *inregs, union REGS *outregs);

#ifdef __cplusplus
}
#endif

#endif
