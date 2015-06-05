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
CFLAGS = $(INC) -Wall -std=c99 -g #-O3 #-Wextra -g #-O3
LDFLAGS = -lm

# Liste des objets encadrants

OBJ_FILES = $(OBJ_DIR)/main.o $(OBJ_DIR)/conv.o $(OBJ_DIR)/iqzz.o
OBJ_FILES += $(OBJ_DIR)/upsampler.o $(OBJ_DIR)/huffman.o $(OBJ_DIR)/unpack.o
OBJ_FILES += $(OBJ_DIR)/tiff.o $(OBJ_DIR)/library.o $(OBJ_DIR)/bitstream.o
OBJ_FILES += $(OBJ_DIR)/loeffler.o
# OBJ_FILES += $(OBJ_DIR)/idct.o
OBJ_FILES += $(OBJ_DIR)/jpeg.o
OBJ_FILES += $(OBJ_DIR)/priority_queue.o

# Liste des objets realises
NEW_OBJ_FILES = $(OBJ_DIR)/iqzz.o $(OBJ_DIR)/conv.o
NEW_OBJ_FILES += $(OBJ_DIR)/library.o $(OBJ_DIR)/huffman.o
NEW_OBJ_FILES += $(OBJ_DIR)/unpack.o $(OBJ_DIR)/upsampler.o $(OBJ_DIR)/bitstream.o
NEW_OBJ_FILES += $(OBJ_DIR)/idct.o
NEW_OBJ_FILES += $(OBJ_DIR)/loeffler.o
NEW_OBJ_FILES += $(OBJ_DIR)/jpeg.o $(OBJ_DIR)/main.o
NEW_OBJ_FILES += $(OBJ_DIR)/tiff.o
NEW_OBJ_FILES += $(OBJ_DIR)/priority_queue.o

EXEC=jpeg_encode

all : ${EXEC}

TEST_FILE_O= test_file $(OBJ_DIR)/test_file.o $(OBJ_DIR)/priority_queue.o

test_file: $@ $(OBJ_DIR)/test_file.o $(OBJ_DIR)/priority_queue.o
	$(LD) -o $@ $(OBJ_DIR)/test_file.o $(OBJ_DIR)/priority_queue.o $(LDFLAGS)

TEST_DCT_O = test_dct $(OBJ_DIR)/test_dct.o $(OBJ_DIR)/idct.o $(OBJ_DIR)/library.o $(OBJ_DIR)/bitstream.o

test_dct: $@ $(OBJ_DIR)/test_dct.o $(OBJ_DIR)/idct.o $(OBJ_DIR)/library.o $(OBJ_DIR)/bitstream.o
	$(LD) -o $@ $(OBJ_DIR)/test_dct.o $(OBJ_DIR)/idct.o $(OBJ_DIR)/library.o $(OBJ_DIR)/bitstream.o $(LDFLAGS)


# Edition de lien des executables
${EXEC} : $(OBJ_FILES)
	$(LD) -o $@ $(OBJ_FILES) $(LDFLAGS)

# Compilation des sources

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c 
	$(CC) $(CFLAGS) ${INC} $< -o $@ -c

clean:
	rm -f ${EXEC} $(NEW_OBJ_FILES) gmon.out *.tiff out.jpg $(TEST_FILE_O)


# IMAGE=tests/shaun_the_sheep
IMAGE=tests/ice_age
# IMAGE=pic
JPEG=${IMAGE}.jpeg
TIFF=${IMAGE}.tiff
OUT=out.jpg

run: all
	./${EXEC} ${JPEG} -o $(OUT)
	#dot -Tpng dc_tree.dot > dc_tree.png
	#dot -Tpng ac_tree.dot > ac_tree.png

	# if [ -a ${OUT} ]; \
	# then \
	# 	display ${OUT}; \
	# 	rm -f ${OUT}; \
	# fi;

	#rm -f ${TIFF};

