#include "const.h"
#include "gaussianLib.h"
#include "kern.h"
#include "mpi.h"
#include "qdbmp.h"

int do_slave(int me, int kern_size, int stdev, int kern_orig) {

  float **kern_data, kernel_max, colour_max;
  UINT size, width, height;
  USHORT depth;
  UCHAR *data;
  BMP *bmp, *new_bmp;
  MPI_Status status;

  /* TODO: Non blocking would be better */
  /* Receive payload for processing configuration */
  MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, MPI_MASTER_NODE, MPI_SIZE_TAG,
    MPI_COMM_WORLD, &status);
  MPI_Recv(&width, 1, MPI_UNSIGNED_LONG, MPI_MASTER_NODE, MPI_WIDTH_TAG,
    MPI_COMM_WORLD, &status);
  MPI_Recv(&height, 1, MPI_UNSIGNED_LONG, MPI_MASTER_NODE, MPI_HEIGHT_TAG,
    MPI_COMM_WORLD, &status);
  MPI_Recv(&depth, 1, MPI_UNSIGNED_SHORT, MPI_MASTER_NODE, MPI_DEPTH_TAG,
    MPI_COMM_WORLD, &status);
  data = calloc(size, sizeof(UCHAR));
  MPI_Recv(data, size, MPI_UNSIGNED_CHAR, MPI_MASTER_NODE, MPI_DATA_TAG,
    MPI_COMM_WORLD, &status);

  /* Configure the kernel for gaussian distribution */
  kern_data = init_kern_data(kern_size);
  if (kern_data == NULL) {
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }
  generateGaussianKernel(kern_data, kern_size, stdev, kern_orig, &kernel_max,
    &colour_max);

  /* Create a set of BMPs for reading/writing the augmentation */
  bmp = BMP_Create(width, height, depth);
  if (BMP_CheckError(stderr) != BMP_OK) {
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }
  BMP_SetData(bmp, data);
  new_bmp = BMP_Create(width, height, depth);
  if (BMP_CheckError(stderr) != BMP_OK) {
    MPI_Abort(MPI_COMM_WORLD, MPI_ABORT_FAIL_CODE);
    return EXIT_FAILURE;
  }

  /* Process the data */
  applyConvolution(kern_data, kern_size, kern_orig, colour_max, bmp, new_bmp);
  data = BMP_GetData(new_bmp);

  /* Send the processed data */
  /* TODO: Non blocking would be better */
  MPI_Send(data, size, MPI_UNSIGNED_CHAR, MPI_MASTER_NODE, MPI_DATA_TAG,
    MPI_COMM_WORLD);

  if (new_bmp != NULL)
    BMP_Free(new_bmp);  /* NOTE: 'data' will be freed from within new_bmp */

  return EXIT_SUCCESS;

}
