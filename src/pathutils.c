#include "common.h"

/* Returns a path with prefix removed if prefix matches, else returns NULL.
 * The returned buffer is simply a pointer to memory within 'self' argument */
char *path_remove_prefix(char *self, char *op)
{
    if (!self || !op)
    {
        return self;
    }

    /* Ignore leading slash */
    while (self[0] == '/')
    {
        self++;
    }
    while (op[0] == '/')
    {
        op++;
    }

    ssize_t op_len = strlen(op);
    if (op[op_len - 1] == '/')
    {
        op_len--;
    }
    if (!op_len)
    {
        /* "/" is a prefix for everything */
        return self;
    }

    if (!strncmp(self, op, op_len))
    {
        /* found match */
        char *ret = self + op_len;
        if (ret[0] != '/' && ret[0])
        {
            /* not a perfect match */
            return NULL;
        }
        return ret;
    }
    return NULL;
}

/* Returns a char buffer that should be freed by caller, or NULL on failure */
char *path_concat(char *first, char *second)
{
    ssize_t first_len = strlen(first);
    ssize_t second_len = strlen(second);
    while (first_len && first[first_len - 1] == '/')
    {
        /* Ignore trailing slashes */
        first_len--;
    }

    while (second[0] == '/')
    {
        /* Ignore leading slashes */
        second++;
        second_len--;
    }

    /* +2 is for / and trailing NULL */
    char *ret = malloc(first_len + second_len + 2);
    if (ret)
    {
        memcpy(ret, first, first_len);
        if (second_len)
        {
            ret[first_len] = '/';
            memcpy(ret + first_len + 1, second, second_len);
            ret[first_len + second_len + 1] = '\0';
        }
        else
        {
            /* Avoid trailing slash */
            ret[first_len] = '\0';
        }
    }
    return ret;
}

/* Returns a char buffer that should be freed by caller, or NULL on failure */
ErrCode path_sock_sendfile(int sock, FILE *infile, int pwrite)
{
    int fd = -1;
    int sz = -1;
    ErrCode ret = infile ? ERR_NONE : ERR_SYS;
    if (infile && infile != stdin)
    {
        fd = fileno(infile);
        if (fd == -1)
        {
            perror("[SELF] Error getting file descriptor");
            ret = ERR_SYS;
        }
        else
        {
            if (flock(fd, LOCK_SH | LOCK_NB) == -1)
            {
                fd = -1;
                ret = ERR_LOCK;
            }
        }
        if (ret == ERR_NONE && !pwrite)
        {
            fseek(infile, 0L, SEEK_END);
            sz = ftell(infile);
            rewind(infile);
        }
    }

    MessageInt msg_size;
    msg_size.info = (ret == ERR_NONE) ? sz : (int)ret;
    msg_size.op = (ret == ERR_NONE) ? OP_SIZE : OP_ACK;
    if (!sock_send(sock, (Message *)&msg_size))
    {
        ret = ERR_CONN;
    }

    MessageChunk chunk;
    chunk.op = OP_RAW;
    while (ret == ERR_NONE)
    {
        int read_bytes = fread(chunk.chunk, 1, sizeof(chunk.chunk), infile);
        if (!read_bytes)
        {
            ret = ferror(infile) ? ERR_SYS : ERR_NONE;
            clearerr(infile);
            break;
        }
        chunk.size = read_bytes;
        if (!sock_send(sock, (Message *)&chunk))
        {
            ret = ERR_CONN;
            goto end;
        }
        ret = sock_get_ack(sock);
    }
end:
    if (fd >= 0)
    {
        if (flock(fd, LOCK_UN) == -1)
        {
            ret = ERR_LOCK;
        }
    }
    sock_send_ack(sock, &ret);
    return sock_get_ack(sock);
}

ErrCode path_sock_getfile(int sock, Message *msg_header, FILE *outfile, char **buffer, int *buffer_size)
{
    int fd = -1;
    ErrCode ret = outfile ? ERR_NONE : ERR_SYS;
    if (outfile && outfile != stdout)
    {
        fd = fileno(outfile);
        if (fd == -1)
        {
            perror("[SELF] Error getting file descriptor");
            ret = ERR_SYS;
        }
        else
        {
            if (flock(fd, LOCK_EX | LOCK_NB) == -1)
            {
                fd = -1;
                ret = ERR_LOCK;
            }
        }
    }

    /* If we are sending ack packet, set error */
    if (msg_header->op == OP_ACK)
    {
        ((MessageInt *)msg_header)->info = ret;
    }

    if (!sock_send(sock, msg_header))
    {
        ret = ERR_CONN;
    }

    if (buffer_size)
    {
        *buffer_size = -1;
    }
    MessageInt *msg_size = (MessageInt *)sock_get(sock);
    if (!msg_size)
    {
        ret = ERR_CONN;
    }
    else if (msg_size->op != OP_SIZE)
    {
        ret = (msg_size->op == OP_ACK && msg_size->info != ERR_NONE)
                  ? msg_size->info
                  : ERR_SYNC;
    }
    else
    {
        if (buffer_size)
        {
            *buffer_size = msg_size->info;
        }
    }
    free(msg_size);

    char *cur_buffer = NULL;
    if (ret == ERR_NONE && buffer && buffer_size && *buffer_size > FILE_THRESHOLD)
    {
        *buffer = malloc(*buffer_size);
        if (!*buffer)
        {
            ret = ERR_SYS;
        }
        cur_buffer = *buffer;
    }

    while (ret == ERR_NONE)
    {
        Message *read_data = sock_get(sock);
        if (!read_data)
        {
            ret = ERR_CONN;
            break;
        }
        if (read_data->op != OP_RAW)
        {
            ret = (read_data->op == OP_ACK) ? ((MessageInt *)read_data)->info : ERR_SYNC;
            free(read_data);
            break;
        }
        MessageChunk *read_chunk = (MessageChunk *)read_data;
        if (cur_buffer)
        {
            memcpy(cur_buffer, read_chunk->chunk, read_chunk->size);
            cur_buffer += read_chunk->size;
        }
        else
        {
            fwrite(read_chunk->chunk, 1, read_chunk->size, outfile);
        }
        free(read_data);
        sock_send_ack(sock, &ret);
    }
    if (cur_buffer)
    {
        /* Async write */
        ret = ERR_QUIET;
    }
    else
    {
        /* Sync write */
        if (fd >= 0)
        {
            if (flock(fd, LOCK_UN) == -1)
            {
                perror("[SELF] Error unlocking file");
                ret = ERR_LOCK;
            }
        }
    }
    sock_send_ack(sock, &ret);
    return ret;
}

/* Normalize a path, dealing with ., .. and repeated / */
void path_norm(char *path, int *size)
{
    if (!path)
    {
        return;
    }

    int length = strlen(path);

    /* Empty string goes out as empty string */
    if (!length)
    {
        if (size)
        {
            *size = 0;
        }
        return;
    }

    int write = 0;
    int read = 0;

    /* If leading slash, leave it in place */
    if (path[0] == '/')
    {
        read++;
        path[write++] = '/';
    }

    while (read < length)
    {
        while (path[read] == '/' && read < length)
        {
            read++;
        }
        int name = read;
        while (read < length && path[read] != '/')
        {
            read++;
        }

        int name_len = read - name;
        if (name_len == 0 || (name_len == 1 && path[name] == '.'))
        {
            continue;
        }

        if (name_len == 2 && path[name] == '.' && path[name + 1] == '.')
        {
            if (write > 0)
            {
                write--;
                while (write > 0 && path[write - 1] != '/')
                {
                    write--;
                }
            }
            continue;
        }
        if (write > 0 && path[write - 1] != '/')
        {
            path[write++] = '/';
        }
        for (int i = 0; i < name_len; i++)
        {
            path[write++] = path[name + i];
        }
    }

    if (write == 0 || (path[length - 1] == '/' && path[write - 1] != '/'))
    {
        path[write++] = '/';
    }

    if (size)
    {
        *size = write;
    }
    path[write] = '\0';
}
