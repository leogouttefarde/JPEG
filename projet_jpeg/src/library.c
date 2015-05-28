
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


