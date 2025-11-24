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

#define UPDATES_UNTIL_DAMAGE 20

enum UnitType {
    UNIT_STAR,
    UNIT_SQUARE,
    UNIT_CIRCLE,
    UNIT_TRIANGLE
};

struct Unit {
    teenyat t;

    UnitType type;

    int player = 0;
    float health = 100;
    float damage = 0;
    float speed = 1;

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

struct RaycastHit {
    Unit* unit;
    float distance;
};

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);
string get_file_path(string folder_name, string file_name);
void get_counts(int points[], string file_path);
bool rayCircleIntersect(vec2f origin, vec2f direction, vec2f circleCenter, float radius, float maxDistance, float& outDistance);
RaycastHit performRaycast(Unit* sourceUnit, vec2f direction, float maxDistance);
uint16_t encodeDetectionResult(Unit* sourceUnit, Unit* detectedUnit);
vec2f getDirectionVector(Unit* unit, int direction);
void draw_detection_rays(Unit* unit, bool show_rays);


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
const int cycles_per_frame = 1e3 / FPS;

float detect_range = 100.0f;

bool damage_possible = false;

vector<Star> star_list;
vector<Square> square_list;
vector<Circle> circle_list;
vector<Triangle> triangle_list;
vector<Unit*> unit_list;

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
            //draw_detection_rays(unit, true);
            break;
        case UNIT_CIRCLE:
            tigrFillCircle(base_image, unit->position.x, unit->position.y, (int)unit->size/2, unit->color);
            //draw_detection_rays(unit, true);
            break;
        case UNIT_SQUARE:
            /* draw rectangles at center of x,y */
            tigrFillRect(base_image, unit->position.x-(unit->size/2), unit->position.y-(unit->size/2), unit->size, unit->size, unit->color);
            //draw_detection_rays(unit, true);
            break;
        default:
            break;
    }

    tigrCircle(base_image, unit->position.x, unit->position.y, (int)unit->size/1.5, unit->color);
    if(damage_possible) {
        tigrCircle(base_image, unit->position.x, unit->position.y, (int)unit->size, unit->color);
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
        unit_list.push_back(&star);
    }

    for (auto &triangle : triangle_list) {
        triangle_random_order.push_back(&triangle);
        unit_list.push_back(&triangle);
    }

    for (auto &circle : circle_list) {
        circle_random_order.push_back(&circle);
        unit_list.push_back(&circle);
    }

    for (auto &square : square_list) {
        square_random_order.push_back(&square);
        unit_list.push_back(&square);
    }

    random_device rd;
    mt19937 rng(rd());

    int cycles_until_frame = 0;
    int frames_until_damage_tick = UPDATES_UNTIL_DAMAGE;
    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {

        tigrClear(base_image, tigrRGB(255, 255, 255)); 

        // Randomize order
        shuffle(star_random_order.begin(), star_random_order.end(), rng);
        shuffle(triangle_random_order.begin(), triangle_random_order.end(), rng);
        shuffle(circle_random_order.begin(), circle_random_order.end(), rng);
        shuffle(square_random_order.begin(), square_random_order.end(), rng);

        // Clock all teenyat instances
        for (auto &star : star_random_order) {
            tny_clock(&star->t);
        }

        for (auto &triangle : triangle_random_order) {
            tny_clock(&triangle->t);
        }

        for (auto &circle : circle_random_order) {
            tny_clock(&circle->t);
        }

        for (auto &square : square_random_order) {
            tny_clock(&square->t);
        }

        --cycles_until_frame;
        if(cycles_until_frame < 0) {
            tigrClear(base_image, {255,255,255,255});

            frames_until_damage_tick--;
            if(frames_until_damage_tick < 0) {
                frames_until_damage_tick = UPDATES_UNTIL_DAMAGE;
                damage_possible = true;
            }

            for (auto &star : star_list) draw_unit(&star);
            for (auto &square : square_list) draw_unit(&square);
            for (auto &circle : circle_list) draw_unit(&circle);
            for (auto &triangle : triangle_list) draw_unit(&triangle);

            /* Move base_image ontop of our window */
            tigrBlit(window, base_image, 0, 0, 0, 0, base_image->w, base_image->h);

            tigrUpdate(window);
            cycles_until_frame = cycles_per_frame;
            damage_possible = false;

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
            new_unit.size = 30;
        }

        if(unit_type_id == UNIT_TRIANGLE) {
            new_unit.texture = (player_number == 1) ? red_triangle_image : blue_triangle_image;
            new_unit.size = 13;
        }

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
            countUnits(NULL, NULL, &(data->u));
            break;
        case DETECT_FORWARD: {
            vec2f unit_direction = getDirectionVector(unit, 0);
            RaycastHit hit = performRaycast(unit, unit_direction, detect_range); // 200 pixel range
            data->u = encodeDetectionResult(unit, hit.unit);
            break;
        }
        case DETECT_BACKWARD: {
            vec2f unit_direction = getDirectionVector(unit, 1);
            RaycastHit hit = performRaycast(unit, unit_direction, detect_range);
            data->u = encodeDetectionResult(unit, hit.unit);
            break;
        }
        case DETECT_LEFT: {
            vec2f unit_direction = getDirectionVector(unit, 2);
            RaycastHit hit = performRaycast(unit, unit_direction, detect_range);
            data->u = encodeDetectionResult(unit, hit.unit);
            break;
        }
        case DETECT_RIGHT: {
            vec2f unit_direction = getDirectionVector(unit, 3);
            RaycastHit hit = performRaycast(unit, unit_direction, detect_range);
            data->u = encodeDetectionResult(unit, hit.unit);
            break;
        }
    }
    return;
}

bool check_collision(Unit* unit) {
    for(const Unit *other : unit_list) {
        if (other == unit) continue;
        if (other->health <= 0) continue;
        
        float distance = (unit->position - other->position).length();
        if(distance <= (unit->size/1.5) + (unit->size/1.5)) {
            return true;
         }
    }
    return false;
}

void update_unit(teenyat* t, bool update_x, float update_value) {
    Unit* unit = static_cast<Unit*>(t->ex_data);
    if(update_x) {
        unit->position.x += update_value;
        if(check_collision(unit)) {
            unit->position.x -= update_value;
        }
    }else{
        unit->position.y += update_value;
        if(check_collision(unit)) {
            unit->position.y -= update_value;
        }
    }
}

void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    Unit *unit = nullptr;
    if (t->ex_data) {
        unit = static_cast<Unit*>(t->ex_data);
    }

    switch(addr) {
        case MOVE_FORWARD:
            if (unit->player == 2) {
                update_unit(t,false, unit->speed);
            }
            else {
                update_unit(t,false, -unit->speed);
            }
            break;
        case MOVE_BACKWARD:
            if (unit->player == 2) {
                update_unit(t,false, -unit->speed);
            }
            else {
                update_unit(t,false, unit->speed);
            }
            break;
        case MOVE_LEFT:
            if (unit->player == 2) {
                update_unit(t,true, unit->speed);
            }
            else {
                update_unit(t,true, -unit->speed);
            }
            break;
        case MOVE_RIGHT:
            if (unit->player == 2) {
                update_unit(t,true, -unit->speed);
            }
            else {
                update_unit(t,true, unit->speed);
            }
            break;
    }
    return;
}

// Ray-Circle intersection function
bool rayCircleIntersect(vec2f origin, vec2f direction, vec2f circleCenter, float radius, float maxDistance, float& outDistance) {
    vec2f toCircle = circleCenter - origin;
    
    // Project toCircle onto ray direction
    float projection = toCircle * direction; // dot product
    
    // If projection is negative, circle is behind ray
    if (projection < 0) return false;
    
    // Find closest point on ray to circle center
    vec2f closestPoint = origin + (projection * direction);
    
    // Distance from circle center to closest point
    vec2f diff = circleCenter - closestPoint;
    float distSq = diff * diff; // dot product with itself
    
    float radiusSq = radius * radius;
    
    if (distSq <= radiusSq && projection <= maxDistance) {
        // Calculate actual intersection distance
        float offset = sqrt(radiusSq - distSq);
        outDistance = projection - offset;
        return outDistance >= 0 && outDistance <= maxDistance;
    }
    return false;
}

// Perform raycast and return the closest hit
RaycastHit performRaycast(Unit* sourceUnit, vec2f direction, float maxDistance) {
    RaycastHit closestHit = {nullptr, maxDistance + 1.0f};
    
    vec2f rayOrigin = sourceUnit->position;
    
    for (Unit* other : unit_list) {
        // Skip self
        if (other == sourceUnit) continue;
        
        // Skip dead units
        if (other->health <= 0) continue;
        
        float hitDistance;
        float detectionRadius = other->size / 1.5f; // Match the collision radius
        
        if (rayCircleIntersect(rayOrigin, direction, other->position, detectionRadius, maxDistance, hitDistance)) {
            if (hitDistance < closestHit.distance) {
                closestHit.unit = other;
                closestHit.distance = hitDistance;
            }
        }
    }
    
    return closestHit;
}

// Convert Unit type and team info to the encoded format
uint16_t encodeDetectionResult(Unit* sourceUnit, Unit* detectedUnit) {
    if (detectedUnit == nullptr) {
        return 0; // Nothing detected
    }
    
    uint16_t result = 0;
    
    // Encode unit type (bits 0-2)
    switch(detectedUnit->type) {
        case UNIT_SQUARE:   result = 1; break;
        case UNIT_CIRCLE:   result = 2; break;
        case UNIT_TRIANGLE: result = 3; break;
        case UNIT_STAR:     result = 4; break;
    }
    
    // Set most significant bit if it's a foe (different player)
    if (detectedUnit->player != sourceUnit->player) {
        result |= 0x8000; // Set bit 15 (most significant bit)
    }
    
    return result;
}

// Get direction vector based on unit's player and direction
vec2f getDirectionVector(Unit* unit, int direction) {
    // Player 1 faces "up" (negative Y)
    // Player 2 faces "down" (positive Y)
    // These directions are relative to the unit's facing
    
    vec2f dir;
    
    if (unit->player == 1) {
        switch(direction) {
            case 0: dir = vec2f(0, -1);  break; // Forward = up
            case 1: dir = vec2f(0, 1);   break; // Backward = down
            case 2: dir = vec2f(-1, 0);  break; // Left
            case 3: dir = vec2f(1, 0);   break; // Right
        }
    } else { // player == 2
        switch(direction) {
            case 0: dir = vec2f(0, 1);   break; // Forward = down
            case 1: dir = vec2f(0, -1);  break; // Backward = up
            case 2: dir = vec2f(1, 0);   break; // Left (flipped for player 2)
            case 3: dir = vec2f(-1, 0);  break; // Right (flipped for player 2)
        }
    }
    
    return dir;
}

// Testing function to draw detection rays
void draw_detection_rays(Unit* unit, bool show_rays) {
    if (!show_rays) return;
    
    for (int i = 0; i < 4; i++) {
        vec2f dir = getDirectionVector(unit, i);
        RaycastHit hit = performRaycast(unit, dir, detect_range);
        
        vec2f endPos;
        if (hit.unit) {
            endPos = unit->position + (hit.distance * dir);
            // Draw red line if hit
            tigrLine(base_image, unit->position.x, unit->position.y, 
                    endPos.x, endPos.y, tigrRGB(255, 0, 0));
        } else {
            endPos = unit->position + (detect_range * dir);
            // Draw gray line if no hit
            tigrLine(base_image, unit->position.x, unit->position.y, 
                    endPos.x, endPos.y, tigrRGB(128, 128, 128));
        }
    }
}