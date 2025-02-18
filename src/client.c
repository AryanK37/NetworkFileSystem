#include "common.h"

void *handle_async(void *sock_server_ptr)
{
    int sock_server = (int)(intptr_t)sock_server_ptr;
    ErrCode ret = sock_get_ack(sock_server);
    if (ret == ERR_NONE)
    {
        printf("\n[SELF] Asynchronous operation succeeded\n");
    }
    else
    {
        printf("[SELF] Asynchronous operation failed: %s\n", errcode_to_str(ret));
    }
    close(sock_server);
    return NULL;
}

int main(int argc, char *argv[])
{
    char *nserver_host = DEFAULT_HOST;
    uint16_t nserver_port = NS_DEFAULT_PORT;
    switch (argc)
    {
    case 1:
        break;
    case 3:
        nserver_port = (uint16_t)atoi(argv[2]);
        /* Fall through */
    case 2:
        nserver_host = argv[1];
        break;
    default:
        fprintf(stderr, "usage: ./client [nserver_host] [nserver_port]\n");
        return 1;
    }

    int sock_fd = sock_connect(nserver_host, &nserver_port, NULL);
    if (sock_fd >= 0)
    {
        printf("List of supported commands:\n");
        printf("- READ [path]:[optional local file to write contents to]\n");
        printf("- PWRITE [path]:[optional local file to read contents from] (this is the --SYNC implementation)\n");
        printf("- WRITE [path]:[optional local file to read contents from]\n");
        printf("- STREAM [path]\n");
        printf("- INFO [path]\n");
        printf("- COPY [destination path]:[source path]\n");
        printf("- LS [path]\n");
        printf("- LR [path] (list subpaths recursively)\n");
        printf("- CREATE [path]\n");
        printf("- DELETE [path]\n");
        while (1)
        {
            ErrCode ret = ERR_NONE;
            MessageFile request;
            char op[16];
            printf("> ");
            if (scanf("%15s ", op) != 1)
            {
                if (feof(stdin) || ferror(stdin))
                {
                    break;
                }
                ret = ERR_REQ;
                goto end_loop;
            }

            if (!fgets(request.file, sizeof(request.file), stdin))
            {
                break;
            }
            /* Strip newline */
            size_t arg_len = strlen(request.file);
            while (arg_len && request.file[arg_len - 1] == '\n')
            {
                arg_len--;
            }
            request.file[arg_len] = '\0';

            char *arg2 = NULL;
            if (strcasecmp(op, "READ") == 0 || strcasecmp(op, "WRITE") == 0 || strcasecmp(op, "PWRITE") == 0)
            {
                arg2 = strchr(request.file, ':');
                if (arg2)
                {
                    arg2[0] = '\0';
                    arg2++;
                    if (!arg2[0])
                    {
                        arg2 = NULL;
                    }
                }
            }

            if (strcasecmp(op, "COPY") == 0)
            {
                request.op = OP_NS_COPY;
            }
            else if (strcasecmp(op, "LS") == 0)
            {
                request.op = OP_NS_LS;
            }
            else if (strcasecmp(op, "LR") == 0)
            {
                request.op = OP_NS_LR;
            }
            else if (strcasecmp(op, "CREATE") == 0)
            {
                request.op = OP_NS_CREATE;
            }
            else if (strcasecmp(op, "DELETE") == 0)
            {
                request.op = OP_NS_DELETE;
            }
            else if (strcasecmp(op, "WRITE") == 0 || strcasecmp(op, "PWRITE") == 0)
            {
                request.op = OP_NS_GET_SS_FORCE;
            }
            else
            {
                request.op = OP_NS_GET_SS;
            }
            if (request.op == OP_NS_LS || request.op == OP_NS_LR)
            {
                ret = path_sock_getfile(sock_fd, (Message *)&request, stdout, NULL, NULL);
                goto end_loop;
            }

            ret = sock_send(sock_fd, (Message *)&request) ? ERR_NONE : ERR_CONN;
            if (ret != ERR_NONE)
            {
                goto end_loop;
            }
            ret = sock_get_ack(sock_fd);
            MessageAddr *ss_addr = NULL;
            if (ret == ERR_NONE && (request.op == OP_NS_GET_SS || request.op == OP_NS_GET_SS_FORCE))
            {
                ss_addr = (MessageAddr *)sock_get(sock_fd);
                if (ss_addr)
                {
                    if (ss_addr->op != OP_NS_REPLY_SS)
                    {
                        ret = ERR_SYNC;
                        free(ss_addr);
                        ss_addr = NULL;
                    }
                }
                else
                {
                    ret = ERR_CONN;
                }
            }

            if (!ss_addr)
            {
                /* For create/delete/copy, we are done here */
                goto end_loop;
            }

            if (strcasecmp(op, "READ") == 0)
            {
                FILE *outfile = arg2 ? fopen(arg2, "w") : stdout;
                if (!outfile)
                {
                    perror("[SELF] Could not open local file");
                }
                else
                {
                    int sock_server = sock_connect_addr(&ss_addr->addr);
                    request.op = OP_SS_READ;
                    ret = path_sock_getfile(sock_server, (Message *)&request, outfile, NULL, NULL);
                    if (outfile != stdout)
                    {
                        fclose(outfile);
                    }
                    close(sock_server);
                }
            }
            else if (strcasecmp(op, "PWRITE") == 0 || strcasecmp(op, "WRITE") == 0)
            {
                int pwrite = (strcasecmp(op, "PWRITE") == 0);
                FILE *infile = arg2 ? fopen(arg2, "r") : stdin;
                if (!infile)
                {
                    perror("[SELF] Could not open local file");
                }
                else
                {
                    int sock_server = sock_connect_addr(&ss_addr->addr);
                    request.op = OP_SS_WRITE;
                    if (!sock_send(sock_server, (Message *)&request))
                    {
                        ret = ERR_CONN;
                    }
                    if (ret == ERR_NONE)
                    {
                        ret = sock_get_ack(sock_server);
                    }
                    if (ret == ERR_NONE)
                    {
                        ret = path_sock_sendfile(sock_server, infile, pwrite);
                    }
                    if (infile != stdin)
                    {
                        fclose(infile);
                    }
                    if (ret == ERR_QUIET)
                    {
                        pthread_t tid;
                        pthread_create(&tid, NULL, handle_async, (void *)(intptr_t)sock_server);
                        pthread_detach(tid);
                    }
                    else
                    {
                        close(sock_server);
                    }
                }
            }
            else if (strcasecmp(op, "STREAM") == 0)
            {
                char ip[INET_ADDRSTRLEN + 1];
                uint16_t port;
                inet_ntop(AF_INET, &ss_addr->addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
                port = ntohs(ss_addr->addr.sin_port);
                int sock_server = sock_connect(ip, &port, NULL);

                request.op = OP_SS_STREAM;
                sock_send(sock_server, (Message *)&request);
                MessageInt *port_msg = (MessageInt *)sock_get(sock_server);
                if (!port_msg || port_msg->op != OP_ACK)
                {
                    ret = ERR_CONN;
                }
                else
                {
                    sleep(1);
                    stream_music(ip, port_msg->info);
                    ret = sock_get_ack(sock_server);
                }
                close(sock_server);
            }
            else if (strcasecmp(op, "INFO") == 0)
            {
                FILE *outfile = arg2 ? fopen(arg2, "w") : stdout;
                if (!outfile)
                {
                    perror("[SELF] Could not open local file");
                }
                else
                {
                    int sock_server = sock_connect_addr(&ss_addr->addr);
                    request.op = OP_SS_INFO;
                    printf("%s \n", request.file);
                    ret = path_sock_getfile(sock_server, (Message *)&request, outfile, NULL, NULL);
                    if (outfile != stdout)
                    {
                        fclose(outfile);
                    }
                    close(sock_server);
                }
            }
            else
            {
                ret = ERR_REQ;
            }
            free(ss_addr);
        end_loop:
            if (ret == ERR_NONE)
            {
                printf("[SELF] Operation succeeded\n");
            }
            else if (ret == ERR_QUIET)
            {
                printf("[SELF] Asynchronous operation started and pending\n");
            }
            else
            {
                printf("[SELF] Operation failed: %s\n", errcode_to_str(ret));
            }
        }
    }

    close(sock_fd);
    return 0;
}
