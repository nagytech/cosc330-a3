#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "const.h"
#include "init.h"
#include "mpi.h"

/* parse_args
 * -------
 * parses and validates the main argument array
 *
 * argc:  as per main
 * argv:  as per main
 * stdev: standard deviation value (see MIN_STDEV, MAX_STDEV)
 * fn_in: filename for image input
 * fn_out: filename for image output
 *
 * returns: success or failure
 *
 */
int parse_args(int argc, char **argv, int *stdev, char *fn_in, char *fn_out) {

  int stdev_in;

  stdev_in = atoi(argv[3]);

  /* Check range of standard deviaion */
  if (stdev_in < MIN_STDEV || stdev_in > MAX_STDEV) {
    fprintf(stderr, EM_STDEV_RANGE, MIN_STDEV, MAX_STDEV);
    return EXIT_FAILURE;
  }
  *stdev = atoi(argv[3]);

  /* Check filenames are present */
  if (strlen(fn_in) > MAX_PATH || strlen(fn_out) > MAX_PATH) {
    fprintf(stderr, EM_MAX_PATH, MAX_PATH);
    return EXIT_FAILURE;
  }

  strcpy(fn_in, argv[1]);
  strcpy(fn_out, argv[2]);

  return EXIT_SUCCESS;

}

/* init_out
 * ------
 * Initialize the output file by providing an up front lock.
 *
 * NOTE: If the file exists, the open will fail.  If the file does not exist,
 * it will be created.
 *
 * fn_out: output filename
 * f_out:  file handle pointer id
 *
 * return: success or fail
 *
 */
int init_out(char *fn_out, int *f_out) {

  int modes, perms, fh;

  modes = O_CREAT | O_WRONLY;
#ifndef DEBUG
  /* Fail to open where file exists */
  modes |= O_EXCL; /* NOTE: Removed for debug */
#endif
  perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  if ((fh = open(fn_out, modes, perms)) < 0) {
    fprintf(stderr, EM_IO_DEST_FAIL, strerror(errno));
    return EXIT_FAILURE;
  }

  *f_out =  fh;

  return EXIT_SUCCESS;

}

/* init_bmp
 * ------
 * Initialize the source bitmap and gather some reusable metadata
 *
 * fn_in:   input file name
 * src:     bitmap of source image
 * width:   width of source image (pixels)
 * height:  height of source image (pixels)
 * depth:   depth of image (bits)
 *
 * returns: success or failure
 *
 */
int init_bmp(char *fn_in, BMP **src, UINT *width, UINT *height, USHORT *depth) {

  int e;

  e = BMP_OK;

  /* Load image into BMP structure */
  *src = BMP_ReadFile(fn_in);
  if ((e = BMP_CheckError(stderr)) != BMP_OK) {
    return EXIT_FAILURE;
  }

  /* Get image metadata */
  *height = BMP_GetHeight(*src);
  if ((e = BMP_CheckError(stderr)) != BMP_OK) {
    BMP_Free(*src);
    return EXIT_FAILURE;
  }
  *width = BMP_GetWidth(*src);
  if ((e = BMP_CheckError(stderr)) != BMP_OK) {
    BMP_Free(*src);
    return EXIT_FAILURE;
  }
  *depth = BMP_GetDepth(*src);
  if ((e = BMP_CheckError(stderr)) != BMP_OK) {
    BMP_Free(*src);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}

/* init_mpi
 * ------
 * Iniitalize the MPI framework
 *
 * argc:  as per main (MPIs arguments will be withdrawn)
 * argv:  as per main (MPIs arguments will be withdrawn)
 * me:    current rank
 * nproc: number of processes, including master
 *
 * return: success or failure
 *
 */
int init_mpi(int *argc, char ***argv, int *me, int *nproc) {

  int e;

  if ((e = MPI_Init(argc, argv)) != MPI_SUCCESS) {
    fprintf(stderr, "Failed to initialize MPI\n");
    return e;
  }
  if ((e = MPI_Comm_rank(MPI_COMM_WORLD, me)) != MPI_SUCCESS) {
    fprintf(stderr, "Failed to establish MPI Comm rank\n");
    return e;
  }
  if ((e = MPI_Comm_size(MPI_COMM_WORLD, nproc)) != MPI_SUCCESS) {
    fprintf(stderr, "Failed to establish MPI Comm size\n");
    return e;
  }

  return e;

}
