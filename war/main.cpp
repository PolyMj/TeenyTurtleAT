#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include "vec.hpp"
#include "tigr.h"
#include "teenyat.h"

using namespace std;

#define MOVE_FORWARD    0xD000
#define MOVE_BACKWARD   0xD001
#define MOVE_LEFT       0xD002
#define MOVE_RIGHT      0xD003
#define DETECT_FORWARD  0xE000
#define DETECT_BACKWARD 0xE001
#define DETECT_LEFT     0xE002
#define DETECT_RIGHT    0xE003
#define CURRENT_HEALTH  0xF000
#define ALVIE_UNITS     0xF001
#define DEAD_UNITS      0xF002

#define DAMANGE_INTERRUPT   TNY_XINT0
#define STAR_INTERRUPT      TNY_XINT1

enum UnitType {
    UNIT_STAR,
    UNIT_SQUARE,
    UNIT_CIRCLE,
    UNIT_TRIANGLE
};

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);
string get_file_path(string folder_name, string file_name);
void get_counts(int points[], string file_path);

template<typename T>
void create_units(vector<T> &unit_list, const string &bin_path,
                  int player_number, UnitType unit_type_id, int count,
                  TNY_READ_FROM_BUS_FNPTR bus_read,
                  TNY_WRITE_TO_BUS_FNPTR bus_write);

Tigr* window;
Tigr* base_image;
Tigr* red_star_image;
Tigr* blue_star_image;
Tigr* red_triangle_image;
Tigr* blue_triangle_image;

const int   windowWidth = 640;
const int   windowHeight = 500;

const int FPS = 60;
const int cycles_per_frame = 1e6 / FPS;

struct Unit {
    teenyat t;

    UnitType type;

    int player;
    float health;
    float damage;
    float speed;

    vec2f position;

    TPixel color;
    Tigr*  texture;

    int size;
};

struct Star : Unit {

};

struct Square : Unit {

};

struct Circle : Unit {

};

struct Triangle : Unit {

};

vector<Star> star_list;
vector<Square> square_list;
vector<Circle> circle_list;
vector<Triangle> triangle_list;


inline uint16_t healthToInt(const float &h) { return round(h); }
inline float healthToFloat(const uint16_t &h) { return float(h); }

template<typename T>
void countUnits(const vector<T> &units, uint16_t *total, uint16_t *alive, uint16_t *dead) {
    for (const auto &u : units) {
        if (total) ++(*total);
        if (u.health > 0)
            if (alive) ++(*alive);
        else
            if (dead) ++(*dead);
    }
}

void countUnits(uint16_t *total, uint16_t *alive, uint16_t *dead) {
    countUnits(star_list, total, alive, dead);
    countUnits(square_list, total, alive, dead);
    countUnits(circle_list, total, alive, dead);
    countUnits(triangle_list, total, alive, dead);
}

void draw_unit(struct Unit* unit) {
    /* Assign unit color based on which player they are */
    switch(unit->type) {
        case UNIT_STAR:
        case UNIT_TRIANGLE:
            tigrBlitAlpha(base_image, unit->texture, unit->position.x - (unit->texture->w / 2), unit->position.y - (unit->texture->h / 2),
                          0, 0, unit->texture->w, unit->texture->h, 1.0f);
            break;
        case UNIT_CIRCLE:
            tigrFillCircle(base_image, unit->position.x, unit->position.y, (int)unit->size/2, unit->color);
            break;
        case UNIT_SQUARE:
            /* draw rectangles at center of x,y */
            tigrFillRect(base_image, unit->position.x-(unit->size/2), unit->position.y-(unit->size/2), unit->size, unit->size, unit->color);
            break;
        default:
            break;
    }
    return;
}

int main(int argc, char *argv[]) {

    if(argc < 2 || argc > 3) {
        std::cout << "usage: war <player1 folder name> <player2 folder name>" << std::endl;
        return EXIT_FAILURE;
    }

    string player1_folder_name = argv[1];
    string player2_folder_name = argv[2];

    string player1_points_path = get_file_path(player1_folder_name, "points.txt");
    string player1_star_path = get_file_path(player1_folder_name, "star.bin");
    string player1_square_path = get_file_path(player1_folder_name, "square.bin");
    string player1_circle_path = get_file_path(player1_folder_name, "circle.bin");
    string player1_triangle_path = get_file_path(player1_folder_name, "triangle.bin");

    string player2_points_path = get_file_path(player2_folder_name, "points.txt");
    string player2_star_path = get_file_path(player2_folder_name, "star.bin");
    string player2_square_path = get_file_path(player2_folder_name, "square.bin");
    string player2_circle_path = get_file_path(player2_folder_name, "circle.bin");
    string player2_triangle_path = get_file_path(player2_folder_name, "triangle.bin");

    int player1_unit_counts[3];
    int player2_unit_counts[3];

    get_counts(player1_unit_counts, player1_points_path);
    get_counts(player2_unit_counts, player2_points_path);

    star_list.reserve(2);
    square_list.reserve(player1_unit_counts[0] + player2_unit_counts[0]);
    circle_list.reserve(player1_unit_counts[1] + player2_unit_counts[1]);
    triangle_list.reserve(player1_unit_counts[2] + player2_unit_counts[2]);

    red_triangle_image = tigrLoadImage("img/Red_Triangle.png");
    if(!red_triangle_image) tigrError(0, "Could not img/Red_Triangle.png");

    blue_triangle_image = tigrLoadImage("img/Blue_Triangle.png");
    if(!blue_triangle_image) tigrError(0, "Could not img/Blue_Triangle.png");

    red_star_image = tigrLoadImage("img/Red_Star.png");
    if(!red_star_image) tigrError(0, "Could not img/Red_Star.png");

    blue_star_image = tigrLoadImage("img/Blue_Star.png");
    if(!blue_star_image) tigrError(0, "Could not img/Blue_Star.png");

    // Player 1 units
    create_units(star_list, player1_star_path, 1, UNIT_STAR,
                 1,
                 bus_read, bus_write);


    create_units(square_list, player1_square_path, 1, UNIT_SQUARE,
                 player1_unit_counts[0],
                 bus_read, bus_write);

    create_units(circle_list, player1_circle_path, 1, UNIT_CIRCLE,
                 player1_unit_counts[1],
                 bus_read, bus_write);

    create_units(triangle_list, player1_triangle_path, 1, UNIT_TRIANGLE,
                 player1_unit_counts[2],
                 bus_read, bus_write);

    // Player 2 units
    create_units(star_list, player2_star_path, 2, UNIT_STAR,
                1,
                bus_read, bus_write);

    create_units(square_list, player2_square_path, 2, UNIT_SQUARE,
                 player2_unit_counts[0],
                 bus_read, bus_write);

    create_units(circle_list, player2_circle_path, 2, UNIT_CIRCLE,
                 player2_unit_counts[1],
                 bus_read, bus_write);

    create_units(triangle_list, player2_triangle_path, 2, UNIT_TRIANGLE,
                 player2_unit_counts[2], bus_read, bus_write);

    window = tigrWindow(windowWidth, windowHeight, "Teeny WAR", TIGR_FIXED);
    base_image = tigrBitmap(window->w, window->h);
    tigrClear(window, tigrRGB(255, 255, 255));
    tigrClear(base_image, tigrRGB(255, 255, 255));

    vector<Star*> star_random_order;
    vector<Triangle*> triangle_random_order;
    vector<Circle*> circle_random_order;    
    vector<Square*> square_random_order;

    star_random_order.reserve(star_list.size());
    triangle_random_order.reserve(triangle_list.size());
    circle_random_order.reserve(circle_list.size());
    square_random_order.reserve(square_list.size());

    for (auto &star : star_list) {
        star_random_order.push_back(&star);
    }

    for (auto &triangle : triangle_list) {
        triangle_random_order.push_back(&triangle);
    }

    for (auto &circle : circle_list) {
        circle_random_order.push_back(&circle);
    }

    for (auto &square : square_list) {
        square_random_order.push_back(&square);
    }

    random_device rd;
    mt19937 rng(rd());

    int cycles_until_frame = 0;
    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {

        // Randomize order
        shuffle(star_random_order.begin(), star_random_order.end(), rng);
        shuffle(triangle_random_order.begin(), triangle_random_order.end(), rng);
        shuffle(circle_random_order.begin(), circle_random_order.end(), rng);
        shuffle(square_random_order.begin(), square_random_order.end(), rng);

        // Clock all teenyat instances
        for (auto &star : star_random_order) {
            tny_clock(&star->t);
            draw_unit(star);
        }

        for (auto &triangle : triangle_random_order) {
            tny_clock(&triangle->t);
            draw_unit(triangle);
        }

        for (auto &circle : circle_random_order) {
            tny_clock(&circle->t);
            draw_unit(circle);
        }

        for (auto &square : square_random_order) {
            tny_clock(&square->t);
            draw_unit(square);
        }

        --cycles_until_frame;
        if(cycles_until_frame < 0) {
            /* Move base_image ontop of our window */
            tigrBlit(window, base_image, 0, 0, 0, 0, base_image->w, base_image->h);

            tigrUpdate(window);
            cycles_until_frame = cycles_per_frame;
        }
    }

    tigrFree(base_image);
    tigrFree(window);

    return EXIT_SUCCESS;

}

string get_file_path(string folder_name, string file_name) {
    string full_path = folder_name;
    if (full_path.back() != '/' && full_path.back() != '\\') {
        #ifdef _WIN32
            full_path += '\\';
        #else
            full_path += '/';
        #endif
    }
    full_path += file_name;

    return full_path;
}

void get_counts(int points[], string file_path) {
    ifstream inputFile(file_path);

    if (inputFile.is_open()) {
        string line;
        if (getline(inputFile, line)) {
            istringstream iss(line);
            iss >> points[0] >> points[1] >> points[2];
        }
        inputFile.close();
    } else {
        std::cerr << "Failed to open file: " << file_path << std::endl;
    }
}

template<typename T>
void create_units(vector<T> &unit_list, const string &bin_path,
                  int player_number, UnitType unit_type_id, int count,
                  TNY_READ_FROM_BUS_FNPTR bus_read,
                  TNY_WRITE_TO_BUS_FNPTR bus_write) {

    int start_heights[4] = {45, 100, 120, 140};
    const int start_width_offset = (windowWidth / 2) - 50;
    for (int i = 0; i < count; i++) {
        T new_unit;

        // Set basic unit fields
        new_unit.type = unit_type_id;
        new_unit.player = player_number;
        new_unit.color = (player_number == 1) ? TPixel{237,28,36,255} : TPixel{0,162,232,255};
        new_unit.size = 15;

        int start_y = start_heights[unit_type_id];
        if(player_number == 1) start_y = windowHeight - start_y;

        new_unit.position = {start_width_offset + (25 *  i), start_y};

        if(unit_type_id == UNIT_STAR) {
            new_unit.texture    = (player_number == 1) ? red_star_image : blue_star_image;
            new_unit.position.x = windowWidth / 2;
        }

        if(unit_type_id == UNIT_TRIANGLE) {
            new_unit.texture = (player_number == 1) ? red_triangle_image : blue_triangle_image;
        }

        // new_unit.health = 100.0f;
        // new_unit.damage = 10.0f;
        // new_unit.speed = 1.0f;

        // Initialize the teenyat instance from the .bin
        FILE* bin_file = fopen(bin_path.c_str(), "rb");
        if (bin_file != NULL) {
            if (!tny_init_from_file(&new_unit.t, bin_file, bus_read, bus_write)) {
                std::cout << "tny_init_from_file failed for: " << bin_path << std::endl;
                fclose(bin_file);
                continue;
            }
            fclose(bin_file);
        } else {
            std::cout << "Failed to open bin file (invalid path?): " << bin_path << std::endl;
            continue;
        }

        unit_list.push_back(new_unit);

        // Now that the unit is in the vector, point ex_data to the stored element
        unit_list.back().t.ex_data = static_cast<void*>(&unit_list.back());
    }
    return;
}

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    // ex_data points to the stored unit inside the vector
    Unit *unit = nullptr;
    if (t->ex_data) {
        unit = static_cast<Unit*>(t->ex_data);
    }

    switch(addr) {
        case CURRENT_HEALTH:
            data->u = healthToInt(unit->health);
            break;
        case ALVIE_UNITS:
            data->u = 0;
            countUnits(NULL, &(data->u), NULL);
            break;
        case DEAD_UNITS:
            data->u = 0;
            countUnits(NULL, &(data->u), NULL);
            break;
    }
    return;
}

void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    Unit *unit = nullptr;
    if (t->ex_data) {
        unit = static_cast<Unit*>(t->ex_data);
    }

    switch(addr) {
        case MOVE_FORWARD:
            unit->position.y += unit->speed;
            break;
        case MOVE_BACKWARD:
            unit->position.y -= unit->speed;
            break;
        case MOVE_LEFT:
            unit->position.x -= unit->speed;
            break;
        case MOVE_RIGHT:
            unit->position.x += unit->speed;
            break;

    }
    return;
}
