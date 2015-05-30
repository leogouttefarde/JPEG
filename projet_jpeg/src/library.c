
#include "library.h"


uint32_t truncate(int32_t s)
{
        if (s > 255)
                s = 255;

        else if (s < 0)
                s = 0;

        return (uint32_t)s;
}

uint8_t double2uint8(double x)
{
        uint8_t res;

        if (x > 255)
                res = 255;

        else if (x < 0)
                res = 0;

        else
                res = (uint8_t)x;

        return res;
}

bool print_block(int32_t *block)
{
        bool error = true;

        for (uint8_t i = 0; i < BLOCK_DIM; i++) {
                for (uint8_t j = 0; j < BLOCK_DIM; j++)
                        printf("%u ", block[i * BLOCK_DIM + j]);

                printf("\n");
        }
        
        printf("\n\n");

        return error;
}


