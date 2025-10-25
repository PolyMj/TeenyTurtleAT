#define _USE_MATH_DEFINES

#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <ctime>

#include "vec.hpp"
#include "tigr.h"
#include "draw.hpp"
#include "teenyat.h"
#include "util.hpp"
#include <unordered_set>
#include <queue>

using namespace std;

#define TURTLE_X       0xD000
#define TURTLE_Y       0xD001
#define TURTLE_ANGLE   0xD002
#define SET_X          0xD003
#define SET_Y          0xD004
#define PEN_DOWN       0xD010
#define PEN_UP         0xD011
#define PEN_COLOR      0xD012
#define PEN_SIZE       0xD013
#define GOTO_XY        0xE000
#define FACE_XY        0xE001
#define MOVE           0xE002
#define DETECT         0xE003
#define SET_ERASER     0xE004
#define DETECT_AHEAD   0xE005
#define GET_COLOR_CHANGE  0xE010 // Load the current color change into the given register
#define NEXT_COLOR_CHANGE 0xE011 // Loads the number of color changes remaining into the given register; loads the next color into the GET_COLOR_CHANGE peripheral
#define STOP_MOVE      0xE012 // Cancels the current movement


#define TURTLE_INT_MOVE_DONE        TNY_XINT0
#define TURTLE_INT_HIT_EDGE         TNY_XINT1
#define TURTLE_INT_COLOR_CHANGE     TNY_XINT2

// Shared constants
const int windowWidth = 640;
const int windowHeight = 500;
const int turtle_size = 8;
const uint16_t detect_ahead_distance = 15;
const int FPS = 60;
const int cycles_per_frame = 1e6 / FPS;
const float speed_fps_adjust = 60.0f / (float)(FPS);

void rotate_turtle(Tigr* dest, Tigr* turtle, float cx, float cy, float angleDegrees);

// Registry to map teenyat* -> instance
struct TurtleInstance;
static unordered_map<teenyat*, TurtleInstance*> g_registry;

typedef struct ColorChange_data {
    uint16_t new_color;
    vec2p old_position;
    bool operator==(const ColorChange_data &other) const {
        return other.new_color == new_color;
    }
} ColorChange;

struct CC_Hash {
    size_t operator()(const ColorChange &cc) const {
        return hash<uint16_t>()(cc.new_color);
    }
};

// Per-instance state and behavior
struct TurtleInstance {
    teenyat t; // embedded teenyat instance

    vec2f position;
    vec2f target_position;
    double heading = 0.0;

    TPixel pen_color = {0,0,0,255};
    int pen_size = 5;
    bool pen_down = false;

    bool move_target = false;
    bool move_forward = false;
    float move_speed = 6.0f;

    vec2f subtarget_pos;
    bool move_done = false;
    // color change detection state (per-instance)
    queue<ColorChange> change_queue;
    unordered_set<ColorChange, CC_Hash> seen_colors;
    vec2p prev_position_for_detect;
    ColorChange last_color_change;
    bool color_change_set = false;

    // set defaults
    TurtleInstance(): position(vec2(320.0f,250.0f)), target_position(position), subtarget_pos(position) {}

    // bus read/write functions used via wrapper
    void bus_read(tny_uword addr, tny_word *data, uint16_t *delay, Tigr* base_image) {
        switch(addr) {
            case TURTLE_X:
                data->u = position.x;
                break;
            case TURTLE_Y:
                data->u = position.y;
                break;
            case TURTLE_ANGLE:
                data->u = heading;
                break;
            case DETECT:
                data->u = colorTo16b(tigrGet(base_image,
                    (int)position.x,
                    (int)position.y));
                break;
            case DETECT_AHEAD: {
                // compute ahead pos and return pixel color
                float angle_rad = (heading + 270.0) * (M_PI / 180.0);
                float dx = position.x + detect_ahead_distance * cos(angle_rad);
                float dy = position.y + detect_ahead_distance * sin(angle_rad);
                int xi = (int)dx; int yi = (int)dy;
                if(xi < 0) xi = 0; if(yi < 0) yi = 0;
                if(xi >= base_image->w) xi = base_image->w - 1;
                if(yi >= base_image->h) yi = base_image->h - 1;
                data->u = colorTo16b(tigrGet(base_image, xi, yi));
                break;
            }
            case SET_X:
                data->u = target_position.x;
                break;
            case SET_Y:
                data->u = target_position.y;
                break;
            case GET_COLOR_CHANGE:
                data->u = last_color_change.new_color;
                break;
            case NEXT_COLOR_CHANGE:
                data->u = change_queue.size();
                if (change_queue.empty()) {
                    color_change_set = false;
                    break;
                }
                last_color_change = change_queue.front(); change_queue.pop();
                color_change_set = true;
                break;
            default:
                data->u = 0;
                break;
        }
    }

    void bus_write(tny_uword addr, tny_word data, uint16_t *delay, Tigr* base_image) {
        switch(addr) {
            case GOTO_XY:
                move_target = true;
                break;
            case MOVE:
                move_forward = data.u;
                break;
            case FACE_XY:
                // compute heading to face target_position
                {
                    vec2 dir = target_position - position;
                    float angle_adjustment = 270.0f;
                    float target_angle = atan2(dir.y, dir.x) * (180.0f / M_PI) - angle_adjustment;
                    float angle_diff = target_angle - heading;
                    while(angle_diff > 180.0f) angle_diff -= 360.0f;
                    while(angle_diff < -180.0f) angle_diff += 360.0f;
                    heading += angle_diff;
                    while(heading >= 360.0f) heading -= 360.0f;
                    while(heading < 0.0f) heading += 360.0f;
                }
                break;
            case PEN_UP:
                pen_down = false;
                break;
            case PEN_DOWN:
                pen_down = true;
                drawDot(base_image);
                break;
            case PEN_COLOR:
                pen_color = colorTo24b(data.u);
                if (pen_down) drawDot(base_image);
                break;
            case PEN_SIZE:
                pen_size = data.u;
                if (pen_down) drawDot(base_image);
                break;
            case SET_X:
                target_position.x = data.u;
                break;
            case SET_Y:
                target_position.y = data.u;
                break;
            case TURTLE_ANGLE:
                heading = data.u;
                while(heading >= 360.0f) heading -= 360.0f;
                while(heading < 0.0f) heading += 360.0f;
                break;
            case STOP_MOVE:
                if (color_change_set) {
                    subtarget_pos = last_color_change.old_position;
                }
                else {
                    subtarget_pos = position;
                }
                break;
            default:
                break;
        }
    }

    // Draws a "dot" to the screen. Should be called when the pen is down and a change to the pen has been made. 
    inline void drawDot(Tigr* base_image) {
        fillCircle(base_image, position, pen_size, pen_color, NULL);
    }

    void checkForChange(Tigr *img, vec2p pos) {
        ColorChange cc = {
            .new_color = colorTo16b(tigrGet(img, pos.x, pos.y)),
            .old_position = prev_position_for_detect
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

    void checkCircle(Tigr *img, vec2p c, uint16_t r) {
        vec2p lbound, ubound;
        lbound.x = max(0, c.x-r);
        lbound.y = max(0, c.y-r);
        ubound.x = min((uint16_t)img->w, (uint16_t)(c.x+r+1));
        ubound.y = min((uint16_t)img->h, (uint16_t)(c.y+r+1));

        for (int32_t x = lbound.x; x < ubound.x; ++x)
        for (int32_t y = lbound.y; y < ubound.y; ++y)
        {
            int32_t res = (x-c.x)*(x-c.x) + (y-c.y)*(y-c.y) - r*r;
            if (res < 0) checkForChange(img, {x,y});
        }
    }

    void checkVerticalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t y2, uint16_t r)
    {
        for (uint16_t x = max(0,x1-r); x < min(img->w, x1+r+1); ++x)
        {
            for (uint16_t y = min(y1, y2); y <= max(y1, y2); ++y)
            {
                checkForChange(img, {x,y});
            }
        }

        if (r > 1)
        {
            checkCircle(img, {x1, y1}, r);
            checkCircle(img, {x1, y2}, r);
        }
        prev_position_for_detect = {x1, (uint16_t)max(y1, y2)};
    }

    void checkHorizontalLine(Tigr *img, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t r)
    {
        for (uint16_t y = max(0,y1-r); y < min(img->h, y1+r+1); ++y)
        {
            for (uint16_t x = min(x1, x2); x <= max(x1, x2); ++x)
            {
                checkForChange(img, {x,y});
            }
        }

        if (r > 1)
        {
            checkCircle(img, {x1, y1}, r);
            checkCircle(img, {x2, y1}, r);
        }
        prev_position_for_detect = {(uint16_t)max(x1, x2), y1};
    }

    void getColorChanges(Tigr *img, vec2p p1, vec2p p2, uint16_t r) {
        // Reinitialize per-instance lists
        change_queue = queue<ColorChange>();
        seen_colors = unordered_set<ColorChange, CC_Hash>();

        // Ignore colors that the turtle is currently sitting on
        // Manually add starting colors to seen_colors (inline addColorsToSeen logic)
        vec2p lbound, ubound;
        lbound.x = max(0, p1.x-r);
        lbound.y = max(0, p1.y-r);
        ubound.x = min((uint16_t)img->w, (uint16_t)(p1.x+r+1));
        ubound.y = min((uint16_t)img->h, (uint16_t)(p1.y+r+1));

        for (int32_t x = lbound.x; x < ubound.x; ++x)
        for (int32_t y = lbound.y; y < ubound.y; ++y)
        {
            int32_t res = (x-p1.x)*(x-p1.x) + (y-p1.y)*(y-p1.y) - r*r;
            if (res < 0) {
                addColorsToSeen(img, {x,y});
            }
        }
        
        prev_position_for_detect = p1;

        int16_t dx = p2.x - p1.x;
        int16_t dy = p2.y - p1.y;

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

        int16_t sx = (dx > 0) ? 1 : -1;
        int16_t sy = (dy > 0) ? 1 : -1;

        int16_t abs_dx = abs(dx);
        int16_t abs_dy = abs(dy);

        int16_t err = abs_dx - abs_dy;

        vec2p cur = p1;
        while (cur.x != p2.x || cur.y != p2.y) {
            int16_t double_err = 2 * err;

            if (double_err > -abs_dy) {
                cur.x += sx;
                err -= abs_dy;
            }

            if (double_err < abs_dx) {
                err += abs_dx;
                cur.y += sy;
            }

            if (r <= 1)
                checkForChange(img, cur);
            else
                checkCircle(img, cur, r);

            prev_position_for_detect = cur;
        }
    }

    void calc_move_turtle(Tigr* base_image) {
        vec2f dir;
        float move_amnt = speed_fps_adjust * move_speed;

        if (move_target) {
            dir = target_position - position;
            float distance = dir.length();
            if (distance <= move_amnt) {
                subtarget_pos = target_position;
                move_done = true;
            }
            else {
                dir = dir.normalize();
                subtarget_pos += move_amnt * dir;
                move_done = false;
            }
        }
        else if (move_forward) {
            double fixed_angle = (heading + 270) * M_PI / 180.0;
            dir = vec2f(cos(fixed_angle), sin(fixed_angle)).normalize();
            subtarget_pos += move_amnt * dir;
            move_done = false;
        }

        getColorChanges(base_image, position, subtarget_pos, turtle_size);
        if (!change_queue.empty()) {
            tny_external_interrupt(&t, TURTLE_INT_COLOR_CHANGE);
            last_color_change = change_queue.front(); change_queue.pop();
        }
    }

    void do_move_turtle(Tigr* base_image) {
        bool hit_edge = false;
        if (subtarget_pos.x < 0) { subtarget_pos.x = 0; hit_edge = true; }
        if (subtarget_pos.y < 0) { subtarget_pos.y = 0; hit_edge = true; }
        if (subtarget_pos.x > base_image->w - 1) { subtarget_pos.x = base_image->w - 1; hit_edge = true; }
        if (subtarget_pos.y > base_image->h - 1) { subtarget_pos.y = base_image->h - 1; hit_edge = true; }

        if (hit_edge) {
            tny_external_interrupt(&t, TURTLE_INT_HIT_EDGE);
        }
        if (move_done) {
            tny_external_interrupt(&t, TURTLE_INT_MOVE_DONE);
            move_target = false;
            move_done = false;
        }

        vec2p true_pos = position;
        vec2p true_targ = subtarget_pos;

        if (pen_down && true_pos != true_targ) {
            line(base_image, true_pos, true_targ, pen_size, pen_color, NULL);
        }

        position = subtarget_pos;
    }
};

// Bus wrapper callbacks used by teenyat (they lookup the instance)
static Tigr* g_base_image = nullptr;
static Tigr* g_turtle_image = nullptr;

static void bus_read_wrapper(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    auto it = g_registry.find(t);
    if (it != g_registry.end()) {
        it->second->bus_read(addr, data, delay, g_base_image);
    } else {
        data->u = 0;
    }
}

static void bus_write_wrapper(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    auto it = g_registry.find(t);
    if (it != g_registry.end()) {
        it->second->bus_write(addr, data, delay, g_base_image);
    }
}

int main(int argc, char *argv[]) {
    if(argc < 2 || argc > 4) {
        cout << "usage: multi_turtle <binary_file> [num_turtles?] [map_image?]" << endl;
        return 1;
    }

    string bin_name = argv[1];
    int num_turtles = 2;
    if(argc >= 3) num_turtles = atoi(argv[2]);
    if(num_turtles < 1) num_turtles = 1;

    vector<unique_ptr<TurtleInstance>> instances;

    Tigr* window = tigrWindow(windowWidth, windowHeight, "TeenyAT Multi-Turtle", TIGR_FIXED);
    Tigr* base_image = tigrBitmap(window->w, window->h);
    tigrClear(window, tigrRGB(255,255,255));
    tigrClear(base_image, tigrRGB(255,255,255));

    if(argc == 4) {
        const char* img_name = argv[3];
        base_image = tigrLoadImage(img_name);
        if(!base_image) tigrError(0, "Could not load image file");
    }

    g_base_image = base_image;

    g_turtle_image = tigrLoadImage("Images/Turtle.png");
    if(!g_turtle_image) tigrError(0, "Could not load Turtle.png");

    // Create instances
    for(int i = 0; i < num_turtles; ++i) {
        auto inst = make_unique<TurtleInstance>();

        // initialize teenyat from file
        FILE *bin_file = fopen(bin_name.c_str(), "rb");
        if(bin_file == NULL) {
            cerr << "Failed to open binary: " << bin_name << endl;
            return 1;
        }
        if(!tny_init_from_file(&inst->t, bin_file, bus_read_wrapper, bus_write_wrapper)) {
            cerr << "tny_init_from_file failed for instance " << i << endl;
            fclose(bin_file);
            return 1;
        }
        fclose(bin_file);

        // Diversify per-instance RNG so multiple instances don't produce identical
        // RAND sequences when created near-simultaneously.
        uintptr_t mix = (uintptr_t)inst.get();
        uint64_t timev = (uint64_t)time(NULL);
        inst->t.random.state ^= (uint64_t)(mix * 6364136223846793005ULL) + (timev << 3) + (uint64_t)i;
        inst->t.random.increment |= (uint64_t)(i * 2 + 1);

        // register mapping
        g_registry[&inst->t] = inst.get();

        instances.push_back(std::move(inst));
    }

    int cycles_until_frame = 0;
    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {
        // advance each teenyat once per loop
        for(auto &update : instances) {
            tny_clock(&update->t);
        }

        --cycles_until_frame;
        if(cycles_until_frame < 0) {
            tigrBlit(window, base_image, 0, 0, 0, 0, base_image->w, base_image->h);

            // draw each turtle
            for(auto &update : instances) {
                rotate_turtle(window, g_turtle_image, update->position.x, update->position.y, update->heading);
            }

            // perform movement updates for each instance: calc -> detect -> do_move
            for(auto &update : instances) {
                if(update->move_target || update->move_forward) {
                    update->calc_move_turtle(base_image);
                    update->do_move_turtle(base_image);
                }
            }

            tigrUpdate(window);
            cycles_until_frame = cycles_per_frame;
        }
    }

    // cleanup
    tigrFree(g_turtle_image);
    tigrFree(base_image);
    tigrFree(window);

    return EXIT_SUCCESS;
}

void rotate_turtle(Tigr* dest, Tigr* turtle, float cx, float cy, float angleDegrees) {
    float angle = angleDegrees * (M_PI / 180.0f);
    float cosA = cos(angle);
    float sinA = sin(angle);

    int w = turtle->w;
    int h = turtle->h;
    int halfW = w / 2;
    int halfH = h / 2;

    float corners[4][2] = {
        {-halfW, -halfH}, {halfW, -halfH},
        {halfW, halfH}, {-halfW, halfH}
    };

    float minX = 0, maxX = 0, minY = 0, maxY = 0;
    for(int i = 0; i < 4; i++) {
        float rotX = corners[i][0] * cosA - corners[i][1] * sinA;
        float rotY = corners[i][0] * sinA + corners[i][1] * cosA;
        if(i == 0 || rotX < minX) minX = rotX;
        if(i == 0 || rotX > maxX) maxX = rotX;
        if(i == 0 || rotY < minY) minY = rotY;
        if(i == 0 || rotY > maxY) maxY = rotY;
    }

    for(int dy = (int)minY; dy <= (int)maxY; dy++) {
        for(int dx = (int)minX; dx <= (int)maxX; dx++) {
            float srcX = dx * cosA + dy * sinA;
            float srcY = -dx * sinA + dy * cosA;

            int sx = (int)(srcX + halfW + 0.5f);
            int sy = (int)(srcY + halfH + 0.5f);

            if(sx >= 0 && sx < w && sy >= 0 && sy < h) {
                TPixel p = tigrGet(turtle, sx, sy);
                if(p.a > 0) {
                    tigrPlot(dest, (int)(cx + dx), (int)(cy + dy), p);
                }
            }
        }
    }
}
