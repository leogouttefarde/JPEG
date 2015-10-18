# JPEG Encoder / Decoder

Usage : ./jpeg_encode &lt;input_file&gt; -o &lt;output_file&gt; [options]

Options list :

    -c <quality>  : Compression rate [0-25] (0 : lossless, 25 : highest)
    -m <mcu_size> : Output MCU sizes, either 8x8 / 16x8 / 8x16 / 16x16
    -g            : Encode as a gray image
    -d            : Decode to TIFF instead of encoding
    -h            : Display this help

Supported input images : TIFF, JPEG
