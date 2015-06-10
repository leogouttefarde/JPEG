#!/bin/bash

if [[ "$#" -eq 1 ]]
then
    if [[ -e "$1" ]]
    then
        if [[ -f "$1" ]]
        then

            # Compile decoder project
            make

            # Decode input file
            time ./jpeg2tiff "$1"

            # Compute standard output file name
            if [[ "$1" == *".jpeg"* ]]
            then
                res_name="$(dirname "$1")""/""$(basename "$1" .jpeg)"".tiff"
            elif [[ "$1" == *".jpg"* ]]
            then
                res_name="$(dirname "$1")""/""$(basename "$1" .jpg)"".tiff"
            else
                echo "Input extension must be jpeg or jpg."
            fi

            # Display input and decode file
            eog "$1" & 
            eog "$res_name" &
        else
            echo "Input is not a file."
        fi
    else
        echo "Input file not found."
    fi
else
    echo "Usage : $0 <jpeg file>"
fi