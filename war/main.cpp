#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
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

int   windowWidth = 640;
int   windowHeight = 500;

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
};

struct Star : Unit {

};

struct Square : Unit {

};

struct Circle : Unit {

};

struct Triangle : Unit {

};


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

    vector<Star> star_list;
    vector<Square> square_list;
    vector<Circle> circle_list;
    vector<Triangle> triangle_list;
   
    star_list.reserve(2);
    square_list.reserve(player1_unit_counts[0] + player2_unit_counts[0]);
    circle_list.reserve(player1_unit_counts[1] + player2_unit_counts[1]);
    triangle_list.reserve(player1_unit_counts[2] + player2_unit_counts[2]);


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


    int cycles_until_frame = 0;
    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {

        // Clock all teenyat instances
        for (auto &star : star_list) {
            tny_clock(&star.t);
        }
        
        for (auto &square : square_list) {
            tny_clock(&square.t);
        }
        
        for (auto &circle : circle_list) {
            tny_clock(&circle.t);
        }
        
        for (auto &triangle : triangle_list) {
            tny_clock(&triangle.t);
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
    for (int i = 0; i < count; i++) {
        T new_unit;

        // Set basic unit fields
        new_unit.type = unit_type_id;
        new_unit.player = player_number;
        // new_unit.health = 100.0f;
        // new_unit.damage = 10.0f;
        // new_unit.speed = 1.0f;
        // new_unit.position = {0.0f, 0.0f};

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

    }
    return;
}

void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    Unit *unit = nullptr;
    if (t->ex_data) {
        unit = static_cast<Unit*>(t->ex_data);
    }

    switch(addr) {

    }
    return;
}