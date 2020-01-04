////////////////////////////////////////////////////////////////////////////////
//
// Snake for ZX Spectrum 48K
//
// (C) 2019, 2020 David Crespo - https://github.com/dcrespo3d/zx-spectrum-snake
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

static char color_trans[8] = { 0, 1, 4, 5, 2, 3, 14, 7 };

void text_print(char* msg, char col, char row, char paper, char ink)
{
    // see https://github.com/z88dk/z88dk/wiki/Platform---Sinclair-ZX-Spectrum
    // and https://github.com/z88dk/z88dk/wiki/Classic-GenericConsole
    // and https://www.z88dk.org/forum/viewtopic.php?id=4395
    row += 0x20; // add displacement
    //col <<= 1; // double column coordinate for 32 column mode
    col += 0x20; // add displacement
    paper = color_trans[paper & 7];
    ink   = color_trans[ink   & 7];
    printf("\x10%c\x11%c\x16%c%c%s", ink, paper, row, col, msg);
}

