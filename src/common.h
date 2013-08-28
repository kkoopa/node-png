#ifndef COMMON_H
#define COMMON_H

#include <node.h>
#include <cstring>

struct Point {
    int x, y;
    Point() {}
    Point(int xx, int yy) : x(xx), y(yy) {}
};

struct Rect {
    int x, y, w, h;
    Rect() {}
    Rect(int xx, int yy, int ww, int hh) : x(xx), y(yy), w(ww), h(hh) {}
    bool isNull() { return x == 0 && y == 0 && w == 0 && h == 0; }
};

bool str_eq(const char *s1, const char *s2);

typedef enum { BUF_RGB, BUF_BGR, BUF_RGBA, BUF_BGRA, BUF_GRAY } buffer_type;

#endif

