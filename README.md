# Huffman-Codec

## Overview
### Huffman compression/decompression

This program utilizes byte-level Huffman compression/decompression to decrease file size. 

The algorithm behind this codec represents symbols with varying length bit patterns based on frequency in a file to save space. Therefore, this codec works best on data where symbols greatly vary in frequency in a file.

## Usage

The program produces `.huf` compressed archive files that can be decompressed.

### Compression

`./huff -c <file>`

### Decompression

`./huff -d <file>`

## Test Cases

A PPM image (`golfcore.ppm`), text file (`declaration.txt`), and a binary file (`hello`) are included in this repository.
