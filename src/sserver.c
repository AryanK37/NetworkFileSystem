#include "common.h"

/* Indexes are SSPath entries, values are paths */
char *storage_path[SS_PATH_MAX] = {NULL};

struct sockaddr_in backups[NUM_BACKUPS] = {0};

/* Returns 1 on success, 0 on error */
int _sserver_send_files(int sock, char *parent, MessageFile *cur)
{
    struct dirent *entry;
    char *actual_path = path_concat(parent, cur->file);
    if (!actual_path)
    {
        return 0;
    }

    DIR *dir = opendir(actual_path);
    if (!dir)
    {
        free(actual_path);
        switch (errno)
        {
        case EACCES:
        case ENOENT:
            /* Ignore permission error, don't handle path */
            return 1;
        case ENOTDIR:
            /* Send file */
            cur->op = OP_NS_INIT_FILE;
            printf("[SELF] Exposing file '%s'\n", cur->file);
            return sock_send(sock, (Message *)cur);
        default:
            perror("[SELF] open dir failed");
            return 0;
        }
    }
    free(actual_path);

    /* Make folder */
    strcat(cur->file, "/");

    int ret = 1;
    int is_empty = 1;
    while ((entry = readdir(dir)) != NULL)
    {
        MessageFile new;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        is_empty = 0;
        int len = snprintf(new.file, sizeof(new.file), "%s%s", cur->file, entry->d_name);
        if (len <= 0 || len >= (int)sizeof(new.file))
        {
            continue;
        }

        if (!_sserver_send_files(sock, parent, &new))
        {
            ret = 0;
            break;
        }
    }
    if (is_empty && strcmp(cur->file, "/") != 0)
    {
        cur->op = OP_NS_INIT_FILE;
        printf("[SELF] Exposing folder '%s'\n", cur->file);
        ret &= sock_send(sock, (Message *)cur);
    }
    closedir(dir);
    return ret;
}

/* Returns 1 on success, 0 on error */
int sserver_send_files(int sock, char *path)
{
    MessageFile cur = {0};
    int ret = _sserver_send_files(sock, path, &cur);

    /* Send empty file to signal end */
    cur.op = OP_NS_INIT_FILE;
    cur.file[0] = '\0';
    ret &= sock_send(sock, (Message *)&cur);
    if (ret)
    {
        printf("[SELF] Finsihed exposing all files, now ready to process requests\n");
    }
    else
    {
        printf("[SELF] Failed while exposing files to naming server\n");
    }
    return ret;
}

ErrCode sserver_create(char *input_file_path, SSPath path)
{
    char *file_path = path_concat(storage_path[path], input_file_path);
    for (char *p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/'))
    {
        *p = '\0';
        if (mkdir(file_path, 0755) == -1)
        {
            if (errno != EEXIST)
            {
                free(file_path);
                return ERR_SYS;
            }
        }
        *p = '/';
    }
    size_t fsize = strlen(file_path);
    if (fsize && file_path[fsize - 1] == '/')
    {
        /* Create dir */
        if (mkdir(file_path, 0755) == -1)
        {
            if (errno != EEXIST)
            {
                free(file_path);
                return ERR_SYS;
            }
        }
        free(file_path);
    }
    else
    {
        /* Create file */
        FILE *file = fopen(file_path, "w");
        free(file_path);
        if (!file)
        {
            return ERR_SYS;
        }

        fclose(file);
    }

    return ERR_NONE;
}

FILE *_sserver_fopen(char *virtual_path, SSPath path, const char *mode)
{
    char *actual_path = path_concat(storage_path[path], virtual_path);
    FILE *ret = actual_path ? fopen(actual_path, mode) : NULL;
    free(actual_path);
    return ret;
}

FILE *sserver_fopen(char *virtual_path, int write)
{
    if (write)
    {
        sserver_create(virtual_path, SS_PATH_ACTUAL);
        return _sserver_fopen(virtual_path, SS_PATH_ACTUAL, "w");
    }
    FILE *ret = _sserver_fopen(virtual_path, SS_PATH_ACTUAL, "r");
    if (!ret)
    {
        ret = _sserver_fopen(virtual_path, SS_PATH_BACKUP, "r");
    }
    return ret;
}

FILE *sserver_fopen_backup(char *virtual_path, int write)
{
    if (write)
    {
        sserver_create(virtual_path, SS_PATH_BACKUP);
        return _sserver_fopen(virtual_path, SS_PATH_BACKUP, "w");
    }
    return _sserver_fopen(virtual_path, SS_PATH_BACKUP, "r");
}

/* Returns 1 on success, 0 on failure */
int _sserver_delete(char *file_path)
{
    struct dirent *entry;
    DIR *dir = opendir(file_path);
    if (!dir)
    {
        switch (errno)
        {
        case ENOTDIR:
            /* It is a file, delete it */
            if (unlink(file_path) != 0)
            {
                perror("[SELF] unlink failed");
                return 0;
            }
            return 1;
        default:
            perror("[SELF] open dir failed");
            return 0;
        }
    }

    int ret = 1;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char *concated_path = path_concat(file_path, entry->d_name);
        ret &= _sserver_delete(concated_path);
        free(concated_path);
    }
    closedir(dir);
    if (rmdir(file_path) != 0)
    {
        perror("[SELF] rmdir failed");
        ret = 0;
    }
    return ret;
}

ErrCode sserver_delete(char *input_file_path, SSPath path)
{
    char *file_path = path_concat(storage_path[path], input_file_path);
    ErrCode ret = _sserver_delete(file_path) ? ERR_NONE : ERR_SYS;
    free(file_path);
    return ret;
}

void *handle_async_and_backup(void *arg)
{
    AsyncWriteInfo *info = (AsyncWriteInfo *)arg;
    ErrCode err = ERR_NONE;
    if (info->buffer)
    {
        size_t written_bytes = fwrite(info->buffer, 1, info->size, info->outfile);
        if (written_bytes != (size_t)info->size)
        {
            err = ERR_SYS;
        }
        if (info->outfile != stdout && flock(fileno(info->outfile), LOCK_UN) == -1)
        {
            perror("[SELF] Error unlocking file");
            err = ERR_SYS;
        }
        sock_send_ack(info->sock_fd, &err);
    }
    free(info->buffer);
    if (info->outfile)
    {
        fclose(info->outfile);
    }
    /* Backup logic */
    ErrCode ecode = ERR_NONE;
    FILE *read_file = sserver_fopen(info->write_path, 0);
    if (read_file)
    {
        MessageFile msg;
        msg.op = OP_SS_WRITE_BACKUP;
        strcpy(msg.file, info->write_path);
        for (int i = 0; i < NUM_BACKUPS; i++)
        {
            if (!backups[i].sin_port)
            {
                continue;
            }
            int dst_sock = sock_connect_addr(&backups[i]);
            if (!dst_sock)
            {
                ecode = ERR_CONN;
            }
            else
            {
                ecode = sock_send(dst_sock, (Message *)&msg)
                            ? sock_get_ack(dst_sock)
                            : ERR_CONN;
                if (ecode == ERR_NONE)
                {
                    fseek(read_file, 0, SEEK_SET);
                    ecode = path_sock_sendfile(dst_sock, read_file, 1);
                }
                close(dst_sock);
            }
        }
        fclose(read_file);
    }
    else
    {
        ecode = ERR_SYS;
    }
    if (ecode == ERR_NONE)
    {
        printf("[CLIENT %d] Successfully backed up file '%s'\n",
               info->sock_fd, info->write_path);
    }
    else
    {
        printf("[CLIENT %d] Failed to backup file '%s': %s\n",
               info->sock_fd, info->write_path, errcode_to_str(ecode));
    }
    close(info->sock_fd);
    free(info->write_path);
    free(info);
    return NULL;
}

void *handle_client(void *fd_ptr)
{
    int sock_fd = (int)(intptr_t)fd_ptr;
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(sock_fd);
        if (!msg)
        {
            break;
        }
        ErrCode ecode = ERR_NONE;
        char *operation = "invalid operation";
        FILE *file = NULL;
        switch (msg->op)
        {
        case OP_NS_CREATE:
            operation = "create path in backup";
            ecode = sserver_create(msg->file, SS_PATH_BACKUP);
            break;
        case OP_NS_DELETE:
            operation = "delete path in backup";
            ecode = sserver_delete(msg->file, SS_PATH_BACKUP);
            break;
        case OP_SS_READ:
            operation = "read path";
            file = sserver_fopen(msg->file, 0);
            if (!file)
            {
                perror("[SELF] Could not open file");
                ecode = ERR_SYS;
            }
            ecode = path_sock_sendfile(sock_fd, file, 1);
            if (file)
            {
                fclose(file);
            }
            break;
        case OP_SS_WRITE:
            operation = "write path";
            file = sserver_fopen(msg->file, 1);
            if (!file)
            {
                perror("[SELF] Could not open file");
                ecode = ERR_SYS;
            }
            MessageInt ack;
            ack.op = OP_ACK;
            ack.info = ecode;
            char *buffer = NULL;
            int buffer_size = -1;
            ecode = path_sock_getfile(sock_fd, (Message *)&ack, file, &buffer, &buffer_size);
            AsyncWriteInfo *arg = malloc(sizeof(AsyncWriteInfo));
            arg->outfile = file;
            arg->buffer = buffer;
            arg->size = buffer_size;
            arg->sock_fd = sock_fd;
            arg->write_path = strdup(msg->file);
            /* Async write and backup */
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, handle_async_and_backup, arg);
            pthread_detach(thread_id);
            break;
        case OP_SS_WRITE_BACKUP:
            operation = "write path in backup";
            file = sserver_fopen_backup(msg->file, 1);
            if (!file)
            {
                perror("[SELF] Could not open file");
                ecode = ERR_SYS;
            }
            MessageInt ack2;
            ack2.op = OP_ACK;
            ack2.info = ecode;
            ecode = path_sock_getfile(sock_fd, (Message *)&ack2, file, NULL, NULL);
            if (file)
            {
                fclose(file);
            }
            break;
        case OP_SS_STREAM:
            operation = "stream path";
            uint16_t port = 0;
            file = sserver_fopen(msg->file, 0);
            if (!file)
            {
                perror("[SELF] Could not open file");
                ecode = ERR_SYS;
            }

            int server_socket = sock_init(&port);
            // inform client of port and ip
            MessageInt port_msg;
            port_msg.op = OP_ACK;
            port_msg.info = port;
            ecode = sock_send(sock_fd, (Message *)&port_msg);
            if (!ecode)
            {
                close(server_socket);
                ecode = ERR_CONN;
                break;
            }

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket < 0)
            {
                ecode = ERR_CONN;
            }
            else
            {
                ecode = stream_file(client_socket, file);
                close(client_socket);
            }
            fclose(file);
            sock_send_ack(sock_fd, &ecode);
            close(server_socket);
            break;
        case OP_SS_INFO:
            operation = "info of path";
            FILE *temp = tmpfile();
            if (!temp)
            {
                perror("[SELF] Could not create temporary file for processing");
                ecode = ERR_SYS;
            }
            file = sserver_fopen(msg->file, 0);
            struct stat st;
            fstat(fileno(file), &st);
            fprintf(temp, "Permissions: %o\n", st.st_mode & 0777);
            fprintf(temp, "Size: %ld bytes\n", st.st_size);
            fprintf(temp, "Last modified: %s", ctime(&st.st_mtime));
            fprintf(temp, "Last accessed: %s", ctime(&st.st_atime));
            fprintf(temp, "Creation time: %s", ctime(&st.st_ctime));
            rewind(temp);
            ecode = path_sock_sendfile(sock_fd, temp, 1);
            if (temp)
                fclose(temp);
            if (file)
            {
                fclose(file);
            }

            break;
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(sock_fd, &ecode);
            break;
        }
        if (ecode == ERR_NONE || ecode == ERR_QUIET)
        {
            printf("[CLIENT %d] Executed %s '%s'\n", sock_fd, operation, msg->file);
        }
        else
        {
            printf("[CLIENT %d] Failed to %s '%s': %s\n",
                   sock_fd, operation, msg->file, errcode_to_str(ecode));
        }
        free(msg);
    }

    printf("[CLIENT %d] Disconnected\n", sock_fd);
    close(sock_fd);
    return NULL;
}

void forward_req_backups(MessageFile *req)
{
    for (int i = 0; i < NUM_BACKUPS; i++)
    {
        if (backups[i].sin_port)
        {
            int dst_sock = sock_connect_addr(&backups[i]);
            if (dst_sock)
            {
                sock_send(dst_sock, (Message *)req);
                close(dst_sock);
            }
        }
    }
}

void *handle_ns(void *ns_fd_ptr)
{
    int ns_fd = (int)(intptr_t)ns_fd_ptr;
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(ns_fd);
        if (!msg)
        {
            break;
        }

        ErrCode ecode = ERR_NONE;
        char *operation = "invalid operation";
        switch (msg->op)
        {
        case OP_BACKUP_INFO1:
            backups[0] = ((MessageAddr *)msg)->addr;
            printf("[NAMING SERVER] Got backup (primary) info ");
            ipv4_print_addr(&backups[0], NULL);
            break;
        case OP_BACKUP_INFO2:
            backups[1] = ((MessageAddr *)msg)->addr;
            printf("[NAMING SERVER] Got backup (secondary) info ");
            ipv4_print_addr(&backups[1], NULL);
            break;
        case OP_NS_CREATE:
            operation = "create path";
            ecode = sserver_create(msg->file, SS_PATH_ACTUAL);
            if (ecode == ERR_NONE)
            {
                forward_req_backups(msg);
            }
            sock_send_ack(ns_fd, &ecode);
            break;
        case OP_NS_DELETE:
            operation = "delete path";
            ecode = sserver_delete(msg->file, SS_PATH_ACTUAL);
            if (ecode == ERR_NONE)
            {
                forward_req_backups(msg);
            }
            sock_send_ack(ns_fd, &ecode);
            break;
        case OP_NS_COPY:
            operation = "copy paths";
            int dst_sock = -1;
            FILE *src_file = NULL;
            char *src_path = strchr(msg->file, ':');
            if (!src_path)
            {
                ecode = ERR_REQ;
            }
            else
            {
                *src_path = '\0';
                src_path++;
                if (src_path)
                {
                    src_file = sserver_fopen(src_path, 0);
                    if (!src_file)
                    {
                        ecode = ERR_SYS;
                    }
                }
            }
            MessageAddr *reply_addr = (MessageAddr *)sock_get(ns_fd);
            if (!reply_addr)
            {
                ecode = ERR_CONN;
            }
            else if (reply_addr->op != OP_NS_REPLY_SS)
            {
                ecode = ERR_SYNC;
            }
            else
            {
                dst_sock = sock_connect_addr(&reply_addr->addr);
                if (dst_sock < 0)
                {
                    ecode = ERR_CONN;
                }
            }
            free(reply_addr);
            if (ecode == ERR_NONE)
            {
                msg->op = OP_SS_WRITE;
                ecode = sock_send(dst_sock, (Message *)msg) ? sock_get_ack(dst_sock) : ERR_CONN;
                if (ecode == ERR_NONE)
                {
                    ecode = path_sock_sendfile(dst_sock, src_file, 1);
                }
                fclose(src_file);
                close(dst_sock);
            }

            /* Ignore error while attempting to copy folder */
            if (ecode == ERR_SYS && src_path[strlen(src_path) - 1] == '/')
            {
                ecode = ERR_NONE;
            }
            sock_send_ack(ns_fd, &ecode);
            break;
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(ns_fd, &ecode);
        }
        if (msg->op != OP_BACKUP_INFO1 && msg->op != OP_BACKUP_INFO2)
        {
            if (ecode == ERR_NONE)
            {
                printf("[NAMING SERVER] Executed %s '%s'\n", operation, msg->file);
            }
            else
            {
                printf("[NAMING SERVER] Failed to %s '%s': %s\n",
                       operation, msg->file, errcode_to_str(ecode));
            }
        }
        free(msg);
    }

    printf("[NAMING SERVER] Disconnected\n");
    close(ns_fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    char *nserver_host = DEFAULT_HOST;
    uint16_t nserver_port = NS_DEFAULT_PORT;
    PortAndID pd;
    pd.port = 0;
    pd.id = -1;
    switch (argc)
    {
    case 4:
        nserver_port = (uint16_t)atoi(argv[3]);
        /* Fall through */
    case 3:
        nserver_host = argv[2];
        /* Fall through */
    case 2:
        storage_path[SS_PATH_BASE] = argv[1];
        if (sserver_create("actual/", SS_PATH_BASE) != ERR_NONE)
        {
            goto end;
        }
        storage_path[SS_PATH_ACTUAL] = path_concat(argv[1], "actual");
        if (sserver_create("backup/", SS_PATH_BASE) != ERR_NONE)
        {
            goto end;
        }
        storage_path[SS_PATH_BACKUP] = path_concat(argv[1], "backup");
        break;
    default:
        fprintf(
            stderr,
            "usage: ./sserver <storage_path> [nserver_host] [nserver_port]\n");
        return 1;
    }

    int sserver_fd = sock_init(&pd.port);
    if (sserver_fd < 0)
    {
        return 1;
    }
    char *metadata_file = path_concat(storage_path[SS_PATH_BASE], SS_METADATA);
    if (metadata_file)
    {
        /* Open a file for a single read, and create it if it does not exist */
        FILE *f = fopen(metadata_file, "a+");
        if (!f)
        {
            perror("[SELF] Could not access storage path");
            free(metadata_file);
            goto end;
        }
        fseek(f, 0, SEEK_SET);
        if (fscanf(f, "%hd", &pd.id) == 1)
        {
            printf("[SELF] Metadata file successfully read\n");
        }
        fclose(f);
    }

    int nserver_fd = sock_connect(nserver_host, &nserver_port, &pd);
    if (nserver_fd < 0)
    {
        free(metadata_file);
        goto end;
    }

    if (metadata_file)
    {
        FILE *f = fopen(metadata_file, "w");
        free(metadata_file);
        if (!f)
        {
            perror("[SELF] Could not access storage path");
            goto end;
        }
        fprintf(f, "%hd\n", pd.id);
        fclose(f);
    }

    printf("[SELF] Started storage server with id %hd on %s\n", pd.id, storage_path[SS_PATH_BASE]);
    if (!sserver_send_files(nserver_fd, storage_path[SS_PATH_ACTUAL]))
    {
        goto end;
    }

    pthread_t ns_thread_id;
    if (pthread_create(&ns_thread_id, NULL, handle_ns, (void *)(intptr_t)nserver_fd) != 0)
    {
        perror("[SELF] Thread creation failed");
        goto end;
    }

    while (1)
    {
        struct sockaddr_in sock_addr;
        int conn_fd = sock_accept(sserver_fd, &sock_addr, NULL);
        if (conn_fd < 0)
        {
            continue;
        }
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)(intptr_t)conn_fd) != 0)
        {
            perror("[SELF] Thread creation failed");
            continue;
        }
        pthread_detach(thread_id);
    }

end:
    free(storage_path[SS_PATH_ACTUAL]);
    free(storage_path[SS_PATH_BACKUP]);
    close(sserver_fd);
    close(nserver_fd);
    return 0;
}
