////////////////////////////////////////////////////////////////////////////////
//
// Snake for ZX Spectrum 48K
//
// (C) 2019, 2020 David Crespo - https://github.com/dcrespo3d/zx-spectrum-snake
//
////////////////////////////////////////////////////////////////////////////////
#include "params0.h"
#include "screen.h"
#include "keyb.h"
#include "colors.h"
#include "frame.h"
#include "menu.h"
#include "tile.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>

// color definitions
#define COLOR_BG         ZX_BLK_BLK
#define COLOR_FRAME      ZX_BLK_BLU
#define COLOR_FRAME_OVER ZX_BLK_RED
#define PAPER_MSG        ZX_BLK
#define INK_MSG          ZX_CYA
#define COLOR_MSG_OVER   ZX_BLK_YEL
#define COLOR_SNAKE      ZX_WHI_RED
#define COLOR_FRUIT      ZX_BLK_GRE
#define PAPER_OVER       ZX_YEL
#define INK_OVER         ZX_BLK

// assembler inline shortcut for HALT
#define HALT {asm("HALT");}

// directions
#define DIRN 0x00   // no direction
#define DIRB 0x08   // bottom
#define DIRL 0x04   // left
#define DIRR 0x02   // right
#define DIRT 0x01   // top

// board limits
char min_x = 1;
char min_y = 2;
char max_x = 30;
char max_y = 22;

// fruit coordinates
char fruit_x;
char fruit_y;

// current and previous coordinates of snake head
char curr_x;
char curr_y;
char prev_x;
char prev_y;

// fruit graphic is defined elsewhere
extern ubyte fruit_glyph;

// game running flag, 1 for game in progress, 0 for game over
char game_running;

// pause flag, 1 for paused.
char paused = 0;
// pause lock prevents continous 0 -> 1 -> 0 -> 1 cycle when P key pressed.
char pauselock = 0;

// previous, current and next directions (values are DIR*)
char prev_dir;
char curr_dir;
char next_dir;

// score points
uword score_counter;

// index into arrays pos_x_grid, pos_y_grid, dir_grid for snake head and snake tail
uword player_head_idx;
uword player_tail_idx;

// 4 buffers of 1Kbyte each (overkill)
// these buffers are not declared as char[], but as char*
// and have been assigned absolute hardwired addresses.
// By this way, generated TAPE file does not contain
// 4 kb of empty space, and load time in a real speccy is faster.
uword buf_size   =        0x0400;
uword buf_mask   =        0x03FF;
char* pos_x_grid = (char*)0xB000;   // stored snake x position
char* pos_y_grid = (char*)0xB400;   // stored snake y position
char* dir_grid   = (char*)0xB800;   // stored snake direction
char* used_grid  = (char*)0xBC00;   // mask of used (1) / non used (0) coordinates

// speed is controlled allowing the snake to move only each N frames.

// this variable takes value from skill_halt_table
char halts_per_frame;   
// this variable gets assigned value from halts_per_frame,
// and then it is decremented every frame,
// when it reachs zero, snake is moved and back to begin.
char halt_counter;      


// table of halts for skill level
//                             0   1   2   3   4   5   6   7   8   9
char skill_halt_table[10] = { 40, 25, 20, 15, 12, 10,  7,  4,  3,  2 };

// top screen message
//                         1         2         3         4         5         6
//               0123456789012345678901234567890123456789012345678901234567890123
char* top_message = "Level X  |  WASD to move  |   P to pause   |  Score :";

// game over message
//                           1         2         3         4         5         6
//                 0123456789012345678901234567890123456789012345678901234567890123
char* gover_msg =    "  GAME OVER  |  press ENTER to play again  |  M for menu  ";

// some forward declarations
void restart_game();
void print_top_message();
void set_used_grid(char x, char y);
void put_rand_fruit();
void print_score();

// initialize stdlib random number generator using the R register
void init_rand()
{
    __asm
        LD   A, R
        LD  (_param0b), A
    __endasm
    srand(param0b);
    rand();
}

// clear the screen with default colors
void clear_screen()
{
    screen_border_set(ZX_BLK);
    screen_clear_bmap(0x00);
    screen_clear_attr(COLOR_BG);
}

// draw game over traits
void do_game_over()
{
    game_running = 0;
    draw_frame(0, 32, 1, 23, COLOR_FRAME_OVER);
    screen_clear_attr_blocks(COLOR_MSG_OVER, 0, 0, 32, 1);
    text_print(gover_msg, 3, 12, PAPER_OVER, INK_OVER);
}

// reset variables to initial state
void reset_vars()
{
    curr_x = 15;
    curr_y = 11;
    prev_x = 15;
    prev_y = 11;

    prev_dir = DIRN;
    curr_dir = DIRR;
    next_dir = DIRR;

    score_counter = 0;
    player_head_idx = 2;
    player_tail_idx = 0;

    game_running = 1;

    dir_grid[0] = DIRR;
    dir_grid[1] = DIRR;
    dir_grid[2] = DIRR;

    pos_y_grid[0] = curr_y;
    pos_y_grid[1] = curr_y;
    pos_y_grid[2] = curr_y;

    pos_x_grid[0] = curr_x-1;
    pos_x_grid[1] = curr_x;
    pos_x_grid[2] = curr_x;

    set_used_grid(curr_x-1, curr_y);
    set_used_grid(curr_x,   curr_y);
}

// functions for used grid
// it is a 32x24 grid containing 1 for used positions, 0 otherwise
void init_used_grid()
{
    for (short i = 0; i < 768; i++)
        used_grid[i] = 0;
}

void set_used_grid(char x, char y)
{
    used_grid[32*y + x] = 1;
}

void reset_used_grid(char x, char y)
{
    used_grid[32*y + x] = 0;
}

char is_used_grid(char x, char y)
{
    return used_grid[32*y + x];
}

// main frame processing function
void update_game()
{
    prev_x = curr_x;
    prev_y = curr_y;

    // calculate current position from current direction,
    // checking for boundary collisions.
    switch(curr_dir) {
        case DIRL:
            curr_x--;
            if (curr_x < min_x) { do_game_over(); return; }
            break;
        case DIRR:
            curr_x++;
            if (curr_x > max_x) { do_game_over(); return; }
            break;
        case DIRT:
            curr_y--;
            if (curr_y < min_y) { do_game_over(); return; }
            break;
        case DIRB:
            curr_y++;
            if (curr_y > max_y) { do_game_over(); return; }
            break;
    }

    // check for snake self-collisions
    if (is_used_grid(curr_x, curr_y)) {
        do_game_over();
        return;
    }

    // check for fruit eaten
    char on_fruit = (curr_x == fruit_x && curr_y == fruit_y) ? 1 : 0;
    if (on_fruit)
        put_rand_fruit();   // if eaten, put another fruit

    // logic for drawing snake head
    char tile = 0;
    switch(curr_dir) {
        case DIRR: tile = DIRL; break;
        case DIRL: tile = DIRR; break;
        case DIRT: tile = DIRB; break;
        case DIRB: tile = DIRT; break;
    }

    param0w = screen_get_bmap_addr(curr_x, curr_y);
    param0b = tile;
    param_draw_tile();
    screen_print_attr(curr_x, curr_y, COLOR_SNAKE);

    // mark current position as used
    set_used_grid(curr_x, curr_y);

    // logic for drawing tile after snake head
    switch(prev_dir) {
        case DIRR: tile = DIRL; break;
        case DIRL: tile = DIRR; break;
        case DIRT: tile = DIRB; break;
        case DIRB: tile = DIRT; break;
    }

    param0w = screen_get_bmap_addr(prev_x, prev_y);
    param0b = tile | curr_dir;
    param_draw_tile();
    screen_print_attr(prev_x, prev_y, COLOR_SNAKE);

    // put current position and direction in circular buffers
    pos_x_grid[player_head_idx] = curr_x;
    pos_y_grid[player_head_idx] = curr_y;
    dir_grid  [player_head_idx] = curr_dir;

    // logic for erasing tail position
    char last_x = pos_x_grid[player_tail_idx];
    char last_y = pos_y_grid[player_tail_idx];
    param0w = screen_get_bmap_addr(last_x, last_y);
    param_clear_tile();
    screen_print_attr(last_x, last_y, COLOR_BG);

    // mark last position as not used
    reset_used_grid(last_x, last_y);

    // logic for drawing last snake tile
    uword ptidx1 = (player_tail_idx + 1) & buf_mask;    // note buf_mask for circular buffer
    uword ptidx2 = (player_tail_idx + 2) & buf_mask;    // note buf_mask for circular buffer

    char prelast_x = pos_x_grid[ptidx1];
    char prelast_y = pos_y_grid[ptidx1];
    char prelast_dir = dir_grid[ptidx2];
    param0w = screen_get_bmap_addr(prelast_x, prelast_y);
    param0b = prelast_dir;
    param_draw_tile();

    // head always advances
    player_head_idx++;
    player_head_idx &= buf_mask;    // note buf_mask for circular buffer
    if (!on_fruit) {
        // tail only advances when fruit NOT eaten
        player_tail_idx++;
        player_tail_idx &= buf_mask;    // note buf_mask for circular buffer
    }
    else {
        // update score when fruit eaten
        score_counter += skill_level;
        print_score();
    }

    // draw fruit
    screen_print_user(fruit_x, fruit_y, &fruit_glyph);
    screen_print_attr(fruit_x, fruit_y, COLOR_FRUIT);

    // remember previous direction
    prev_dir = curr_dir;
}

// calculate random position for fruit,
// avoiding positions used by snake
void put_rand_fruit()
{
    char on_snake = 1;
    while (on_snake) {
        fruit_x = min_x + rand() % (1 + max_x - min_x);
        fruit_y = min_y + rand() % (1 + max_y - min_y);
        on_snake = is_used_grid(fruit_x, fruit_y);
    }
}

// process keypresses
void process_keyb()
{
    if (game_running)
    {
        // avoid self-killing by moving backwards
        if (keyWdown()) { if (curr_dir != DIRB) next_dir = DIRT; return; }
        if (keyAdown()) { if (curr_dir != DIRR) next_dir = DIRL; return; }
        if (keySdown()) { if (curr_dir != DIRT) next_dir = DIRB; return; }
        if (keyDdown()) { if (curr_dir != DIRL) next_dir = DIRR; return; }

        // pause key with pauselock for avoiding cycling on -> off -> on -> off
        if (keyPdown()) {
            if (!pauselock) {
                paused = !paused;
                print_top_message();
                pauselock = 1;
            }
        }
        else {
            pauselock = 0;
        }
    }
    else
    {
        // game over: restart game when ENTER pressed
        if (keyENTdown()) { restart_game(); return; }
    }
}

///////////////////////////////////////////////////////////////////////////////

// main game loop
void enter_game_loop()
{
    // setup game variables initialy
    restart_game();

    while(1)
    {
        // read keypresses and process them
        keyb_read();
        process_keyb();

        // wait for vertical retrace - simple timing
        HALT;

        // only when game running and not paused:
        if (game_running && !paused)
        {
            // move snake only each N frame
            halt_counter--;
            if (halt_counter == 0) {
                halt_counter = halts_per_frame;
                curr_dir = next_dir;
                update_game();
            }
        }

        // exit to menu when M pressed
        if (!game_running && keyMdown())
            break;
    }
}

// top message, with variation when paused
void print_top_message()
{
    text_print(top_message, 3, 0, PAPER_MSG, INK_MSG);

    if (paused)
        text_print("P to unpause", 32, 0, PAPER_MSG, INK_MSG);
}

// score on top message
void print_score()
{
    char buf[8];
    sprintf(buf, "%04u", score_counter);
    text_print(buf, 57, 0, PAPER_MSG, INK_MSG);
}

// game initialization / reinitialization
void restart_game()
{
    clear_screen();
    reset_vars();

    draw_frame(0, 32, 1, 23, COLOR_FRAME);
    print_top_message();

    put_rand_fruit();
    //do_first_frame();
    print_score();

    init_used_grid();
    halt_counter = 1;
}

// main entry point
void main()
{
    init_rand();

    while(1)
    {
        // do menu loop until menu exits
        enter_menu_loop();

        // after menu exits, set skill level in game
        halts_per_frame = skill_halt_table[skill_level];
        top_message[6] = '0' + skill_level;

        // do game loop until game exits
        enter_game_loop();
    }

}

// this is not the best practice possible, but it is OK for a small project like this.
// i'm not using a makefile, just a simple "snake_compile" command,
// so i just #include all C sources here.
#include "tile.c"
#include "params0.c"
#include "screen.c"
#include "keyb.c"
#include "frame.c"
#include "menu.c"
#include "text.c"

