#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
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

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);
void move_to_target(teenyat *t);

Tigr* window;
Tigr* base_image;
int   windowWidth = 640;
int   windowHeight = 500;

vec2      turtle_position           = vec2(320.0f,250.0f);
int       turtle_size               = 5;
vec2      turtle_target_position    = vec2(0.0f,0.0f);
double    turtle_heading            = 0.0f;
TPixel    pen_color                 = {0,0,0,0};
int       pen_size                  = 5;
bool      pen_down                  = false;
bool      erase_mode                = false;
bool      move_turtle               = true;

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

    if(argc == 3) {
        const char* img_name = argv[2];
        base_image = tigrLoadImage(img_name);
        if(!base_image) tigrError(0, "Could not load image file");
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

    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {
        tny_clock(&t);

        /* Game ticks every 60 frames */
        if(!frame_number) {
          /* Move base_image ontop of our window */
          tigrBlit(window, base_image, 0, 0, 0, 0, base_image->w, base_image->h);

          fillCircle(base_image, turtle_position, turtle_size, {255,0,0,255}, NULL);

          if(move_turtle) {
            move_to_target(&t);
          }

          tigrUpdate(window);
          // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        frame_number = (frame_number + 1) % 60;
    }

    tigrFree(window);
    return EXIT_SUCCESS;
}

void move_to_target(teenyat *t) {
    vec2f dir = turtle_target_position;
    dir = (dir - turtle_position).normalize();
    turtle_position += dir;
    float distX = turtle_target_position.x - turtle_position.x;
    float distY = turtle_target_position.y - turtle_position.y;
    float distance = std::sqrt( (distX*distX) + (distY*distY) );
    if(distance < turtle_size + turtle_size) {
        turtle_position = turtle_target_position;
        move_turtle = false;
        tny_external_interrupt(t, 0);
    }
}

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    switch(addr) {
    case TURTLE_X:
        data->u = turtle_position.x;
        break;
    case TURTLE_Y:
        data->u = turtle_position.y;
        break;
    return;
    }
    

    return;
}

void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    switch(addr){
        case GOTO_XY: {
          move_turtle = true;
        }
        break;
    }
    return;
}

