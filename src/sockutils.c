#include "common.h"

/* Returns 1 on success, 0 on failure */
int sock_send(int sock, Message *message)
{
    if (sock < 0)
    {
        return 0;
    }
    switch (message->op)
    {
    case OP_NS_INIT_CLIENT:
        message->size = 0;
        break;
    case OP_NS_COPY:
    case OP_NS_INIT_FILE:
    case OP_NS_CREATE:
    case OP_NS_DELETE:
    case OP_NS_LS:
    case OP_NS_LR:
    case OP_SS_READ:
    case OP_SS_WRITE:
    case OP_SS_WRITE_BACKUP:
    case OP_SS_INFO:
    case OP_SS_STREAM:
    case OP_NS_GET_SS:
    case OP_NS_GET_SS_FORCE:
        message->size = strlen(((MessageFile *)message)->file) + 1;
        break;
    case OP_NS_INIT_SS:
    case OP_SIZE:
    case OP_ACK:
        message->size = sizeof(int);
        break;
    case OP_NS_REPLY_SS:
    case OP_BACKUP_INFO1:
    case OP_BACKUP_INFO2:
        message->size = sizeof(struct sockaddr);
        break;
    case OP_RAW:
        break;
    default:
        return 0;
    }

    int tot = sizeof(Message) + message->size;
    int sent = 0;
    do
    {
        int tmp = write(sock, (void *)message + sent, tot - sent);
        if (tmp <= 0)
        {
            return 0;
        }
        sent += tmp;
    } while (sent < tot);
    return 1;
}

Message *sock_get(int sock)
{
    if (sock < 0)
    {
        return 0;
    }
    Message *ret = malloc(sizeof(Message));
    if (!ret)
    {
        return 0;
    }
    if (read(sock, ret, sizeof(Message)) != sizeof(Message))
    {
        free(ret);
        return 0;
    }
    Message *new_ret = realloc(ret, sizeof(Message) + ret->size);
    if (!new_ret)
    {
        free(ret);
        return 0;
    }
    if (read(sock, (void *)new_ret + sizeof(Message), new_ret->size) != new_ret->size)
    {
        free(new_ret);
        return 0;
    }
    switch (new_ret->op)
    {
    case OP_NS_COPY:
    case OP_NS_INIT_FILE:
    case OP_NS_CREATE:
    case OP_NS_DELETE:
    case OP_NS_LS:
    case OP_NS_LR:
    case OP_SS_READ:
    case OP_SS_WRITE:
    case OP_SS_WRITE_BACKUP:
    case OP_SS_INFO:
    case OP_SS_STREAM:
    case OP_NS_GET_SS:
    case OP_NS_GET_SS_FORCE:
        path_norm(((MessageFile *)new_ret)->file, NULL);
        break;
    default:
        break;
    }
    return new_ret;
}

void sock_send_ack(int sock, ErrCode *ecode)
{
    MessageInt msg;
    msg.info = *ecode;
    msg.op = OP_ACK;
    if (!sock_send(sock, (Message *)&msg))
    {
        *ecode = ERR_CONN;
    }
}

ErrCode sock_get_ack(int sock)
{
    MessageInt *msg = (MessageInt *)sock_get(sock);
    if (!msg)
    {
        return ERR_CONN;
    }
    ErrCode ret = (msg->op == OP_ACK) ? msg->info : ERR_SYNC;
    free(msg);
    return ret;
}

void ipv4_print_addr(struct sockaddr_in *addr, const char *interface)
{
    if (addr && addr->sin_family == AF_INET)
    {
        char ip_str[16];
        inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));
        if (interface)
        {
            printf("[SELF] - %s (%s)\n", ip_str, interface);
        }
        else
        {
            printf("%s:%d\n", ip_str, ntohs(addr->sin_port));
        }
    }
    else if (!interface)
    {
        printf("UNKNOWN (family: %d)\n", addr->sin_family);
    }
}

int ipv4_is_wildcard(struct sockaddr *addr)
{
    if (addr && addr->sa_family == AF_INET)
    {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
        return addr_in->sin_addr.s_addr == INADDR_ANY;
    }
    return 0;
}

int _sock_print_info(uint16_t port)
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return -1;
    }

    printf("[SELF] Server listening in wildcard IPv4 address on port %hu\n", port);
    printf("[SELF] Here are some possible addresses clients can use:\n");
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        ipv4_print_addr((struct sockaddr_in *)ifa->ifa_addr, ifa->ifa_name);
    }

    freeifaddrs(ifaddr);
    return 0;
}

/* Passing port="0" will make this function dynamically choose any available port */
int sock_init(uint16_t *port)
{
    int status, ret;
    struct addrinfo hints, *res, *i;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%hu", *port);
    if ((status = getaddrinfo(NULL, port_str, &hints, &res)))
    {
        fprintf(stderr,
                "[SELF] Could not start server: %s\n",
                gai_strerror(status));
        return -1;
    }

    for (i = res; i; i = i->ai_next)
    {
        if (ipv4_is_wildcard(i->ai_addr))
        {
            ret = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
            if (ret < 0)
            {
                goto error;
            }
            if (bind(ret, i->ai_addr, i->ai_addrlen) != 0)
            {
                goto error;
            }
            if (listen(ret, NS_MAX_CONN) != 0)
            {
                goto error;
            }
            if (!(*port))
            {
                /* Get actual port and replace it in 'port' */
                struct sockaddr_in sin;
                socklen_t len = sizeof(sin);
                if (getsockname(ret, (struct sockaddr *)&sin, &len) == -1)
                {
                    goto error;
                }
                *port = ntohs(sin.sin_port);
            }
        }
    }
    if (_sock_print_info(*port) < 0)
    {
        goto error;
    }
    goto end;
error:
    perror("[SELF] Could not start server");
    ret = -1;
end:
    freeaddrinfo(res);
    return ret;
}

int sock_connect(char *node, uint16_t *port, PortAndID *ss_pd)
{
    struct addrinfo *res, *i;
    int status;
    int sock_fd = -1;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%hu", *port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(node, port_str, &hints, &res)))
    {
        fprintf(stderr,
                "[SELF] Could not connect to server: %s\n",
                gai_strerror(status));
        return -1;
    }
    for (i = res; i; i = i->ai_next)
    {
        sock_fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (sock_fd >= 0)
        {
            if (connect(sock_fd, i->ai_addr, i->ai_addrlen) == 0)
            {
                printf("[SELF] Connected to server: ");
                ipv4_print_addr((struct sockaddr_in *)i->ai_addr, NULL);
                break;
            }
            close(sock_fd);
            sock_fd = -1;
        }
    }

    freeaddrinfo(res);
    if (sock_fd == -1)
    {
        perror("[SELF] Could not connect to server");
        return -1;
    }

    if (ss_pd)
    {
        MessageInt msg;
        msg.info = *(int *)ss_pd;
        msg.op = OP_NS_INIT_SS;
        sock_send(sock_fd, (Message *)&msg);
        MessageInt *resp = (MessageInt *)sock_get(sock_fd);
        if (!resp || resp->op != OP_ACK)
        {
            free(resp);
            close(sock_fd);
            fprintf(stderr, "[SELF] Could not connect to server\n");
            sock_fd = -1;
        }
        else
        {
            ss_pd->id = (int16_t)resp->info;
        }
        free(resp);
    }
    else
    {
        Message msg;
        msg.op = OP_NS_INIT_CLIENT;
        sock_send(sock_fd, &msg);
    }

    return sock_fd;
}

int sock_connect_addr(struct sockaddr_in *addr)
{
    char ip[INET_ADDRSTRLEN + 1];
    uint16_t port;
    inet_ntop(AF_INET, &addr->sin_addr.s_addr, ip, INET_ADDRSTRLEN);
    port = ntohs(addr->sin_port);
    return sock_connect(ip, &port, NULL);
}

int sock_accept(
    int sock_fd, struct sockaddr_in *sock_addr, PortAndID *ss_pd)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int ret = accept(sock_fd, (struct sockaddr *)sock_addr, &addrlen);
    if (ret < 0)
    {
        perror("[SELF] Accept failed");
        return -1;
    }
    Message *init_msg = sock_get(ret);
    if (!init_msg)
    {
        close(ret);
        return -1;
    }
    switch (init_msg->op)
    {
    case OP_NS_INIT_SS:
        if (ss_pd)
        {
            PortAndID *pd = (PortAndID *)&(((MessageInt *)init_msg)->info);
            *ss_pd = *pd;
        }
        break;
    case OP_NS_INIT_CLIENT:
        printf("[CLIENT %d] Connected client on: ", ret);
        ipv4_print_addr(sock_addr, NULL);
        break;
    default:
        close(ret);
        ret = -1;
    }
    free(init_msg);
    return ret;
}
