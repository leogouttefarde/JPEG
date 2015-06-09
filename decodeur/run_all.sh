#!/bin/bash

if [[ "$#" -eq 1 ]]
then
    if [[ -e "$1" ]]
    then
        if [[ -d "$1" ]]
        then

            # Compile decoder project
            make

            # Decode all JPEG files from <test directory> 
            for file in "$1"/*
            do
                echo "############## Decoding $(basename "$file") ##############"
                time ./jpeg2tiff "$file"
                echo "##############################################################"
                echo -e "\n"
            done
        else
            echo "Input is not a file."
        fi
    else
        echo "Input file not found."
    fi
else
    echo "Usage : ./run_all.sh <test directory>"
fi