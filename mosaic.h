#ifndef _MOSAIC_H_
#define _MOSAIC_H_

/*
 * mosaic.h
 * --------
 * Utility functions for splitting bitmap images into smaller horizontal tiles
 *
 */

#include "qdbmp.h"

/* Error messages */
#define EM_BMP_DEPTH      "Failed to get source bitmap depth\n"
#define EM_BMP_HEIGHT     "Failed to get source bitmap height\n"
#define EM_BMP_WIDTH      "Failed to get source bitmap width\n"
#define EM_TILE_OVERFLOW  "Tile grid smaller than kernel size\n"

/*
 * Linked list for iterating through a sequence of bitmap tiles
 */
typedef struct mosaic_tile {
  int id;                    /* Sequence number of tile */
  int imaxy, iminy;          /* Reference pixels on source image */
  int bot_over, top_over;    /* Margin excluded from tile mapping */
  UINT h, w;                 /* Height and width of tile (pixels) */
  UINT size;                 /* Size of tile data in bytes */
  BMP *bmp;                  /* Tile image */
  int processed;             /* Flag to indicate if the tile was processed */
  struct mosaic_tile *next;  /* Next tile in the linked list (or NULL) */
} MOSAIC_TILE;


struct mosaic_tile *create_tiles(BMP *src, int num, int kern_size,
  int *overlap, int *max_data_size);

int remap_tile(struct mosaic_tile *tile, BMP *src, BMP *dest);

#endif /* _MOSAIC_H_ */
