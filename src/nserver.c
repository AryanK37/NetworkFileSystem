#include "common.h"
#include "try-trie/wrapper_opt.h"

pthread_mutex_t sservers_lock = PTHREAD_MUTEX_INITIALIZER;
SServerInfo sservers[NS_MAX_CONN] = {0};
int16_t sservers_count = 0;

char *nserver_meta_path = NULL;

SServerInfo *sserver_register(PortAndID *pd, struct sockaddr_in *addr, int sock_fd)
{
    SServerInfo *ret = NULL;
    pthread_mutex_lock(&sservers_lock);
    if (pd->id < 0)
    {
        pd->id = sservers_count++;
        FILE *meta = fopen(NS_METADATA, "w");
        if (!meta)
        {
            goto end;
        }
        fprintf(meta, "%hd\n", sservers_count);
        fclose(meta);
    }
    if (pd->id < NS_MAX_CONN)
    {
        ret = &sservers[pd->id];
        if (ret->is_used)
        {
            /* Used already, don't accept connection */
            ret = NULL;
        }
        else
        {
            ret->addr = *addr;
            ret->sock_fd = sock_fd;
            ret->is_used = 1;
            ret->id = pd->id;
            ret->_port = pd->port;
            if (ret->id == 0)
            {
                ret->backup[0] = 1;
                ret->backup[1] = 2;
                ret->backup_by[0][0] = 1;
                ret->backup_by[0][1] = -1;
                ret->backup_by[1][0] = 2;
                ret->backup_by[1][1] = -1;
            }
            else if (ret->id == 1)
            {
                ret->backup[0] = 0;
                ret->backup[1] = 2;
                ret->backup_by[0][0] = 0;
                ret->backup_by[0][1] = 2;
                ret->backup_by[1][0] = 3;
                ret->backup_by[1][1] = -1;
            }
            else if (ret->id == 2)
            {
                ret->backup[0] = 1;
                ret->backup[1] = 0;
                ret->backup_by[0][0] = 3;
                ret->backup_by[0][1] = -1;
                ret->backup_by[1][0] = 1;
                ret->backup_by[1][1] = 0;
            }
            else
            {
                ret->backup[0] = ret->id - 1;
                ret->backup[1] = ret->id - 2;
                ret->backup_by[0][0] = ret->id + 1;
                ret->backup_by[0][1] = -1;
                ret->backup_by[1][0] = ret->id + 2;
                ret->backup_by[1][1] = -1;
            }
        }
    }
end:
    pthread_mutex_unlock(&sservers_lock);
    return ret;
}

ErrCode _sserver_random(char *path, int *sserver_id)
{
    int ret = ERR_SS;
    if (sservers_count == 0)
    {
        return ret;
    }

    /* First try random */
    int rand_id;
    for (int try = 0; try < NS_MAX_CONN; try++)
    {
        rand_id = rand() % sservers_count;
        if (sservers[rand_id].is_used)
        {
            ret = ERR_NONE;
            break;
        }
    }

    /* If random fails try all SS */
    if (ret != ERR_NONE)
    {
        for (rand_id = 0; rand_id < NS_MAX_CONN; rand_id++)
        {
            if (sservers[rand_id].is_used)
            {
                ret = ERR_NONE;
                break;
            }
        }
    }

    if (ret == ERR_NONE)
    {
        ret = (create(rand_id, path) < 0) ? ERR_EXISTS : ERR_NONE;
        if (ret == ERR_NONE)
        {
            *sserver_id = rand_id;
        }
    }
    return ret;
}

ErrCode sserver_random(char *path, int *sock_fd)
{
    int sserver_id = -1;
    ErrCode ret;
    pthread_mutex_lock(&sservers_lock);
    int is_partial = 0;
    sserver_id = search_v2(path, &is_partial);
    if (sserver_id < 0)
    {
        ret = _sserver_random(path, &sserver_id);
    }
    else if (is_partial)
    {
        create(sserver_id, path);
        ret = ERR_NONE;
    }
    else
    {
        ret = ERR_EXISTS;
    }
    if (ret == ERR_NONE && sserver_id >= 0)
    {
        *sock_fd = sservers[sserver_id].sock_fd;
    }
    pthread_mutex_unlock(&sservers_lock);
    return ret;
}

ErrCode sserver_by_path(char *path, int *sock_fd, struct sockaddr_in *addr, int force_create, int with_delete)
{
    int ret = ERR_NONE;
    if (!path || !path[0])
    {
        return ERR_REQ;
    }

    pthread_mutex_lock(&sservers_lock);
    int is_partial = 0;
    int sserver_id = with_delete ? delete_file_folder(path) : search_v2(path, &is_partial);
    if (sserver_id < 0)
    {
        ret = force_create ? _sserver_random(path, &sserver_id) : ERR_SS;
    }
    else if (is_partial)
    {
        if (force_create)
        {
            ret = (create(sserver_id, path) < 0) ? ERR_EXISTS : ERR_NONE;
        }
        else
        {
            ret = ERR_SS;
        }
    }

    int is_write = force_create || with_delete;
    if (ret == ERR_NONE)
    {
        if (!sservers[sserver_id].is_used)
        {
            if (is_write)
            {
                ret = ERR_SS;
            }
            else
            {
                sserver_id = sservers[sserver_id].backup[0];
                if (!sservers[sserver_id].is_used)
                {
                    sserver_id = sservers[sserver_id].backup[1];
                    if (!sservers[sserver_id].is_used)
                    {
                        ret = ERR_SS;
                    }
                }
            }
        }
    }
    if (ret == ERR_NONE)
    {
        if (addr)
        {
            *addr = sservers[sserver_id].addr;
        }
        if (sock_fd)
        {
            *sock_fd = sservers[sserver_id].sock_fd;
        }
    }
    pthread_mutex_unlock(&sservers_lock);
    return ret;
}

void *handle_client(void *client_socket)
{
    int sock = (int)(intptr_t)client_socket;
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(sock);
        if (!msg)
        {
            break;
        }

        ErrCode ecode = ERR_NONE;
        int sserver_fd = -1;
        char *operation = "invalid operation";
        switch (msg->op)
        {
        case OP_NS_CREATE:
            operation = "create path";
            ecode = sserver_random(msg->file, &sserver_fd);
            if (ecode == ERR_NONE)
            {
                if (sock_send(sserver_fd, (Message *)msg))
                {
                    ecode = sock_get_ack(sserver_fd);
                }
                else
                {
                    ecode = ERR_CONN;
                }
                if (ecode != ERR_NONE)
                {
                    delete_file_folder(msg->file);
                }
            }
            sock_send_ack(sock, &ecode);
            break;
        case OP_NS_DELETE:
            operation = "delete path";
            ecode = sserver_by_path(msg->file, &sserver_fd, NULL, 0, 1);
            if (ecode == ERR_NONE)
            {
                if (sock_send(sserver_fd, (Message *)msg))
                {
                    ecode = sock_get_ack(sserver_fd);
                }
                else
                {
                    ecode = ERR_CONN;
                }
            }
            sock_send_ack(sock, &ecode);
            break;
        case OP_NS_COPY:
            operation = "copy paths";
            MessageAddr msg_addr;
            msg_addr.op = OP_NS_REPLY_SS;
            char *tmp = strchr(msg->file, ':');
            int src_exists = 0;
            int dest_exists = 0;
            if (!tmp)
            {
                ecode = ERR_REQ;
            }
            else
            {
                src_exists = IS_FILE(tmp + 1);
                *tmp = '\0';
                dest_exists = IS_FILE(msg->file);
                ecode = sserver_by_path(msg->file, NULL, &msg_addr.addr, 1, 0);
                if (ecode == ERR_NONE)
                {
                    ecode = sserver_by_path(tmp + 1, &sserver_fd, NULL, 0, 0);
                }
                *tmp = ':';
            }
            if (ecode == ERR_NONE)
            {
                // for folder use ls_v2 and list all the files and folder in folder
                if ((src_exists == 1 || src_exists == -1))
                {
                    if (sock_send(sserver_fd, (Message *)msg))
                    {
                        if (sock_send(sserver_fd, (Message *)&msg_addr))
                        {
                            ecode = sock_get_ack(sserver_fd);
                        }
                        else
                        {
                            ecode = ERR_CONN;
                        }
                    }
                    else
                    {
                        ecode = ERR_CONN;
                    }
                }
                else
                {
                    if (dest_exists == 0 || dest_exists == -1)
                    {
                        FILE *temp = tmpfile();
                        ls_v2(tmp + 1, temp);
                        fseek(temp, 0, SEEK_SET);
                        // get line by line from temp and send to sserver_fd
                        // after concatenating with destination path i.e msg->file
                        // so create a new message
                        char *line = NULL;
                        size_t len = 0;
                        ssize_t read;
                        *tmp = '\0';

                        while ((read = getline(&line, &len, temp)) != -1)
                        {
                            MessageFile msg_file;
                            msg_file.op = OP_NS_COPY;
                            /* Remove leading slash and trailing newline */
                            char *src_path = line;
                            while (src_path[0] == '/')
                            {
                                src_path++;
                                read--;
                            }
                            while (read && src_path[read - 1] == '\n')
                            {
                                src_path[--read] = '\0';
                            }

                            if (!read || src_path[read - 1] == '/')
                            {
                                continue;
                            }

                            char *dst_path = path_remove_prefix(src_path, tmp + 1);
                            if (!dst_path)
                            {
                                continue;
                            }
                            char *tmp_path = path_concat(msg->file, dst_path);
                            strcpy(msg_file.file, tmp_path);
                            free(tmp_path);
                            sserver_by_path(msg_file.file, NULL, NULL, 1, 0);
                            strcat(msg_file.file, ":");
                            strcat(msg_file.file, src_path);
                            if (sock_send(sserver_fd, (Message *)&msg_file))
                            {
                                if (sock_send(sserver_fd, (Message *)&msg_addr))
                                {
                                    ecode = sock_get_ack(sserver_fd);
                                }
                                else
                                {
                                    ecode = ERR_CONN;
                                }
                            }
                            else
                            {
                                ecode = ERR_CONN;
                            }
                        }
                        free(line);
                    }
                    else
                    {
                        ecode = ERR_EXISTS;
                    }
                }
            }
            sock_send_ack(sock, &ecode);
            break;
        case OP_NS_GET_SS:
        case OP_NS_GET_SS_FORCE:
            operation = "get SS";
            MessageAddr reply_addr;
            reply_addr.op = OP_NS_REPLY_SS;
            ecode = sserver_by_path(msg->file, NULL, &reply_addr.addr, msg->op == OP_NS_GET_SS_FORCE, 0);
            sock_send_ack(sock, &ecode);
            if (ecode == ERR_NONE)
            {
                ecode = sock_send(sock, (Message *)&reply_addr) ? ERR_NONE : ERR_CONN;
            }
            break;
        case OP_NS_LS:
        case OP_NS_LR:
            operation = (msg->op == OP_NS_LS) ? "list dir" : "list dir recursive";
            FILE *temp = tmpfile();
            if (!temp)
            {
                perror("[SELF] Could not create temporary file for processing");
                ecode = ERR_SYS;
                sock_send_ack(sock, &ecode);
            }
            else
            {
                if (msg->op == OP_NS_LS)
                {
                    ecode = ls(msg->file, temp) ? ERR_NONE : ERR_SS;
                }
                else
                {
                    ecode = ls_v2(msg->file, temp) ? ERR_NONE : ERR_SS;
                }
                if (ecode == ERR_NONE)
                {
                    fseek(temp, 0, SEEK_SET);
                    ecode = path_sock_sendfile(sock, temp, 1);
                }
                else
                {
                    sock_send_ack(sock, &ecode);
                    /* Read the ack of getfile */
                    ecode = sock_get_ack(sock);
                }
                fclose(temp);
            }
            break;
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(sock, &ecode);
        }
        if (ecode == ERR_NONE)
        {
            printf("[CLIENT %d] Executed %s '%s'\n", sock, operation, msg->file);
        }
        else
        {
            printf("[CLIENT %d] Failed to %s '%s': %s\n",
                   sock, operation, msg->file, errcode_to_str(ecode));
        }
        free(msg);
    }

    printf("[CLIENT %d] Disconnected\n", sock);
    close(sock);
    return NULL;
}

void *handle_ss(void *sserver_void)
{
    SServerInfo *sserver = (SServerInfo *)sserver_void;

    printf("[STORAGE SERVER %d] Connected storage server on: ", sserver->id);
    ipv4_print_addr(&sserver->addr, NULL);
    sserver->addr.sin_port = htons(sserver->_port);
    printf("[STORAGE SERVER %d] This is listening on: ", sserver->id);
    ipv4_print_addr(&sserver->addr, NULL);

    /* Send ID */
    MessageInt msg_id;
    msg_id.op = OP_ACK;
    msg_id.info = sserver->id;
    if (!sock_send(sserver->sock_fd, (Message *)&msg_id))
    {
        goto end;
    }

    /* Get available paths */
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(sserver->sock_fd);
        if (!msg)
        {
            goto end;
        }

        if (msg->op != OP_NS_INIT_FILE || !(msg->file[0]))
        {
            free(msg);
            break;
        }
        create(sserver->id, msg->file);
        printf("[STORAGE SERVER %d] Has file '%s'\n", sserver->id, msg->file);
        free(msg);
    }
    printf("[STORAGE SERVER %d] Finished exposing all files, now ready to process requests\n", sserver->id);

    pthread_mutex_lock(&sservers_lock);
    // inform sserver of its backups
    for (int i = 0; i < NUM_BACKUPS; i++)
    {
        if (sservers[sserver->backup[i]].is_used)
        {
            MessageAddr backupinfo;
            backupinfo.op = i ? OP_BACKUP_INFO2 : OP_BACKUP_INFO1;
            backupinfo.addr = sservers[sserver->backup[i]].addr;
            sock_send(sserver->sock_fd, (Message *)&backupinfo);
        }
    }

    for (int i = 0; i < NUM_BACKUPS; i++)
    {
        for (int j = 0; j < NUM_BACKUPS; j++)
        {
            int16_t back = sserver->backup_by[i][j];
            MessageAddr backupinfo;
            if (back >= 0 && sservers[back].is_used)
            {
                backupinfo.op = i ? OP_BACKUP_INFO2 : OP_BACKUP_INFO1;
                backupinfo.addr = sserver->addr;
                sock_send(sservers[back].sock_fd, (Message *)&backupinfo);
            }
        }
    }
    pthread_mutex_unlock(&sservers_lock);

    while (1)
    {
        char buffer;
        if (recv(sserver->sock_fd, &buffer, 1, MSG_PEEK) != 1)
        {
            break;
        }
        sleep(1);
    }
end:
    printf("[STORAGE SERVER %d] Disconnected\n", sserver->id);
    pthread_mutex_lock(&sservers_lock);
    close(sserver->sock_fd);
    sserver->is_used = 0;
    pthread_mutex_unlock(&sservers_lock);
    return NULL;
}

int main(int argc, char *argv[])
{
    uint16_t port = NS_DEFAULT_PORT;
    if (argc == 2)
    {
        port = (uint16_t)atoi(argv[1]);
    }
    else if (argc > 2)
    {
        fprintf(stderr, "[SELF] Expected at most 1 argument, got %d\n", argc - 1);
        return 1;
    }

    FILE *meta = fopen(NS_METADATA, "r");
    if (meta)
    {
        if (fscanf(meta, "%hd", &sservers_count) == 1)
        {
            printf("[SELF] Metadata file successfully read\n");
        }
        fclose(meta);
    }

    int server_fd = sock_init(&port);
    if (server_fd >= 0)
    {
        while (1)
        {
            struct sockaddr_in sock_addr;
            PortAndID pd = {0};
            int conn_fd = sock_accept(server_fd, &sock_addr, &pd);
            if (conn_fd < 0)
            {
                continue;
            }
            pthread_t thread_id;
            if (pd.port)
            {
                SServerInfo *sserver = sserver_register(&pd, &sock_addr, conn_fd);
                if (!sserver)
                {
                    close(conn_fd);
                    fprintf(stderr, "[SELF] Rejected storage server request!\n");
                    continue;
                }
                if (pthread_create(&thread_id, NULL, handle_ss, (void *)sserver) != 0)
                {
                    perror("[SELF] Thread creation failed");
                    continue;
                }
            }
            else
            {
                if (pthread_create(&thread_id, NULL, handle_client, (void *)(intptr_t)conn_fd) != 0)
                {
                    perror("[SELF] Thread creation failed");
                    continue;
                }
            }

            pthread_detach(thread_id);
        }
    }
    close(server_fd);
    return 0;
}
