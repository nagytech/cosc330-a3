/* Input validation */
#define MAX_PATH                128
#define MAX_STDEV               20
#define MIN_STDEV               1

/* Tracing */
#define TRACE

/* Codes to use within MPI */
#define MPI_ABORT_FAIL_CODE     -1

/* Payload configuration */
#define PAYLOAD_COUNT           5

/* Tags for data in MPI */
#define MPI_DATA_TAG            1
#define MPI_SIZE_TAG            2
#define MPI_HEIGHT_TAG          3
#define MPI_WIDTH_TAG           4
#define MPI_DEPTH_TAG           5

/* Node Ids */
#define MPI_MASTER_NODE         0

/* Timing - Note: these values are approximate, not guaranteed */
#define MICRO_IN_S     1000000.0f
#define TIMEOUT_PAYLOAD_S    15.0f   /* Timeout for sending payload to slaves */
#define TIMEOUT_PAYLOAD_U       TIMEOUT_PAYLOAD_S * MICRO_IN_S
#define TIMEOUT_PROCESS_S    60.0f   /* Timeout for slaves to process image */
#define TIMEOUT_PROCESS_U       TIMEOUT_S * MICRO_IN_S
#define SLEEP_S              0.1f   /* Time to sleep between iterations*/
#define SLEEP_U                 SLEEP_S * MICRO_IN_S

/* Error messages */
#define EM_BMP_WRITE            "Failed to write output BMP Data\n"
#define EM_IO_CLOSE             "Failed to close output file handle: %s\n"
#define EM_IO_DEST_FAIL         "Output file error: %s\n"
#define EM_NODE_FAIL            "Encountered error on node [%d].\n"
#define EM_MAX_PATH             "File name exceeded max file path of %d\n"
#define EM_OUT_OF_MEMORY        "Out of memory while initializing type %s\n"
#define EM_PARSE_ARGS           "Failed to parse arguments\n"
#define EM_STDEV_RANGE          "Standard deviation range is %d-%d inclusive\n"
#define EM_TILE_NOT_FOUND       "Tile id %d not found in linked list\n"
#define EM_PAYLOAD_TIMEOUT      \
   "Sending payload to slave nodes timed out\n"
#define EM_TIMEOUT_RECV_SLAVE   \
   "Receiving processed data timed out for %d/%d nodes\n"
#define OOM_TYPE_RECEIPT_ARRAY  "receipt array buffer"
