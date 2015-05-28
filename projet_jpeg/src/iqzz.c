
#include "iqzz.h"
#include "common.h"


#define MOVE(index, EVEN, ODD) if (index % 2) { ODD; } else { EVEN; }
#define UP      --x; ++y;
#define DOWN    ++x; --y;
#define TURN if ((down = !down)) { DOWN; } else { UP; }


void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64])
{
        bool down = false;
        uint8_t x = 0;
        uint8_t y = 0;

        for(uint8_t i = 0; i < 64; ++i) {

                out[x*8 + y] = in[i] * quantif[i];

                if (x == 0 || x == 7) {
                        MOVE(y, ++y, TURN);
                }
                else if (y == 0 || y == 7) {
                        MOVE(x, TURN, ++x);
                }
                else if (down) {
                        DOWN;
                }
                else {
                        UP;
                }
        }
}

