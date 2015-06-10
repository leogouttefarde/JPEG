
#include "common.h"
#include "library.h"
#include "dct.h"

#define EPSILON 3


int main(void)
{
        bool success = true;

        uint8_t Y[64] = { 
                0xa6, 0xa1, 0x9b, 0x9a, 0x9b, 0x9c, 0x97, 0x92,
                0x9f, 0xa3, 0x9d, 0x8e, 0x89, 0x8f, 0x95, 0x94,
                0xa5, 0x97, 0x96, 0xa1, 0x9e, 0x90, 0x90, 0x9e,
                0xa7, 0x9b, 0x91, 0x91, 0x92, 0x91, 0x91, 0x94,
                0xca, 0xda, 0xc8, 0x98, 0x85, 0x98, 0xa2, 0x96,
                0xf0, 0xf7, 0xfb, 0xe8, 0xbd, 0x96, 0x90, 0x9d,
                0xe9, 0xe0, 0xf1, 0xff, 0xef, 0xad, 0x8a, 0x90,
                0xe7, 0xf2, 0xf1, 0xeb, 0xf7, 0xfb, 0xd0, 0x97
        };

        // uint8_t Y[64] =
        // {
        //         139, 144, 149, 153, 155, 155, 155, 155,
        //         144, 151, 153, 156, 159, 156, 156, 156,
        //         150, 155, 160, 163, 158, 156, 156, 156,
        //         159, 161, 162, 160, 160, 159, 159, 159, 
        //         159, 160, 161, 162, 162, 155, 155, 155,
        //         161, 161, 161, 161, 160, 157, 157, 157,
        //         162, 162, 161, 163, 162, 157, 157, 157,
        //         162, 162, 161, 161, 163, 158, 158, 158
        // };


        int32_t res[64];
        uint8_t end[64];

        dct_block(Y, res);
        idct_block(res, end);



        for(uint16_t i = 0; i < 64; i++)
                if (abs(Y[i] - end[i]) > EPSILON)
                        success = false;

        printf("DCT input :\n\n");
        print_byte_block(Y);

        printf("iDCT output :\n\n");
        print_byte_block(end);

        if (success)
                printf("Result : SUCCESS\n");

        else
                printf("Result : FAILED\n");


        return 0;
}
