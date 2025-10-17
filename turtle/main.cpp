#define _USE_MATH_DEFINES

#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cmath>
#include "vec.hpp"
#include "tigr.h"
#include "draw.hpp"
#include "teenyat.h"

/*  TeenyAT Turtle System
 *
 *  TURTLE_X:
 *  - 0xD000
 *  - read only
 *  - returns the current x positon of the turtle
 *
 *  TURTLE_Y:
 *  - 0xD001
 *  - read only
 *  - returns the current y positon of the turtle
 *
 *  TURTLE_ANGLE:
 *  - 0xD002
 *  - read only
 *  - returns the current angle of the turtle (can be changed by stores to portA)
 *
 *  SET_X:
 *  - 0xD003
 *  - read/write
 *  - sets the x component of a position the turtle can move too
 *
 *  SET_Y:
 *  - 0xD004
 *  - read/write
 *  - sets the y component of a position the turtle can move too
 *
 *  PEN_DOWN:
 *  - 0xD010
 *  - write only
 *  - puts the pen down onto the paper
 *
 *  PEN_UP:
 *  - 0xD011
 *  - write only
 *  - picks the up off the paper
 *
 *  PEN_COLOR:
 *  - 0xD012
 *  - read/write
 *  - sets the pen color to the value stored
 *
 *  PEN_SIZE:
 *  - 0xD013
 *  - read/write
 *  - sets the size of the turtles pen
 *
 *  GOTO_XY:
 *  - 0xE000
 *  - write only
 *  - moves the turtle until its reached its heading XY position
 *
 *  FACE_XY:
 *  - 0xE001
 *  - write only
 *  - rotates the turtle until they are facing its heading XY position
 *
 *  MOVE:
 *  - 0xE002
 *  - write only
 *  - on stores of non-zero will move the turtle in the direction they are facing
 *
 *  DETECT:
 *  - 0xE003
 *  - read only
 *  - returns the color underneath the turtles head
 *
 *  SET_ERASER:
 *  - 0xE004
 *  - write only
 *  - sets the pen color to white (our eraser color)
 *
 ***/

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
#define DETECT_AHEAD     0xE005

#define TURTLE_INT_MOVE_DONE        TNY_XINT0
#define TURTLE_INT_HIT_EDGE         TNY_XINT1
#define TURTLE_INT_COLOR_CHANGE     TNY_XINT2
// #define TURTLE_INT_FACE_DONE        TNY_XINT3

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);
void calc_move_turtle(teenyat *t);
void do_move_turtle(teenyat *t);
void rotate_turtle(Tigr* dest, Tigr* turtle, float cx, float cy, float angleDegrees);
void angle_to_rotate();
void normalize_angle();
vec2f detect();

Tigr* window;
Tigr* base_image;
Tigr* turtle_image;
int   windowWidth = 640;
int   windowHeight = 500;

vec2f     turtle_position           = vec2(320.0f,250.0f);
vec2f     turtle_last_position      = turtle_position;
vec2      turtle_target_position    = turtle_position;
double    turtle_heading            = 0.0f;
TPixel    pen_color                 = {0,0,0,255};
int       pen_size                  = 5;
bool      pen_down                  = false;
bool      erase_mode                = false;
bool      move_turtle_target        = false;
bool      move_turtle_forward       = false;
float     move_speed                = 1.0;

vec2f turtle_subtarget_pos  = turtle_position;
bool  move_done             = false;

const int      turtle_size           = 5;
const uint16_t detect_ahead_distance = 15;

int main(int argc, char *argv[]) {
    /* If only one parameter is provided then we treat it as the teenyat binary
     * but if two are provided then the first parameter is the teenyat binary and
     * the second is the 'map' image to draw
     *
     * */
    if(argc < 2 || argc > 3) {
        std::cout << "usage: turtle <binary_file> [map_image?]" << std::endl;
        return 1;
    }

    std::string bin_name = argv[1];
    teenyat t;
    FILE *bin_file = fopen(bin_name.c_str(), "rb");
    if(bin_file != NULL) {
        tny_init_from_file(&t, bin_file, bus_read, bus_write);
        fclose(bin_file);
    }else {
        std::cout << "Failed to init bin file (invalid path?)" << std::endl;
        return 0;
    }

    int frame_number = 0;
    window = tigrWindow(windowWidth, windowHeight, "TeenyAT Turtle", TIGR_FIXED);
    base_image = tigrBitmap(window->w, window->h);
    tigrClear(window, tigrRGB(255, 255, 255));
    tigrClear(base_image, tigrRGB(255, 255, 255));

    if(argc == 3) {
        const char* img_name = argv[2];
        base_image = tigrLoadImage(img_name);
        if(!base_image) tigrError(0, "Could not load image file");
    }

    turtle_image = tigrLoadImage("Images/Turtle.png");
    if(!turtle_image) {
        tigrError(0, "Could not load Turtle.png");
    }

    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {
        tny_clock(&t);

        /* Game ticks every 60 frames */
        if(!frame_number) {
            /* Move base_image ontop of our window */
            tigrBlit(window, base_image, 0, 0, 0, 0, base_image->w, base_image->h);

            /* We can just draw the turtle directly to the window as long as its after our base image drawing */
            rotate_turtle(window, turtle_image, turtle_position.x, turtle_position.y, turtle_heading);
            if(move_turtle_target || move_turtle_forward) {
                calc_move_turtle(&t);
                do_move_turtle(&t);
            }

            tigrUpdate(window);
            // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        frame_number = (frame_number + 1) % 60;
    }

    tigrFree(window);
    return EXIT_SUCCESS;
}

void calc_move_turtle(teenyat *t) {
    turtle_last_position = turtle_position;
    vec2f dir;

    // Move to a target
    if (move_turtle_target) {
        dir = turtle_target_position - turtle_position;
        float distance = dir.length();
        if (distance <= move_speed) {
            turtle_subtarget_pos = turtle_target_position;
            move_turtle_target = false;
            move_done = true;
        }
        else {
            dir = dir.normalize();
            turtle_subtarget_pos += move_speed * dir;
            move_done = false;
        }
    }
    // Move forward
    else if (move_turtle_forward) {
        double fixed_angle = (turtle_heading + 270) * M_PI / 180;
        dir = vec2f(std::cos(fixed_angle), std::sin(fixed_angle)).normalize();
        turtle_subtarget_pos += move_speed * dir;
        move_done = false;
    }
}

void do_move_turtle(teenyat *t) {
    bool hit_edge = false;
    // Keep turtle within window
    if (turtle_subtarget_pos.x < 0) {
        turtle_subtarget_pos.x = 0;
        hit_edge = true;
    }
    if (turtle_subtarget_pos.y < 0) {
        turtle_subtarget_pos.y = 0;
        hit_edge = true;
    }
    if (turtle_subtarget_pos.x > windowWidth-1) {
        turtle_subtarget_pos.x = windowWidth-1;
        hit_edge = true;
    }
    if (turtle_subtarget_pos.y > windowHeight-1) {
        turtle_subtarget_pos.y = windowHeight-1;
        hit_edge = true;
    }

    // Call interrupts as needed
    if (hit_edge) {
        tny_external_interrupt(t, TURTLE_INT_HIT_EDGE);
    }
    if (move_done) {
        tny_external_interrupt(t, TURTLE_INT_MOVE_DONE);
    }

    // get the integer version of the position
    vec2p true_pos  = turtle_position;
    vec2p true_targ = turtle_subtarget_pos;

    // Draw onto background only if the turtle actually moves
    if(pen_down && true_pos != true_targ) {
        line(base_image, true_pos, true_targ, pen_size, pen_color, NULL);
    }

    turtle_position = turtle_subtarget_pos;
}

void normalize_angle() {
    // Normalize turtle_heading to [0, 360)
    while(turtle_heading >= 360.0f) turtle_heading -= 360.0f;
    while(turtle_heading < 0.0f) turtle_heading += 360.0f;
    return;
}

void angle_to_rotate() {
    vec2 dir = turtle_target_position - turtle_position;
    float angle_adgustment = 270.0f; // this is to orient the image correctly
    float target_angle = atan2(dir.y, dir.x) * (180.0f / M_PI) - angle_adgustment;  // Angle to target in degrees

    // Calculate the difference between target angle and current heading
    float angle_diff = target_angle - turtle_heading;

    // Normalize to [-180, 180] range for shortest rotation
    while(angle_diff > 180.0f) angle_diff -= 360.0f;
    while(angle_diff < -180.0f) angle_diff += 360.0f;

    // Now turtle_heading should be updated by angle_diff
    turtle_heading += angle_diff;
    normalize_angle();
}

void rotate_turtle(Tigr* dest, Tigr* turtle, float cx, float cy, float angleDegrees) {
    float angle = angleDegrees * (M_PI / 180.0f);
    float cosA = cos(angle);
    float sinA = sin(angle);

    int w = turtle->w;
    int h = turtle->h;
    int halfW = w / 2;
    int halfH = h / 2;

    // Calculate bounding box for rotated image
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

    // Iterate through destination pixels (reverse mapping)
    for(int dy = (int)minY; dy <= (int)maxY; dy++) {
        for(int dx = (int)minX; dx <= (int)maxX; dx++) {
            // Reverse rotate to find source pixel
            float srcX = dx * cosA + dy * sinA;
            float srcY = -dx * sinA + dy * cosA;

            // Add offset to center
            int sx = (int)(srcX + halfW + 0.5f);
            int sy = (int)(srcY + halfH + 0.5f);

            // Check bounds
            if(sx >= 0 && sx < w && sy >= 0 && sy < h) {
                TPixel p = tigrGet(turtle, sx, sy);

                if(p.a > 0) {
                    tigrPlot(dest, (int)(cx + dx), (int)(cy + dy), p);
                }
            }
        }
    }
}

vec2f detect() {
    // Calculate position 15 pixels ahead of turtle based on heading
    float angle_rad = (turtle_heading + 270) * (M_PI / 180.0f);
    float detect_x = turtle_position.x + detect_ahead_distance * cos(angle_rad);
    float detect_y = turtle_position.y + detect_ahead_distance * sin(angle_rad);
    
    // Clamp to image bounds
    int detect_xi = (int)detect_x;
    int detect_yi = (int)detect_y;
    if(detect_xi < 0) detect_xi = 0;
    if(detect_yi < 0) detect_yi = 0;
    if(detect_xi >= base_image->w) detect_xi = base_image->w - 1;
    if(detect_yi >= base_image->h) detect_yi = base_image->h - 1;

    return vec2(detect_xi, detect_yi);
}

uint16_t colorTo16b(TPixel c24b) {
    uint16_t r = (c24b.r >> 3) & 0x1F;  // 5 bits for red
    uint16_t g = (c24b.g >> 2) & 0x3F;  // 6 bits for green
    uint16_t b = (c24b.b >> 3) & 0x1F;  // 5 bits for blue
    return (r << 11) | (g << 5) | b;
}

TPixel colorTo24b(uint16_t c16b) {
    unsigned char r = (c16b >> 11) & 0x1F; // 5 bits for red
    unsigned char g = (c16b >>  5) & 0x3F; // 6 bits for green
    unsigned char b = c16b & 0x1F; // 5 bits for blue
    r <<= 3;
    g <<= 2;
    b <<= 3;
    return {r,g,b,255};
}


void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    switch(addr) {
        case TURTLE_X:
            data->u = turtle_position.x;
            break;
        case TURTLE_Y:
            data->u = turtle_position.y;
            break;
        case TURTLE_ANGLE:
            data->u = turtle_heading + 270;
            break;
        case DETECT:
            // Store a 16bit version of the pixel color in the register
            data->u = colorTo16b(tigrGet(base_image,
                (int)turtle_position.x,
                (int)turtle_position.y)
            );
            break;
        case DETECT_AHEAD: {
            vec2f detect_pos = detect();
            data->u = colorTo16b(tigrGet(base_image, detect_pos.x, detect_pos.y));
            break;
        }
        case SET_X:
            data->u = turtle_target_position.x;
            break;
        case SET_Y:
            data->u = turtle_target_position.y;
            break;
    }
    return;
}

void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    switch(addr) {
        case GOTO_XY:
            move_turtle_target = true;
            break;
        case MOVE:
            move_turtle_forward = data.u;
            break;
        case FACE_XY:
            angle_to_rotate();
            rotate_turtle(window, turtle_image, turtle_position.x, turtle_position.y, turtle_heading);
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
        case SET_X:
            turtle_target_position.x = data.u;
            break;
        case SET_Y:
            turtle_target_position.y = data.u;
            break;
        case TURTLE_ANGLE:
            turtle_heading = data.u;
            normalize_angle();
            break;
    }
    return;
}
