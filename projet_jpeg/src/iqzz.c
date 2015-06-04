
#include "iqzz.h"
#include "common.h"

/* // 1ère version implémentée
#define MOVE(index, EVEN, ODD) if (index % 2) { ODD; } else { EVEN; }
#define UP      --x; ++y;
#define DOWN    ++x; --y;
#define TURN if ((down = !down)) { DOWN; } else { UP; }
*/

/* version optimisée */
static const uint32_t zz[64] = 
{
         0,  1,  5,  6, 14, 15, 27, 28,
         2,  4,  7, 13, 16, 26, 29, 42,
         3,  8, 12, 17, 25, 30, 41, 43,
         9, 11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63
};

void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64])
{
	/* // 1ère version implémentée
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
	*/
	
	/* version optimisée */
        for(uint8_t i = 0; i < 64; ++i) {
                out[i] = in[zz[i]] * quantif[zz[i]];
        }
}


