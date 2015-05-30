#Repertoires du projet

OBJ_DIR = bin
SRC_DIR = src
INC_DIR = include

# Options de compilation 

CC = gcc
LD = gcc
INC = -I$(INC_DIR)
CFLAGS = $(INC) -Wall -std=c99 -O3 #-Wextra -g #-O3
LDFLAGS = -lm

# Liste des objets encadrants

OBJ_FILES = $(OBJ_DIR)/main.o $(OBJ_DIR)/conv.o $(OBJ_DIR)/iqzz.o
OBJ_FILES += $(OBJ_DIR)/upsampler.o $(OBJ_DIR)/huffman.o $(OBJ_DIR)/unpack.o
OBJ_FILES += $(OBJ_DIR)/tiff.o $(OBJ_DIR)/library.o $(OBJ_DIR)/bitstream.o
OBJ_FILES += $(OBJ_DIR)/loeffler.o
#OBJ_FILES += $(OBJ_DIR)/idct.o

# Liste des objets realises
NEW_OBJ_FILES = $(OBJ_DIR)/main.o $(OBJ_DIR)/iqzz.o $(OBJ_DIR)/conv.o
NEW_OBJ_FILES += $(OBJ_DIR)/library.o $(OBJ_DIR)/huffman.o
NEW_OBJ_FILES += $(OBJ_DIR)/unpack.o $(OBJ_DIR)/upsampler.o $(OBJ_DIR)/bitstream.o
NEW_OBJ_FILES += $(OBJ_DIR)/idct.o
NEW_OBJ_FILES += $(OBJ_DIR)/loeffler.o
#NEW_OBJ_FILES += $(OBJ_DIR)/tiff.o

all : jpeg2tiff

# Edition de lien des executables
jpeg2tiff : $(OBJ_FILES)
	$(LD) -o jpeg2tiff $(OBJ_FILES) $(LDFLAGS)

# Compilation des sources

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c 
	$(CC) $(CFLAGS) ${INC} $< -o $@ -c

clean:
	rm -f jpeg2tiff $(NEW_OBJ_FILES)


# IMAGE=tests/shaun_the_sheep
IMAGE=tests/ice_age
# IMAGE=pic
JPEG=${IMAGE}.jpeg
TIFF=${IMAGE}.tiff

run: all
	./jpeg2tiff ${JPEG}

	if [ -a ${TIFF} ]; \
	then \
		display ${TIFF}; \
		rm -f ${TIFF}; \
	fi;
