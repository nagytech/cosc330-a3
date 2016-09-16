#ifndef _INIT_H_
#define _INIT_H_

#include "qdbmp.h"

int init_bmp(char *fn_in, BMP **src, UINT *width, UINT *height, USHORT *depth);
int init_mpi(int *argc, char ***argv, int *me, int *nproc);
int init_out(char *fn_out, int *f_out);
int parse_args(int argc, char **argv, int *stdev, char *fn_in, char *fn_out);

#endif /* _INIT_H_ */
