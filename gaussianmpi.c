#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "const.h"
#include "init.h"
#include "kern.h"
#include "master.h"
#include "mosaic.h"
#include "mpi.h"
#include "qdbmp.h"
#include "slave.h"

/*
 * GAUSSIANMPI
 * ------
 *
 * author: Jonathan Nagy <jnagy@myune.edu.au>
 *
 * Performs a the computations for a gaussian distribution blur of a bitmap
 * across multiple processes using the MPI standard.
 *
 * configuration:
 *   Check the const.h header file for some configurable fields.  Most notably
 *   one might want to configure the timeout in order to support larger images.
 *
 * compilation:
 *   - Requires openmpi, math libraries
 *   - Example:
 *        mpicc gaussianLib.o init.o kern.o master.o mosaic.o qdbmp.o slave.o \
 *          gaussianmpi.c -o gaussianmpi -lm
 *   See the makefile for additional information.
 *
 * usage:
 *   gaussianmpi <input filename> <output filename> <standard deviation>
 *
 * bugs:
 *   - pencils_large.bmp is _not_ processing for some unknown reason.
 *   - 
 *
 * notes:
 *   - Error handling in the MPI layer is handled by MPI itself.  Therefore
 *     any issues that are not recoverable (MPI_Send, MPI_Recv failures) will
 *     be thrown as an MPI_Abort.  Ideally, we should be performing non
 *     blocking calls throughout the application and checking the MPI errors
 *     ourselves.
 *   - if the output file does not exist, the output file will be created and
 *     become locked throughout the duration of the application.
 *   - needs some imrovement on memory management.  It would be more efficient
 *     to load the source bitmap one line at a time.  In addition, we could
 *     also read the kernel more efficiently and write to the destination
 *     bitmap faster by using memcpy/move.
 */
int main(int argc, char **argv) {

  int me, nproc;
  int nslave, stdev, kern_size, kern_orig;
  char fn_in[MAX_PATH], fn_out[MAX_PATH];

  /* Initialize MPI */
  if (init_mpi(&argc, &argv, &me, &nproc) != MPI_SUCCESS) {
    /* TODO: Graceful.  Do we need abort in here - not sure if it would
     * be vailable if MPI failed.. */
    return EXIT_FAILURE;
  }
  nslave = nproc - 1;

  /* Parse arguments and make calculations required for all nodes */
  if (parse_args(argc, argv, &stdev, fn_in, fn_out) == EXIT_FAILURE) {
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }
  if (init_kern(stdev, &kern_size, &kern_orig) == EXIT_FAILURE) {
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }

  /* Distribute work */
  if (me == MPI_MASTER_NODE) {
    if (do_master(nslave, kern_size, fn_in, fn_out) != EXIT_SUCCESS) {
      MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
      return EXIT_FAILURE;
    }
  } else {
    do_slave(me, kern_size, stdev, kern_orig);
    // TODO: Return
  }

  MPI_Finalize();

}
