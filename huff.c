//
//  Adam Patyk
//  huff.c
//  ECE 6680 Lab 4: Huffman Codec
//  Byte-level Huffman Codec
//  Copyright © 2020 Adam Patyk. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "list.h"

#define NUM_SYMS 256

typedef struct huffman_codes_tag {
    unsigned char symbol;
    unsigned char *code;
    int code_len;
} huffman_codes_t;

void huffman_compress(FILE *, char *);
void huffman_decompress(FILE *, char *, int);
void huffman_encode(FILE *, FILE *, huffman_codes_t *);
void huffman_decode(FILE *, FILE *, list_t *, int);
FILE *create_output_file(char *, int);
void calc_freq(FILE *, list_t *, int *);
void store_freq_table(FILE *, list_t *, int);
list_t *read_freq_table(FILE *, int *);
void build_tree(list_t *);
huffman_codes_t *build_codes(list_t *, int);
void build_codes_rec(list_node_t *, huffman_codes_t *, int *, int, int *);
int compare(const data_t *, const data_t *);
int compare_freq(const data_t *, const data_t *);

// debugging functions
void list_debug_print(list_t *);
void debug_print_tree(list_t *);
void ugly_print(list_node_t *, int);
void debug_print_huffman_codes(huffman_codes_t *, int);

int main(int argc, char *const *argv) {
    FILE    *fpt_in;
    int     c, len;

    if (argc != 3) {
        fprintf(stderr, "Usage: ./huff -flag <file>\n");
        printf("Command line options\n");
        printf("Options -----------------\n");
        printf("  -c\tcompress file using Huffman codec\n");
        printf("  -d\tdecompress file using Huffman codec\n");
        exit(0);
    }

    // open file to be compressed
    fpt_in = fopen(argv[2], "rb"); // FILE SIZE LIMITED TO 4GB

    // command line argument handling
    while ((c = getopt(argc, argv, "cd")) != -1)
        switch (c) {
        case 'c': // compress
            huffman_compress(fpt_in, argv[2]);
            break;

        case 'd': // decompress
            // check for .huf extension
            len = strlen(argv[2]);
            if (strcmp(&argv[2][len - 4], ".huf") != 0) {
                fprintf(stderr, "Must be an .huf archive!\n");
                exit(0);
            }

            huffman_decompress(fpt_in, argv[2], len);
            break;

        default:
            printf("Command line options\n");
            printf("Options -----------------\n");
            printf("  -c\t\tcompress file using Huffman codec\n");
            printf("  -d\t\tdecompress file using Huffman codec\n");
            exit(1);
        }

    fclose(fpt_in);
    return 0;
}

void huffman_compress(FILE *fpt_in, char *filename) {
    int num_symbols, file_len;

    // open output file
    FILE *fpt_out = fopen(strcat(filename, ".huf"), "wb");

    // construct a linked list
    list_t *L = list_construct(compare, compare_freq);

    // read symbols from file, calculate frequencies of each symbol
    calc_freq(fpt_in, L, &file_len);
    num_symbols = list_size(L);

    // sort list
    list_sort(L);

    // store frequency of each symbol
    store_freq_table(fpt_out, L, file_len);

    // build tree
    build_tree(L);

    // build varying length bit patterns using tree
    huffman_codes_t *codes = build_codes(L, num_symbols);

    // output varying bit length patterns
    huffman_encode(fpt_in, fpt_out, codes);

    tree_destruct(L);
    fclose(fpt_out);
}

void huffman_decompress(FILE *fpt_in, char *filename, int len) {
    int file_len;

    FILE *fpt_out = create_output_file(filename, len);

    // read symbol frequency table from file
    list_t *L = read_freq_table(fpt_in, &file_len);

    // build huffman codes from frequencies
    build_tree(L);

    // decode varying bit patterns from file and output symbols to new file
    huffman_decode(fpt_in, fpt_out, L, file_len);

    tree_destruct(L);
    fclose(fpt_out);
}

// output Huffman codes to file
void huffman_encode(FILE *fpt_in, FILE *fpt_out, huffman_codes_t *codes) {
  int i, j, len, buffer_size = 0;
  unsigned char sym_from_file, byte_to_write;
  unsigned long int bit_buffer = 0;

  fseek(fpt_in, 0, SEEK_SET);
  while (!(feof(fpt_in))) {
      i = 0;
      fread(&sym_from_file, 1, 1, fpt_in);

      while (codes[i].symbol != sym_from_file)
          i++;

      len = codes[i].code_len;

      // set bits in buffer according to code
      for (j = 0; j < len; j++) {
          // little Endian - write from back to front
          if (codes[i].code[j] == 1) {
              // write codes starting at most significant bit
              bit_buffer |= 1UL << (63 - (j + buffer_size));
          }
      }

      buffer_size += codes[i].code_len;

      // write out lowest byte of buffer
      while (buffer_size >= 8) {
          byte_to_write = bit_buffer >> 56;
          fwrite(&byte_to_write, 1, 1, fpt_out);
          bit_buffer <<= 8;
          buffer_size -= 8;
      }
  }

  // write out remaining odd bits
  byte_to_write = bit_buffer >> 56;
  fwrite(&byte_to_write, 1, 1, fpt_out);
}

// decode Huffman codes and output corresponding symbols to file
void huffman_decode(FILE *fpt_in, FILE *fpt_out, list_t *list, int file_len) {
  int ptr, bit, sym_count = 0;
  unsigned char buf;
  list_node_t *node;

  // read in codes from compressed file
  ptr = 7;
  fread(&buf, 1, 1, fpt_in);
  // find EOF manually
  while (!(feof(fpt_in))) {
      // start at root
      node = list_iter_front(list);

      // work through Huffman tree
      while (node->left != NULL && node->right != NULL) {
          // convert bit to int for easier comparisons
          bit = buf & (1UL << ptr--) ? 1 : 0;

          if (bit == 0)
              node = node->left;
          else
              node = node->right;

          if (ptr < 0) {
              // read more when needed
              fread(&buf, 1, 1, fpt_in);
              ptr = 7;
          }
      }

      if (sym_count < file_len) {
          // output symbol for code
          fwrite(&node->data_ptr->sym, 1, 1, fpt_out);
          sym_count++;
      }
  }
}

// create "-recovered" file name
FILE *create_output_file(char *filename, int len) {
    // open renamed output file
    char *new_name = (char *)calloc(len + 10, sizeof(char));

    // check for original extension
    if (filename[len - 8] == '.') {
        strncpy(new_name, filename, len - 8);
        // append "-recovered" to name
        strcpy(new_name + len - 8, "-recovered");
        // append original extension
        strncpy(new_name + len + 2, &filename[len - 8], 4);
    }
    // no extension (binary files)
    else {
        // append "-recovered" to name
        strncpy(new_name, filename, len - 4);
        strcpy(new_name + len - 4, "-recovered");
    }

    return fopen(new_name, "wb");
}

// calculate frequency of each symbol in a file and file length in bytes
void calc_freq(FILE *fpt, list_t *list, int *file_len) {
    *file_len = 0;
    while (!(feof(fpt))) {
        data_t *tmp_data = calloc(1, sizeof(data_t));
        list_node_t *found_node = NULL;
        data_t *found_data = NULL;
        fread(&tmp_data->sym, 1, 1, fpt);
        if (!(feof(fpt))) *file_len += 1;
        tmp_data->freq = 0;

        // tally frequency of each symbol
        if ((found_node = list_elem_find(list, tmp_data)) != NULL) {
            found_data = list_access(list, found_node);
            found_data->freq++;
            free(tmp_data);
        }
        // add symbol to list
        else {
            tmp_data->freq = 1;
            list_insert(list, tmp_data, list_iter_back(list));
        }
    }
}

// output a table of symbols and their frequencies to a file
void store_freq_table(FILE *fpt, list_t *list, int file_len) {
    int i, zero = 0;
    data_t *data;
    list_node_t *rover;
    rover = list_iter_front(list);

    for (i = 0; i < list_size(list); i++) {
        data = list_access(list, rover);
        fwrite(&data->sym, 1, 1, fpt);
        fwrite(&data->freq, 1, 4, fpt);
        rover = list_iter_next(rover);
    }

    // store entry with frequency 0 to denote end of header
    fwrite(&zero, 1, 1, fpt);
    fwrite(&zero, 1, 4, fpt);
    // store file length
    fwrite(&file_len, 1, 4, fpt); // limited to 4 GB
}

// read in a table of symbols and their frequencies from a file
list_t *read_freq_table(FILE *fpt, int *file_len) {
    int freq;
    unsigned char sym;
    // construct a linked list
    list_t *list = list_construct(compare, compare_freq);
    // read in frequency table from file
    fread(&sym, 1, 1, fpt);
    fread(&freq, 1, 4, fpt);

    while (freq != 0) {
        data_t *tmp_data = calloc(1, sizeof(data_t));
        tmp_data->sym = sym;
        tmp_data->freq = freq;
        list_insert(list, tmp_data, list_iter_back(list));
        fread(&sym, 1, 1, fpt);
        fread(&freq, 1, 4, fpt);
    }
    fread(file_len, 1, 4, fpt);

    list_sort(list);
    return list;
}

// build a Huffman tree from a linked list (converts list to tree)
void build_tree(list_t *list) {
    while (list_size(list) > 1) {
        // combine two smallest frequencies into parent node
        list_node_t *L_node = calloc(1, sizeof(list_node_t));
        list_node_t *R_node = calloc(1, sizeof(list_node_t));
        data_t *parent_data = calloc(1, sizeof(data_t));
        L_node->left = list_iter_front(list)->left;
        L_node->right = list_iter_front(list)->right;
        L_node->data_ptr = list_remove(list, list_iter_front(list));
        R_node->left = list_iter_front(list)->left;
        R_node->right = list_iter_front(list)->right;
        R_node->data_ptr = list_remove(list, list_iter_front(list));
        parent_data->freq = L_node->data_ptr->freq + R_node->data_ptr->freq;
        list_insert(list, parent_data, list_iter_front(list));
        list_iter_front(list)->left = L_node;
        list_iter_front(list)->right = R_node;
        // resort frequencies (order stops mattering after combined)
        list_sort(list);
    }
}

// determine the Huffman code for each symbol in a Huffman tree
huffman_codes_t *build_codes(list_t *list, int num_syms) {
    huffman_codes_t *huff_codes = (huffman_codes_t *)calloc(num_syms, sizeof(huffman_codes_t));
    int sym_code[NUM_SYMS - 1], idx = 0;
    build_codes_rec(list->head, huff_codes, sym_code, 0, &idx);
    return huff_codes;
}

// recursive auxiliary function to traverse tree
void build_codes_rec(list_node_t *node, huffman_codes_t *huff_codes, int *code, int level, int *idx) {
    int i;

    // recurse through left nodes in tree
    if (node->left != NULL) {
        code[level] = 0;
        build_codes_rec(node->left, huff_codes, code, level + 1, idx);
    }

    // recurse through right nodes in tree
    if (node->right != NULL) {
        code[level] = 1;
        build_codes_rec(node->right, huff_codes, code, level + 1, idx);
    }

    // root node
    if (node->right == NULL && node->left == NULL) {
        // store symbol and code in struct
        huff_codes[*idx].symbol = node->data_ptr->sym;
        huff_codes[*idx].code = (unsigned char *)calloc(level, 1);

        for (i = 0; i < level; i++) {
            huff_codes[*idx].code[i] = code[i];
        }

        huff_codes[*idx].code_len = level;
        *idx += 1;
    }
}

// comparison function for linked list
int compare(const data_t *a, const data_t *b) {
    if (a->sym < b->sym)
        return 1;
    else if (a->sym > b->sym)
        return -1;
    else
        return 0;
}

// comparison function for linked list sorting algorithms
int compare_freq(const data_t *a, const data_t *b) {
    if (a->freq < b->freq)
        return 1;
    else if (a->freq > b->freq)
        return -1;
    else { // sort ties by alphabetical order
        if (a->sym < b->sym)
            return 1;
        else if (a->sym > b->sym)
            return -1;
        else
            return 0;
    }
}

// prints linked list
void list_debug_print(list_t *L) {
    list_node_t *n = list_iter_front(L);
    data_t *d = list_access(L, n);
    printf("[%c] - %d\n", d->sym, d->freq);

    while (list_iter_next(n) != NULL) {
        n = list_iter_next(n);
        d = list_access(L, n);
        printf("[%c] - %d\n", d->sym, d->freq);
    }
}

// prints tree left to right (rotated left 90 degrees)
void debug_print_tree(list_t *T) {
    ugly_print(T->head, 0);
}

// recursive auxiliary function for bst_debug_print_tree
void ugly_print(list_node_t *N, int level) {
    int i = 0;

    if (N == NULL) return;

    ugly_print(N->right, level + 1);

    for (i = 0; i < level; i++) printf("     "); /* 5 spaces */

    printf("%5d\n", N->data_ptr->freq);
    ugly_print(N->left, level + 1);
}

// prints symbols and their corresponding Huffman codes
void debug_print_huffman_codes(huffman_codes_t *codes, int num_symbols) {
    int i, j;

    for (i = 0; i < num_symbols; i++) {
        printf("[%c]\t", codes[i].symbol);

        for (j = 0; j < codes[i].code_len; j++) {
            printf("%d", codes[i].code[j]);
        }

        printf("\n");
    }
}
