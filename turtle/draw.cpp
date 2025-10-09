#include "draw.hpp"


void verticalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t y2, TPixel color, LINE_PLOT_CALLBACK plot)
{
    for (uint16_t y = min(y1, y2); y <= max(y1, y2); ++y)
    {
        if (plot) plot(img, {x1, y});
        else tigrPlot(img, x1, y, color);
    }
}

void horizontalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t x2, TPixel color, LINE_PLOT_CALLBACK plot)
{
    for (uint16_t x = min(x1, x2); x <= max(x1, x2); ++x)
    {
        if (plot) plot(img, {x, y1});
        else tigrPlot(img, x, y1, color);
    }
}

void line(Tigr *img, vec2p p1, vec2p p2, uint16_t r, TPixel color, LINE_PLOT_CALLBACK plot) {
 
    int16_t dx = p2.x - p1.x;
    int16_t dy = p2.y - p1.y;

    /* If horizontal or vertical, can use simpler functions to draw */
    if (dx == 0)
    {
        verticalLine(img, p1.x, p1.y, p2.y, color, plot);
        return;
    }
    if (dy == 0)
    {
        horizontalLine(img, p1.x, p1.y, p2.x, color, plot);
        return;
    }

    // Determine the direction of movement
    int16_t sx = (dx > 0) ? 1 : -1;
    int16_t sy = (dy > 0) ? 1 : -1;

    int16_t abs_dx = abs(dx);
    int16_t abs_dy = abs(dy);

    int16_t err = abs_dx - abs_dy;

    while (p1.x != p2.x || p1.y != p2.y) {
        // Plot pixels
        if (r == 0)
            if (plot) plot(img, p1);
            else tigrPlot(img, p1.x, p1.y, color);
        else
            fillCircle(img, p1, r, color, plot);
        
        int16_t double_err = 2 * err;

        // Step along the x-axis
        if (double_err > -abs_dy) {
            p1.x += sx;
            err -= abs_dy;
        }

        // Step along the y-axis
        if (double_err < abs_dx) {
            err += abs_dx;
            p1.y += sy;
        }
    }
    // Plot pixels
    if (r == 0)
        if (plot) plot(img, p1);
        else tigrPlot(img, p1.x, p1.y, color);
    else
        fillCircle(img, p1, r, color, plot);
}


void fillCircle(Tigr *img, vec2p c, uint16_t r, TPixel color, LINE_PLOT_CALLBACK plot)
{
    vec2p lbound, ubound;
    lbound.x = max(0, c.x-r);
    lbound.y = max(0, c.y-r);
    ubound.x = min((uint16_t)img->w, (uint16_t)(c.x+r+1));
    ubound.y = min((uint16_t)img->h, (uint16_t)(c.y+r+1));

    for (int32_t x = lbound.x; x < ubound.x; ++x)
    for (int32_t y = lbound.y; y < ubound.y; ++y)
    {
        int32_t res = (x-c.x)*(x-c.x) + (y-c.y)*(y-c.y) - r*r;
        if (res < 0) 
            if (plot) plot(img, vec2p(x, y));
            else tigrPlot(img, x, y, color);
    }
}