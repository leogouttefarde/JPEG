#!/bin/bash

if [[ "$#" -eq 3 ]]
then
    if [[ -e "$1" && -e "$2" ]]
    then
        if [[ -d "$1" && -d "$2" ]]
        then

            # Create all result directories
            mkdir -p "$3"
            res_jpeg2tiff="$3/jpeg2tiff"
            res_tiff2jpeg="$3/tiff2jpeg"
            res_tiff2tiff="$3/tiff2tiff"
            res_jpeg2jpeg="$3/jpeg2jpeg"
            mkdir -p "$res_jpeg2tiff"
            mkdir -p "$res_tiff2jpeg"
            mkdir -p "$res_tiff2tiff"
            mkdir -p "$res_jpeg2jpeg"

            # Compile encoder project 
            make

            # Decode all JPEG file from <jpeg directory> 
            # Results in "$3"/res_jpeg2tiff
            for file in "$1"/*
            do
                if [[ "$file" == *".jpeg" ]]
                then
                    res_name="$res_jpeg2tiff""/""$(basename "$file" .jpeg)"".tiff"
                    echo "############### Decode $(basename "$file") ###############"
                elif [[ "$file" == *".jpg" ]]
                then
                    res_name="$res_jpeg2tiff""/""$(basename "$file" .jpg)"".tiff"
                    echo "############### Decode $(basename "$file") ###############"
                else
                    echo "Input extension must be jpeg or jpg."
                fi
                time ./jpeg_encode "$file" -o "$res_name" -d
                echo "##############################################################"
                echo -e "\n"
            done

            # Encode all tiff files from <tiff directory>
            # Results in "$3"/res_tiff2jpeg
            c_max=25
            for file in "$2"/*
            do  
                if [[ "$file" == *".tiff" ]]
                then
                    res_name="$res_tiff2jpeg""/""$(basename "$file" .tiff)"".jpg"
                    echo "############### Encode $(basename "$file") ###############"
                elif [[ "$file" == *".tif" ]]
                then
                    res_name="$res_tiff2jpeg""/""$(basename "$file" .tif)"".jpg"
                    echo "############### Encode $(basename "$file") ###############"
                else
                    echo "Input extension must be tif or tiff."
                fi

                c=0
                echo "Compression rate : $c"
                ./jpeg_encode "$file" -o "$res_name" -c "$c"
                while [ "$?" -ne "0" ] && [ "$c" -le "$c_max" ]
                do
                    c=$((c + 1))
                    echo "Compression rate : $c"
                    ./jpeg_encode "$file" -o "$res_name" -c "$c"
                done
                time ./jpeg_encode "$file" -o "$res_name" -c "$c"
                echo "##############################################################"
                echo -e "\n"
            done

            # Compute tiff to tiff transformation
            # Results in "$3"/res_tiff2tiff
            for file in "$2"/*
            do  
                if [[ "$file" == *".tif" || "$file" == *".tiff" ]]
                then
                    res_name="$res_tiff2tiff""/""$(basename "$file")"
                    echo "########### TIFF to TIFF transformation for $(basename "$file") ###########"
                else
                    echo "Input extension must be tif or tiff."
                fi

                time ./jpeg_encode "$file" -o "$res_name" -d 
                echo "###############################################################################"
                echo -e "\n"
            done

            # Compute JPEG to JPEG transformation
            # Results in "$3"/res_jpeg2jpeg
            c_max=25
            for file in "$1"/*
            do  
                if [[ "$file" == *".jpeg" ]]
                then
                    res_name="$res_jpeg2jpeg""/""$(basename "$file")"
                    echo "########### JPEG to JPEG transformation for $(basename "$file") ###########"
                elif [[ "$file" == *".jpg" ]]
                then
                    res_name="$res_jpeg2jpeg""/""$(basename "$file")"
                    echo "########### JPEG to JPEG transformation for $(basename "$file") ###########"
                else
                    echo "Input extension must be jpeg or jpg."
                fi

                c=0
                echo "Compression rate : $c"
                ./jpeg_encode "$file" -o "$res_name" -c "$c" > /dev/null
                while [ "$?" -ne "0" ] && [ "$c" -le "$c_max" ]
                do
                    c=$((c + 1))
                    echo "Compression rate : $c"
                    ./jpeg_encode "$file" -o "$res_name" -c "$c" > /dev/null
                done
                time ./jpeg_encode "$file" -o "$res_name" -c "$c"
                echo "###############################################################################"
                echo -e "\n"
            done
            
        else
            echo "Inputs are not directory"
        fi
    else
        echo "Input directories not founded"
    fi
else
    echo "Usage : $0 <jpeg directory> <tiff directory> <res directory>"
fi