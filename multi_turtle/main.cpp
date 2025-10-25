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


#define TURTLE_INT_MOVE_DONE        TNY_XINT0
#define TURTLE_INT_HIT_EDGE         TNY_XINT1

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
            default:
                data->u = 0;
                break;
        }
    }

    void bus_write(tny_uword addr, tny_word data, uint16_t *delay) {
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
                break;
            case PEN_COLOR:
                pen_color = colorTo24b(data.u);
                break;
            case PEN_SIZE:
                pen_size = data.u;
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
            default:
                break;
        }
    }

    void calc_move_turtle() {
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
            dir = vec2f(std::cos(fixed_angle), std::sin(fixed_angle)).normalize();
            subtarget_pos += move_amnt * dir;
            move_done = false;
        }
        // NOTE: color-change ignored for now
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
        it->second->bus_write(addr, data, delay);
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
        for(auto &up : instances) {
            tny_clock(&up->t);
        }

        --cycles_until_frame;
        if(cycles_until_frame < 0) {
            tigrBlit(window, base_image, 0, 0, 0, 0, base_image->w, base_image->h);

            // draw each turtle
            for(auto &up : instances) {
                rotate_turtle(window, g_turtle_image, up->position.x, up->position.y, up->heading);
            }

            // perform movement updates for each instance (do_move then calc_move)
            for(auto &up : instances) {
                if(up->move_target || up->move_forward) {
                    up->do_move_turtle(base_image);
                    up->calc_move_turtle();
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
