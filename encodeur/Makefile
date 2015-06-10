
#Repertoires du projet

OBJ_DIR = bin
SRC_DIR = src
INC_DIR = include


# Options de compilation

# PROFILING = -p
PROFILING = 
CC = gcc ${PROFILING}
LD = gcc ${PROFILING}
INC = -I$(INC_DIR)
#CFLAGS = $(INC) -Wall -std=c99 -Wextra -g
CFLAGS = $(INC) -Werror -Wall -std=c99 -O3 -Wextra -s
LDFLAGS = -lm


# Liste des objets à compiler

OBJ_FILES = $(OBJ_DIR)/conv.o $(OBJ_DIR)/qzz.o
OBJ_FILES += $(OBJ_DIR)/upsampler.o $(OBJ_DIR)/huffman.o $(OBJ_DIR)/unpack.o
OBJ_FILES += $(OBJ_DIR)/tiff.o $(OBJ_DIR)/library.o $(OBJ_DIR)/bitstream.o
OBJ_FILES += $(OBJ_DIR)/encode.o $(OBJ_DIR)/decode.o $(OBJ_DIR)/downsampler.o
OBJ_FILES += $(OBJ_DIR)/loeffler.o $(OBJ_DIR)/pack.o $(OBJ_DIR)/priority_queue.o
# OBJ_FILES += $(OBJ_DIR)/dct.o

COMPILE_O = $(OBJ_DIR)/main.o $(OBJ_FILES)


# Liste des objets realises

NEW_OBJ_FILES = $(OBJ_DIR)/main.o $(OBJ_DIR)/qzz.o $(OBJ_DIR)/conv.o
NEW_OBJ_FILES += $(OBJ_DIR)/library.o $(OBJ_DIR)/huffman.o
NEW_OBJ_FILES += $(OBJ_DIR)/unpack.o $(OBJ_DIR)/upsampler.o $(OBJ_DIR)/bitstream.o
NEW_OBJ_FILES += $(OBJ_DIR)/encode.o $(OBJ_DIR)/decode.o $(OBJ_DIR)/downsampler.o
NEW_OBJ_FILES += $(OBJ_DIR)/loeffler.o $(OBJ_DIR)/pack.o $(OBJ_DIR)/tiff.o
NEW_OBJ_FILES += $(OBJ_DIR)/dct.o $(OBJ_DIR)/priority_queue.o



EXEC=jpeg_encode

all : ${EXEC}


# Tests

test_file: $@ $(OBJ_DIR)/test_file.o $(OBJ_DIR)/priority_queue.o
	$(LD) -o $@ $(OBJ_DIR)/test_file.o $(OBJ_DIR)/priority_queue.o $(LDFLAGS)

test_tiff: $@ $(OBJ_DIR)/test_tiff.o $(OBJ_FILES)
	$(LD) -o $@ $(OBJ_DIR)/test_tiff.o $(OBJ_FILES) $(LDFLAGS)

test_dct: $@ $(OBJ_DIR)/test_dct.o $(OBJ_FILES)
	$(LD) -o $@ $(OBJ_DIR)/test_dct.o $(OBJ_FILES) $(LDFLAGS)

TEST_FILE_O= test_file $(OBJ_DIR)/test_file.o $(OBJ_DIR)/priority_queue.o
TEST_TIFF_O= test_tiff $(OBJ_DIR)/test_tiff.o
TEST_DCT_O= test_dct $(OBJ_DIR)/test_dct.o


# Edition de lien des executables

${EXEC} : $(COMPILE_O)
	$(LD) -o $@ $(COMPILE_O) $(LDFLAGS)


# Compilation des sources

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c 
	$(CC) $(CFLAGS) ${INC} $< -o $@ -c

clean:
	rm -f ${EXEC} $(NEW_OBJ_FILES) gmon.out *.tiff out.jpg $(TEST_FILE_O) $(TEST_DCT_O) $(TEST_TIFF_O)


# IMAGE=tests/jpeg/shaun_the_sheep
IMAGE=tests/jpeg/ice_age
JPEG=${IMAGE}.jpeg
TIFF=${IMAGE}.tiff
OUT=out.jpg

run: all
	./${EXEC} ${JPEG} -o $(OUT)
	# ./${EXEC} ${TIFF} -o $(OUT)

	if [ -a ${OUT} ]; \
	then \
		display ${OUT}; \
		rm -f ${OUT}; \
	fi;

prof:
	gprof ${EXEC} gmon.out > analysis.txt

