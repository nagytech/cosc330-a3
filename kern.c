#include <stdlib.h>
#include <stdio.h>
#include "kern.h"

/*
 *  Initialize the given kernel values based on the set parameters
 *  ------
 *  stdev:  input standard deviation to use in combination with the
 *          KERNEL_DIMENSION_SD const.
 */
int init_kern(int stdev, int *kern_size, int *kern_orig) {

  *kern_size = (2 * (KERNEL_DIMENSION_SD * stdev)) + 1;
  *kern_orig = KERNEL_DIMENSION_SD * stdev;

  return EXIT_SUCCESS;

}

/*
 *  Initialize an 2-dimensional array of float values.
 *  ------
 *  kern_size:  width and height of the kernel
 */
float **init_kern_data(int kern_size) {

  int e, i;
  float **data;

  e = 0;

  /* Allocate the array of floats */
  data = calloc(kern_size, sizeof(float *));
  if (data == NULL) {
    fprintf(stderr, EM_KERN_OOM);
    return NULL;
  }
  for (i = 0; i < kern_size; i++) {
    data[i] = malloc(kern_size * sizeof(float));
    if (data[i] == NULL) {
      fprintf(stderr, EM_KERN_OOM);
      e = 1;
      break;
    }
  }

  /* Deallocate if we failed partway through */
  if (e == 1) {
    for (i = 0; i < kern_size; i++) {
      if (data[i] != NULL) free(data[i]);
    }
    free(data);
    data = NULL;
  }

  return data;

}
