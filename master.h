#ifndef _MASTER_H_
#define _MASTER_H_

#include "qdbmp.h"
#include "mosaic.h"

int do_master(int nslave, int kern_size, char *fn_in, char *fn_out);
int recv_results(int nslave, BMP *dest, int depth, struct mosaic_tile *head, int max_data_size);
int send_payload(int nslave, struct mosaic_tile *head, USHORT depth);

#endif /* _MASTER_H_ */
