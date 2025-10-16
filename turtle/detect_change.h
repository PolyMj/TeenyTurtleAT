#include <queue>
#include "draw.hpp"

typedef struct ColorChange_data {
    uint16_t new_color;
    vec2p old_position; // Turtle position prior to color change

    inline bool operator==(const ColorChange_data& o) const {
        return new_color == o.new_color;
    };

    inline bool operator==(const uint16_t& other_color) const {
        return new_color == other_color;
    };
} ColorChange_t;

extern queue<ColorChange_t> color_changes;

void getColorChanges(Tigr* img, vec2p start, vec2p end);


// These are just some quick comments so I can figure out how to structure thigns
// The problem is right now our movement is "move first, ask questions later",
// which breaks down when we need to ask the assembler if the turtle can move somewhere
// (e.g. assembler should prevent turtle from going through a "wall")

// The idea is to calculate movement path (and all color changes on that path),
// raise the appropriate interrupts (allowing the assmebler to "cancel" the movement),
// then proceed with the movement (if not cancelled). 


// tpf = 1,000,000 / framerate // Ticks per frame
// while(running)
    // tny_clock
    // --ticks
    
    // if (need_check_color_changes)
        // if (cc_not_raised)
            // raise_cc_int
        // else
            // Do nothing (keep clocking untill we return from the interrupt)
        // continue
    
    // if (ticks < 0)
        // doMove
        // draw
        // tigrUpdate
        // calcMove
        // ticks = tpf

// def calcMove:
    // calculate movement as start and end position
    // calculate all color changes between start and end

// def doMove:
    // if pen down, draw line
    // move turtle to end position

/*
 * The reason that doMove and calcMove are separate is so that
 * movement can be interrupted in an interrupt, e.g. if you hit
 * a wall halfway through your movement path, and the pen is down,
 * you should not pass through the wall (drawing over it), then check
 * if you hit a wall. 
 * I.e., we calculate movement, check with the assembler to see if 
 * the movement is valid (cancelling if not), then proceed with the
 * true movement path if needed. 
 */