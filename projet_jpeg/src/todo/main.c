
#include "common.h"


int main(int argc, char **argv)
{
	if (argc > 1) {
		FILE *file = NULL;
		char *path = argv[1];

		file = fopen(path, "rb");

		if (file != NULL) {

			char buf[2];
			uint16_t mark;
			uint16_t size;

			fread(buf, 1, sizeof(buf), file);
			mark = (buf[0] & 0xFF) << 8 | (buf[1] & 0xFF);

			/* On vérifie la présence du marqueur SOI */
			if (mark == 0xFFD8) {

				do {
					fread(buf, 1, sizeof(buf), file);
					mark = (buf[0] & 0xFF) << 8 | (buf[1] & 0xFF);

					fread(buf, 1, sizeof(buf), file);
					size = (buf[0] & 0xFF) << 8 | (buf[1] & 0xFF);

					/* Lecture section */
					switch (mark) {
					case 0xFFE0:
						fseek(file, size - 2, SEEK_CUR);
						break;

					/* COM */
					case 0xFFFE:
						fseek(file, size - 2, SEEK_CUR);
						break;

					/* DQT */
					case 0xFFDB:
						fseek(file, size - 2, SEEK_CUR);
						break;

					/* SOF0 */
					case 0xFFC0:
						fseek(file, size - 2, SEEK_CUR);
						break;

					/* DHT */
					case 0xFFC4:
						fseek(file, size - 2, SEEK_CUR);
						break;

					/* SOS */
					case 0xFFDA:
						fread(buf, 1, 1, file);
						fseek(file, 2*buf[0] + 3, SEEK_CUR);
						break;

					/* EOI */
					case 0xFFD9:
						exit(0);
						break;

					default:
						printf("Marqueur non supporté :\n0x%X\n", mark);
						exit(1);
					}

					printf("Marqueur :\n0x%X\n", mark);

				} while (!feof(file));
			}


			fclose(file);
		}
	}

	return EXIT_SUCCESS;
}



