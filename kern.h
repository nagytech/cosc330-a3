#ifndef _KERN_H_
#define _KERN_H_

/* Constants */
#define KERNEL_DIMENSION_SD     3

/* Error messages */
#define EM_KERN_OOM       "Kernel failed to initialize float array\n"

float **init_kern_data(int kern_size);

int init_kern(int stdev, int *kern_size, int *kern_orig);

#endif /* _KERN_H_ */
