#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gaussianLib.h"
#include "const.h"
#include "init.h"
#include "master.h"
#include "mosaic.h"
#include "mpi.h"
#include "qdbmp.h"

/* do_master
 * ------
 * Main entry point for master node
 *
 * nslave:      number of slaves
 * kern_size:   size of the kernel
 * fn_in:       file name for input image
 * fn_out:      file name for output image
 *
 * return: success or failure
 *
 */
int do_master(int nslave, int kern_size, char *fn_in, char *fn_out) {

  USHORT depth;
  BMP *src, *dest;
  UINT width, height;
  struct mosaic_tile *head, *tile;
  int f_out, overlap, max_data_size;

  dest = NULL;

  /* Initialize data source */
  if (init_bmp(fn_in, &src, &height, &width, &depth) == EXIT_FAILURE) {
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }
  if (init_out(fn_out, &f_out) == EXIT_FAILURE) {
    BMP_Free(src);
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }
  head = create_tiles(src, nslave, kern_size, &overlap, &max_data_size);
  tile = head;
  if (tile == NULL) {
    BMP_Free(src);
    close(f_out);
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }

  /* Send payload and wait for all, or timeout */
  if (send_payload(nslave, head, depth) != EXIT_SUCCESS) {
    BMP_Free(src);
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }

  /* Receive processed results */
  if (recv_results(nslave, src, depth, head, max_data_size) != EXIT_SUCCESS) {
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }
  BMP_WriteFile(src, f_out);
  if (BMP_CheckError(stderr) != BMP_OK) {
    fprintf(stderr, EM_BMP_WRITE);
    return EXIT_FAILURE;
  }
  if (close(f_out) < 0) {
    fprintf(stderr, EM_IO_CLOSE, strerror(errno));
    return EXIT_FAILURE;
  }

  BMP_Free(dest);

  return EXIT_SUCCESS;
}

/* recv_results
 * ------
 * Receive the results from all slave nodes, translate the processed tiles into
 * the destination bitmap.
 *
 * nslave:        slave count
 * dest:          destination bitmap
 * depth:         bit depth of image
 * head:          head of linked list for all tiles
 * max_data_size: size of the largest tile in bytes
 *
 * return: success or failure
 *
 */
int recv_results(int nslave, BMP *dest, int depth, struct mosaic_tile *head, int max_data_size) {

  UCHAR **data;
  MPI_Status recv_stat;
  struct mosaic_tile *tile;
  MPI_Request recv_reqs[nslave];
  int expire, elapsed, complete, e, i;

  e = BMP_OK;
  tile = head;
  elapsed = 0;
  complete = 0;
  expire = TIMEOUT_PROCESS_S / SLEEP_S;

  data = calloc(nslave, sizeof(UCHAR*));
  if (data == NULL) {
    fprintf(stderr, EM_OUT_OF_MEMORY, OOM_TYPE_RECEIPT_ARRAY);
    return EXIT_FAILURE;
  }
  for (i = 0; i < nslave; i++) {
    data[i] = malloc(max_data_size * sizeof(UCHAR));
    if (data[i] == NULL) {
      fprintf(stderr, EM_OUT_OF_MEMORY, OOM_TYPE_RECEIPT_ARRAY);
      return EXIT_FAILURE;
    }
  }

  /* Pool the receipt of all other ranks */
  do {
    MPI_Irecv(data[tile->id - 1], tile->size, MPI_UNSIGNED_CHAR, tile->id, MPI_DATA_TAG,
      MPI_COMM_WORLD, &recv_reqs[tile->id - 1]);
  } while ((tile = tile->next) != NULL);
  tile = head;

  /* Await receipt of data */
  do {
    int test_flag, test_index;

    /* Test for a single slave entering the receipt queue */
    usleep(SLEEP_U);
    MPI_Testany(nslave, recv_reqs, &test_index, &test_flag, &recv_stat);
    if (test_flag) {

      BMP *section;

      /* Lookup tile by node id (offset by one for zero indexing) */
      tile = head;
      do {
        if (tile->id - 1 == test_index) break;
      } while ((tile = tile->next));
      if (tile == NULL) {
        fprintf(stderr, EM_TILE_NOT_FOUND, test_index);
        e = EXIT_FAILURE;
        break;
      }

      /* Load serialized data into new bitmap and translate the results */
      section = BMP_Create(tile->w, tile->h, depth);
      BMP_SetData(section, data[tile->id - 1]);
      if ((e = BMP_CheckError(stderr)) != BMP_OK) break;
      remap_tile(tile, section, dest);
      BMP_Free(section);
      
      /* Count completed */
      ++complete;
      #ifdef TRACE
          fprintf(stdout, "processed %d/%d nodes\n", complete, nslave);
      #endif
      if (complete == nslave) break;

    }

  } while (elapsed++ <= expire);

  if (data != NULL) {
    for (i = 0; i < nslave; i++) {
      if (data[i] != NULL) free(data[i]);
    }
    free(data);
  }

  /* Check for completeness */
  if (complete < nslave) {
    fprintf(stderr, EM_TIMEOUT_RECV_SLAVE, (nslave) - complete, nslave);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}

/* send_payload
 * non-blocking send of data required for slave nodes to process imagery
 * ------
 * nslave:        number of slave nodes
 * mosaic_tile:   linked list of tiles to process
 * depth:         image depth in bits
 *
 * returns:       success or failure code
 *
 * NOTE: MPI will abort upon any errors encountered
 *
 */
int send_payload(int nslave, struct mosaic_tile *head, USHORT depth) {

  UCHAR *data[nslave];
  struct mosaic_tile *tile;
  MPI_Request send_reqs[nslave * PAYLOAD_COUNT];
  MPI_Status send_stats[nslave * PAYLOAD_COUNT];
  int elapsed, expire, req_index, send_flag;

  tile = head;

  /* Pool the sending of all payload data */
  do {

#ifdef TRACE
    fprintf(stdout, "rank id %d sending payload to rank %d\n", 0, tile->id);
#endif

    req_index = (tile->id - 1); /* Convert to zero based index */
    req_index *= PAYLOAD_COUNT; /* Offset index by iteration index */

    data[tile->id - 1] = BMP_GetData(tile->bmp);

    MPI_Isend(&tile->size, 1, MPI_UNSIGNED_LONG, tile->id, MPI_SIZE_TAG,
        MPI_COMM_WORLD, &send_reqs[req_index++]);
    MPI_Isend(&tile->w, 1, MPI_UNSIGNED_LONG, tile->id, MPI_WIDTH_TAG,
        MPI_COMM_WORLD, &send_reqs[req_index++]);
    MPI_Isend(&tile->h, 1, MPI_UNSIGNED_LONG, tile->id, MPI_HEIGHT_TAG,
        MPI_COMM_WORLD, &send_reqs[req_index++]);
    MPI_Isend(&depth, 1, MPI_UNSIGNED_SHORT, tile->id, MPI_DEPTH_TAG,
        MPI_COMM_WORLD, &send_reqs[req_index++]);
    MPI_Isend(data[tile->id - 1], tile->size, MPI_UNSIGNED_CHAR, tile->id,
        MPI_DATA_TAG, MPI_COMM_WORLD, &send_reqs[req_index++]);

  } while ((tile = tile->next) != NULL);

  /* Wait for ALL slave nodes to respond */
  expire = TIMEOUT_PAYLOAD_S / SLEEP_S;
  elapsed = 0;
  #ifdef TRACE
    setbuf(stdout, NULL);
    fprintf(stdout, "Master waiting for response");
  #endif
  do {
#ifdef TRACE
    fprintf(stdout, ".");
#endif
    usleep(SLEEP_U);
    /*
     * Note: It may be more efficient to deal with incoming nodes
     * as they are sent using Testany.  This would allow us to gracefully
     * exit instad of calling MPI_Abort on a timeout.  However, in order
     * to have the whole application be 'graceful' we would need to override
     * the default MPI error handler - which means checking and handling all
     * errors manually
     *
     */
    MPI_Testall(nslave * PAYLOAD_COUNT, send_reqs, &send_flag, send_stats);
    if (send_flag) {
#ifdef TRACE
      fprintf(stdout, "\nAll responses received\n");
#endif
      break;
    };
  } while (elapsed++ < expire);

  /* Check for timeout */
  if (elapsed >= expire) {
    fprintf(stderr, EM_PAYLOAD_TIMEOUT);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}
