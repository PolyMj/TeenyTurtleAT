#include "detect_change.hpp"
#include "util.hpp"
#include "draw.hpp"
#include <unordered_set>

queue<ColorChange> change_queue;
unordered_set<ColorChange, CC_Hash> seen_colors;
vec2p prev_position;

typedef void(*CHECK_CALLBACK)(Tigr *img, vec2p pos);
void checkForChange(Tigr *img, vec2p pos);
void checkVerticalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t y2, uint16_t r);
void checkHorizontalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t r);
void checkCircle(Tigr *img, vec2p c, uint16_t r, CHECK_CALLBACK cb = checkForChange);

void checkForChange(Tigr *img, vec2p pos) {
    ColorChange cc = {
        .new_color = colorTo16b(tigrGet(img, pos.x, pos.y)),
        .old_position = prev_position
    };

    auto [it, inserted] = seen_colors.insert(cc);
    if (inserted) change_queue.push(cc);
}

void addColorsToSeen(Tigr *img, vec2p pos) {
    ColorChange cc = {
        .new_color = colorTo16b(tigrGet(img, pos.x, pos.y)),
        .old_position = {0,0}
    };

    auto [it, inserted] = seen_colors.insert(cc);
}

void getColorChanges(Tigr *img, vec2p p1, vec2p p2, uint16_t r) {
    // Reinitialize lists
    change_queue = queue<ColorChange>();
    seen_colors = unordered_set<ColorChange, CC_Hash>();

    // Ignore colors that the turtle is currently sitting on
    checkCircle(img, p1, r, addColorsToSeen);

    prev_position = p1;
 
    int16_t dx = p2.x - p1.x;
    int16_t dy = p2.y - p1.y;

    /* If horizontal or vertical, can use simpler functions to draw */
    if (dx == 0)
    {
        checkVerticalLine(img, p1.x, p1.y, p2.y, r);
        return;
    }
    if (dy == 0)
    {
        checkHorizontalLine(img, p1.x, p1.y, p2.x, r);
        return;
    }

    // Determine the direction of movement
    int16_t sx = (dx > 0) ? 1 : -1;
    int16_t sy = (dy > 0) ? 1 : -1;

    int16_t abs_dx = abs(dx);
    int16_t abs_dy = abs(dy);

    int16_t err = abs_dx - abs_dy;

    while (p1.x != p2.x || p1.y != p2.y) {
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

        // Plot pixels
        if (r <= 1)
            checkForChange(img, p1);
        else
            checkCircle(img, p1, r);

        prev_position = p1;
    }
}

void checkVerticalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t y2, uint16_t r)
{
    for (uint16_t x = max(0,x1-r+1); x < min(img->w, x1+r); ++x)
    {
        for (uint16_t y = min(y1, y2); y <= max(y1, y2); ++y)
        {
            checkForChange(img, {x,y});
            prev_position = {x,y};
        }
    }

    if (r > 1)
    {
        checkCircle(img, {x1, y1}, r);
        checkCircle(img, {x1, y2}, r);
    }
}

void checkHorizontalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t r)
{
    for (uint16_t y = max(0,y1-r+1); y < min(img->h, y1+r); ++y)
    {
        for (uint16_t x = min(x1, x2); x <= max(x1, x2); ++x)
        {
            checkForChange(img, {x,y});
            prev_position = {x,y};
        }
    }

    if (r > 1)
    {
        checkCircle(img, {x1, y1}, r);
        checkCircle(img, {x2, y1}, r);
    }
}

void checkCircle(Tigr *img, vec2p c, uint16_t r, CHECK_CALLBACK cb)
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
        if (res < 0) cb(img, {x,y});
    }
}