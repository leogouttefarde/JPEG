/* Projet C - Sujet JPEG */
#ifndef __IQZZ_H__
#define __IQZZ_H__

#include <stdint.h>


/*
 * Computes an inverse zigzag quantification.
 */
extern void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64]);

/*
 * Computes a zigzag quantification.
 */
extern void qzz_block (int32_t out[64], int32_t in[64], uint8_t quantif[64]);

/*
 * Adjusts a quantification table's compression quality.
 *
 * Quality range : 0 - 25
 *
 * 0  : No compression
 * 25 : Max legal compression
 *
 * Quality shouldn't exceed 25 according to JPEG standard :
 * http://www-ljk.imag.fr/membres/Valerie.Perrier/SiteWeb/node10.html
 */
extern void quantify_qtable(uint8_t out[64], const uint8_t in[64], uint8_t quality);

#endif

