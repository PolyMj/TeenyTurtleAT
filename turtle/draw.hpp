#include "tigr.h"
#include "vec.hpp"

using namespace std;

typedef void(*LINE_PLOT_CALLBACK)(Tigr *img, vec2p pos);

// Draws a line
    // plot is the callback function called for each x,y, position on the line
    // plot defaults to NULL, in which case tigrPlot is called to draw each pixel
    // r=0 --> plot is called on the pixels on the line, r>0 --> takes r to be pen radius
void line(Tigr *img, vec2p p1, vec2p p2, uint16_t r, TPixel color, LINE_PLOT_CALLBACK plot = NULL);

void verticalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t y2, TPixel color, LINE_PLOT_CALLBACK plot = NULL);

void horizontalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t x2, TPixel color, LINE_PLOT_CALLBACK plot = NULL);

void fillCircle(Tigr *img, vec2p c, uint16_t r, TPixel color, LINE_PLOT_CALLBACK plot = NULL);
