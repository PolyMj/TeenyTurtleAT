#include <queue>
#include "tigr.h"
#include "vec.hpp"
using namespace std;

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

extern queue<ColorChange> change_queue;

void getColorChanges(Tigr *img, vec2p start, vec2p end, uint16_t r);