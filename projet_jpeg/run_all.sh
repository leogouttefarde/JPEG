#!/bin/bash

if [[ "$#" -eq 1 ]]
then
    if [[ -e "$1" ]]
    then
	if [[ -d "$1" ]]
	then
	    # Decode all jpeg file in <test directory> 
	    for file in "$1"/*
	    do
		echo "############### Decode $(basename "$file") ###############"
		time ./jpeg2tiff "$file"
		echo "##############################################################"
		echo -e "\n"
	    done
	else
	    echo "Input is not a file."
	fi
    else
	echo "Input file not founded."
    fi
else
    echo "Usage : ./run_all <test directory>"
fi