#ifndef _COMMON_H
#define _COMMON_H

#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <vlc/vlc.h>
#include <inttypes.h>
#include <sys/file.h>

#define FILENAME_MAX_LEN 4096
#define CHUNK_SIZE 1000

#define NS_MAX_CONN 1024

#define DEFAULT_HOST "127.0.0.1"
#define NS_DEFAULT_PORT 4269

#define SS_METADATA ".ss_metadata_hidden"
#define NS_METADATA ".ns_metadata_hidden"

#define FILE_THRESHOLD 1000

#define NUM_BACKUPS 2

/* Any message sent over the network must have one of these values in the
 * header */
typedef enum _Operation
{
    OP_ACK,             /* MessageInt */
    OP_RAW,             /* MessageChunk */
    OP_SIZE,            /* MessageInt */
    OP_NS_INIT_SS,      /* MessageInt */
    OP_NS_INIT_CLIENT,  /* Message */
    OP_NS_INIT_FILE,    /* MessageFile */
    OP_NS_CREATE,       /* MessageFile */
    OP_NS_DELETE,       /* MessageFile */
    OP_NS_COPY,         /* MessageFile */
    OP_NS_GET_SS,       /* MessageFile */
    OP_NS_GET_SS_FORCE, /* MessageFile */
    OP_NS_REPLY_SS,     /* MessageAddr */
    OP_NS_LS,           /* MessageFile */
    OP_NS_LR,           /* MessageFile */
    OP_SS_READ,         /* MessageFile */
    OP_SS_WRITE,        /* MessageFile */
    OP_SS_WRITE_BACKUP, /* MessageFile */
    OP_SS_INFO,         /* MessageFile */
    OP_SS_STREAM,       /* MessageFile */
    OP_BACKUP_INFO1,    /* MessageAddr */
    OP_BACKUP_INFO2,    /* MessageAddr */
} Operation;

typedef enum _ErrCode
{
    ERR_NONE,   /* No error, successful transaction */
    ERR_CONN,   /* Some error in connection */
    ERR_REQ,    /* Invalid request */
    ERR_SS,     /* Could not find SS to complete request */
    ERR_SYNC,   /* Synchronisation issue, invalid type of message recieved */
    ERR_SYS,    /* Some system error when processing the request */
    ERR_EXISTS, /* Path already exists */
    ERR_LOCK,   /* Operation locked due to pending operation */
    ERR_QUIET,  /* Async write told async is not needed hence don't print */
} ErrCode;

static inline char *errcode_to_str(ErrCode ecode)
{
    switch (ecode)
    {
    case ERR_NONE:
        return "Success";
    case ERR_CONN:
        return "Connection error";
    case ERR_REQ:
        return "Invalid request";
    case ERR_SS:
        return "Path does not exist (or could not fetch storage server)";
    case ERR_SYNC:
        return "Synchronization issue, unexpected type of message recieved";
    case ERR_SYS:
        return "Some system error on storage server";
    case ERR_EXISTS:
        return "Path already exists";
    case ERR_LOCK:
        return "Operation rejected (locked) due to another pending operation";
    case ERR_QUIET:
        return "Doing async write";
    }
    return "Invalid error code";
}

typedef struct _Message
{
    Operation op;
    int size;
} Message;

typedef struct _MessageFile
{
    Operation op;
    int size;
    char file[2 * FILENAME_MAX_LEN]; /* Need space for upto two files, : separated) */
} MessageFile;

typedef struct _MessageChunk
{
    Operation op;
    int size;
    char chunk[CHUNK_SIZE];
} MessageChunk;

typedef struct _MessageInt
{
    Operation op;
    int size;
    int info;
} MessageInt;

/* Must be 4 bytes to fit int */
typedef struct _PortAndID
{
    uint16_t port;
    int16_t id;
} PortAndID;

typedef struct _MessageAddr
{
    Operation op;
    int size;
    struct sockaddr_in addr;
} MessageAddr;

typedef struct _SServerInfo
{
    struct sockaddr_in addr;
    int sock_fd;
    int is_used;
    int16_t id;
    uint16_t _port;
    int16_t backup[NUM_BACKUPS];
    int16_t backup_by[NUM_BACKUPS][NUM_BACKUPS];
} SServerInfo;

typedef struct _AsyncWriteInfo
{
    FILE *outfile;
    char *buffer;
    char *write_path;
    int size;
    int sock_fd;
} AsyncWriteInfo;

typedef enum _SSPath
{
    SS_PATH_BASE,
    SS_PATH_ACTUAL,
    SS_PATH_BACKUP,
    SS_PATH_MAX,
} SSPath;

void ipv4_print_addr(struct sockaddr_in *addr, const char *interface);

int sock_connect(char *node, uint16_t *port, PortAndID *ss_pd);
int sock_connect_addr(struct sockaddr_in *addr);
int sock_init(uint16_t *port);
int sock_accept(int sock_fd, struct sockaddr_in *sock_addr, PortAndID *ss_pd);
int sock_send(int sock, Message *message);
Message *sock_get(int sock);
void sock_send_ack(int sock, ErrCode *ecode);
ErrCode sock_get_ack(int sock);

void stream_music(char *ip, uint16_t port);
ErrCode stream_file(int client_socket, FILE *file);
/* Path utils */

char *path_remove_prefix(char *self, char *op);
char *path_concat(char *first, char *second);
void path_norm(char *path, int *size);
ErrCode path_sock_sendfile(int sock, FILE *infile, int pwrite);
ErrCode path_sock_getfile(int sock, Message *msg_header, FILE *outfile, char **buffer, int *buffer_size);

#endif
