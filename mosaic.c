#include <stdlib.h>
#include "qdbmp.h"
#include "mosaic.h"

/*
 * create_tiles:
 * Divide the given bitmap into a series of small horizontal tiles.
 * -----
 * src:           source bitmap
 * num:           number of tiles
 * kern_size:     diameter of the kernel
 * overlap:       overlap across tiles to account for kernel offset (out)
 * max_data_size: size of the largest tile (in bytes)
 *
 * returns:       *mosaic_tile: linked list
 */
struct mosaic_tile *create_tiles(BMP *src, int num, int kern_size,
  int *overlap, int *max_data_size) {

  int e;
  UCHAR r, g, b;
  struct mosaic_tile *head;
  UINT i, x, y, py, id, iw, ih, th, mds;

  head = NULL;
  e = BMP_OK;
  mds = 0;

  /* Get image dimensions */
  ih = BMP_GetHeight(src);
  if (BMP_CheckError(stderr) != BMP_OK) {
    fprintf(stderr, EM_BMP_HEIGHT);
    return head;
  }
  iw = BMP_GetWidth(src);
  if (BMP_CheckError(stderr) != BMP_OK) {
    fprintf(stderr, EM_BMP_WIDTH);
    return head;
  }
  id = BMP_GetDepth(src);
  if (BMP_CheckError(stderr) != BMP_OK) {
    fprintf(stderr, EM_BMP_DEPTH);
    return head;
  }

  /* Divide image into a number of tiles */
  th = (ih / num - 1);
  if (th < kern_size) {
    /* Tile height must be greater than the kernel size */
    fprintf(stderr, EM_TILE_OVERFLOW);
    return head;
  }

  /* Tiles must overlap by the 'radius' of the kernel. */
  *overlap = (kern_size - 1) / 2;

  /* Create linked list of tiles (in reverse) */
  for (i = 0; i < num; i++) {
    struct mosaic_tile *tile;

    tile = malloc(sizeof(*tile));
    if (tile == NULL) break;

    tile->next = head;
    head = tile;

    /* Set tile dimensions */
    tile->bot_over = i == 0 ? 0 : *overlap;
    tile->top_over = i < num - 1 ? *overlap : 0;
    tile->id = i + 1;
    tile->imaxy = (th * (i + 1)) + tile->top_over;
    tile->iminy = (th * i) - tile->bot_over;

    /* Check for any remainder and add it to the last tile */
    if (ih - tile->imaxy < th) {
      tile->imaxy = ih;
      tile->top_over = 0;
    }

    tile->h = tile->imaxy - tile->iminy;
    tile->w = iw;

    /* Create bmp to copy source pixels into */
    tile->bmp = BMP_Create(iw, tile->h, id);
    if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
    tile->size = BMP_GetDataSize(tile->bmp);
    if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
    if (mds < tile->size) mds = tile->size;

    /* Scan the columns and rows into new bitmap */
    /* Convert absolute to relative coordinates */
    for (py = 0, y = tile->iminy; y < tile->imaxy; y++, py++) {
      for (x = 0; x < iw; x++) {
        /* NOTE: We can most definitely speed this up by using memcpy
        and grabbing n lines at a time inclusive of the +n overlap lines
        required to accomodate the gaussian kernel */
        BMP_GetPixelRGB(src, x, y, &r, &g, &b);
        if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
        BMP_SetPixelRGB(tile->bmp, x, py, r, g, b);
        if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
      }
      if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
    }
    if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
  }

  *max_data_size = mds;

  return head;

}

/* remap_tile:
 * Maps the output of a convoluted image back to the source coordinates
 * on the original image
 * ------
 * tile:    mosaic tile with coordiante information
 * src:     source bitmap (small tile)
 * dest:    destination bitmap (original image)
 *
 * output:  int: error code (relaying qdbmp.h error codes)
 */
int remap_tile(struct mosaic_tile *tile, BMP *src, BMP *dest) {

  int e, x, y, py;
  UCHAR r, g, b;

  e = BMP_OK;

  /* Ignore the buffers on remapping the coordinates */
  for (y = 0 + tile->bot_over, py = tile->iminy + y;
      py < tile->imaxy - tile->top_over; y++, py++) {
    /* No remapping required for x coordinates */
    for (x = 0; x < tile->w; x++) {

      BMP_GetPixelRGB(src, x, y, &r, &g, &b);
      if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
      BMP_SetPixelRGB(dest, x, py, r, g, b);
      if ((e = BMP_CheckError(stderr)) != BMP_OK) break;

    }
    if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
  }

  return e;

}
