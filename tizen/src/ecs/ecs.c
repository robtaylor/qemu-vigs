/*
 * Emulator Control Server
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi   <jinhyung2.choi@samsung.com>
 *  MunKyu Im       <munkyu.im@samsung.com>
 *  Daiyoung Kim    <daiyoung777.kim@samsung.com>
 *  YeongKyoon Lee  <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

#include "hw/qdev.h"
#include "net/net.h"
#include "ui/console.h"

#include "qemu-common.h"
#include "qemu/queue.h"
#include "qemu/sockets.h"
#include "qemu/option.h"
#include "qemu/timer.h"
#include "qemu/main-loop.h"
#include "sysemu/char.h"
#include "config.h"
#include "qapi/qmp/qint.h"

#include "sdb.h"
#include "ecs.h"

#include "genmsg/ecs.pb-c.h"

#define DEBUG

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

static QTAILQ_HEAD(ECS_ClientHead, ECS_Client)
clients = QTAILQ_HEAD_INITIALIZER(clients);

static ECS_State *current_ecs;

static int port;
static int port_setting = -1;

static pthread_mutex_t mutex_clilist = PTHREAD_MUTEX_INITIALIZER;

static inline void start_logging(void) {
    char path[256];
    char* home;

    home = getenv(LOG_HOME);
    sprintf(path, "%s%s", home, LOG_PATH);

#ifdef _WIN32
    FILE* fnul;
    FILE* flog;

    fnul = fopen("NUL", "rt");
    if (fnul != NULL)
    stdin[0] = fnul[0];

    flog = fopen(path, "at");
    if (flog == NULL)
    flog = fnul;

    setvbuf(flog, NULL, _IONBF, 0);

    stdout[0] = flog[0];
    stderr[0] = flog[0];
#else
    int fd = open("/dev/null", O_RDONLY);
    dup2(fd, 0);

    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0640);
    if (fd < 0) {
        fd = open("/dev/null", O_WRONLY);
    }
    dup2(fd, 1);
    dup2(fd, 2);
#endif
}

int ecs_write(int fd, const uint8_t *buf, int len) {
    LOG("write buflen : %d, buf : %s", len, buf);
    if (fd < 0) {
        return -1;
    }

    return send_all(fd, buf, len);
}

void ecs_client_close(ECS_Client* clii) {
    pthread_mutex_lock(&mutex_clilist);

    if (0 <= clii->client_fd) {
        LOG("ecs client closed with fd: %d", clii->client_fd);
        closesocket(clii->client_fd);
#ifndef CONFIG_LINUX
        FD_CLR(clii->client_fd, &clii->cs->reads);
#endif
        clii->client_fd = -1;
    }

    QTAILQ_REMOVE(&clients, clii, next);
    if (NULL != clii) {
        g_free(clii);
        clii = NULL;
    }

    pthread_mutex_unlock(&mutex_clilist);
}

bool send_to_all_client(const char* data, const int len) {
    pthread_mutex_lock(&mutex_clilist);

    ECS_Client *clii;

    QTAILQ_FOREACH(clii, &clients, next)
    {
        send_to_client(clii->client_fd, data, len);
    }
    pthread_mutex_unlock(&mutex_clilist);

    return true;
}


void send_to_client(int fd, const char* data, const int len)
{
    ecs_write(fd, (const uint8_t*) data, len);
}

void read_val_short(const char* data, unsigned short* ret_val) {
    memcpy(ret_val, data, sizeof(unsigned short));
}

void read_val_char(const char* data, unsigned char* ret_val) {
    memcpy(ret_val, data, sizeof(unsigned char));
}

void read_val_str(const char* data, char* ret_val, int len) {
    memcpy(ret_val, data, len);
}

bool ntf_to_control(const char* data, const int len) {
    return true;
}

bool ntf_to_monitor(const char* data, const int len) {
    return true;
}

void ecs_make_header(QDict* obj, type_length length, type_group group,
        type_action action) {
    qdict_put(obj, "length", qint_from_int((int64_t )length));
    qdict_put(obj, "group", qint_from_int((int64_t )group));
    qdict_put(obj, "action", qint_from_int((int64_t )action));
}

static Monitor *monitor_create(void) {
    Monitor *mon;

    mon = g_malloc0(sizeof(*mon));
    if (NULL == mon) {
        LOG("monitor allocation failed.");
        return NULL;
    }

    return mon;
}

static void ecs_close(ECS_State *cs) {
    ECS_Client *clii;
    LOG("### Good bye! ECS ###");

    if (0 <= cs->listen_fd) {
        LOG("close listen_fd: %d", cs->listen_fd);
        closesocket(cs->listen_fd);
        cs->listen_fd = -1;
    }

    if (NULL != cs->mon) {
        g_free(cs->mon);
        cs->mon = NULL;
    }

    if (NULL != cs->alive_timer) {
        qemu_del_timer(cs->alive_timer);
        cs->alive_timer = NULL;
    }

    QTAILQ_FOREACH(clii, &clients, next)
    {
        ecs_client_close(clii);
    }

    if (NULL != cs) {
        g_free(cs);
        cs = NULL;
    }
}

#ifndef _WIN32
static ssize_t ecs_recv(int fd, char *buf, size_t len) {
    struct msghdr msg = { NULL, };
    struct iovec iov[1];
    union {
        struct cmsghdr cmsg;
        char control[CMSG_SPACE(sizeof(int))];
    } msg_control;
    int flags = 0;

    iov[0].iov_base = buf;
    iov[0].iov_len = len;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &msg_control;
    msg.msg_controllen = sizeof(msg_control);

#ifdef MSG_CMSG_CLOEXEC
    flags |= MSG_CMSG_CLOEXEC;
#endif
    return recvmsg(fd, &msg, flags);
}

#else
static ssize_t ecs_recv(int fd, char *buf, size_t len)
{
    return qemu_recv(fd, buf, len, 0);
}
#endif


static void reset_sbuf(sbuf* sbuf)
{
    memset(sbuf->_buf, 0, 4096);
    sbuf->_use = 0;
    sbuf->_netlen = 0;
}

static void ecs_read(ECS_Client *cli) {

    int read = 0;
    int to_read_bytes = 0;

#ifndef __WIN32
    if (ioctl(cli->client_fd, FIONREAD, &to_read_bytes) < 0)
    {
        LOG("ioctl failed");
        return;
    }
#else
    unsigned long to_read_bytes_long = 0;
    if (ioctlsocket(cli->client_fd, FIONREAD, &to_read_bytes_long) < 0)
    {
        LOG("ioctl failed");
         return;
    }
     to_read_bytes = (int)to_read_bytes_long;
#endif

    if (to_read_bytes == 0) {
        LOG("ioctl FIONREAD: 0\n");
        goto fail;
    }

    if (cli->sbuf._netlen == 0)
    {
        if (to_read_bytes < 4)
        {
            //LOG("insufficient data size to read");
            return;
        }

        long payloadsize = 0;
        read = ecs_recv(cli->client_fd, (char*) &payloadsize, 4);

        if (read < 4)
        {
            LOG("insufficient header size");
            goto fail;
        }

        payloadsize = ntohl(payloadsize);

        cli->sbuf._netlen = payloadsize;

        LOG("payload size: %ld\n", payloadsize);

        to_read_bytes -= 4;
    }

    if (to_read_bytes == 0)
        return;


    to_read_bytes = min(to_read_bytes, cli->sbuf._netlen - cli->sbuf._use);

    read = ecs_recv(cli->client_fd, (char*)(cli->sbuf._buf + cli->sbuf._use), to_read_bytes);
    if (read == 0)
        goto fail;


    cli->sbuf._use += read;


    if (cli->sbuf._netlen == cli->sbuf._use)
    {
        handle_protobuf_msg(cli, (char*)cli->sbuf._buf, cli->sbuf._use);
        reset_sbuf(&cli->sbuf);
    }

    return;
fail:
    ecs_client_close(cli);
}

#ifdef CONFIG_LINUX
static void epoll_cli_add(ECS_State *cs, int fd) {
    struct epoll_event events;

    /* event control set for read event */
    events.events = EPOLLIN;
    events.data.fd = fd;

    if (epoll_ctl(cs->epoll_fd, EPOLL_CTL_ADD, fd, &events) < 0) {
        LOG("Epoll control fails.in epoll_cli_add.");
    }
}
#endif

static ECS_Client *ecs_find_client(int fd) {
    ECS_Client *clii;

    QTAILQ_FOREACH(clii, &clients, next)
    {
        if (clii->client_fd == fd)
            return clii;
    }
    return NULL;
}

static int ecs_add_client(ECS_State *cs, int fd) {

    ECS_Client *clii = g_malloc0(sizeof(ECS_Client));
    if (NULL == clii) {
        LOG("ECS_Client allocation failed.");
        return -1;
    }

    reset_sbuf(&clii->sbuf);

    qemu_set_nonblock(fd);

    clii->client_fd = fd;
    clii->cs = cs;

    ecs_json_message_parser_init(&clii->parser, handle_qmp_command, clii);

#ifdef CONFIG_LINUX
    epoll_cli_add(cs, fd);
#else
    FD_SET(fd, &cs->reads);
#endif

    pthread_mutex_lock(&mutex_clilist);

    QTAILQ_INSERT_TAIL(&clients, clii, next);

    LOG("Add an ecs client. fd: %d", fd);

    pthread_mutex_unlock(&mutex_clilist);

    return 0;
}

static void ecs_accept(ECS_State *cs) {
    struct sockaddr_in saddr;
#ifndef _WIN32
    struct sockaddr_un uaddr;
#endif
    struct sockaddr *addr;
    socklen_t len;
    int fd;

    for (;;) {
#ifndef _WIN32
        if (cs->is_unix) {
            len = sizeof(uaddr);
            addr = (struct sockaddr *) &uaddr;
        } else
#endif
        {
            len = sizeof(saddr);
            addr = (struct sockaddr *) &saddr;
        }
        fd = qemu_accept(cs->listen_fd, addr, &len);
        if (0 > fd && EINTR != errno) {
            return;
        } else if (0 <= fd) {
            break;
        }
    }
    if (0 > ecs_add_client(cs, fd)) {
        LOG("failed to add client.");
    }
}

#ifdef CONFIG_LINUX
static void epoll_init(ECS_State *cs) {
    struct epoll_event events;

    cs->epoll_fd = epoll_create(MAX_EVENTS);
    if (cs->epoll_fd < 0) {
        closesocket(cs->listen_fd);
    }

    events.events = EPOLLIN;
    events.data.fd = cs->listen_fd;

    if (epoll_ctl(cs->epoll_fd, EPOLL_CTL_ADD, cs->listen_fd, &events) < 0) {
        close(cs->listen_fd);
        close(cs->epoll_fd);
    }
}
#endif

static void alive_checker(void *opaque) {
    /*
    ECS_State *cs = opaque;
    ECS_Client *clii;
    QObject *obj;

    obj = qobject_from_jsonf("{\"type\":\"self\"}");

    if (NULL != current_ecs && !current_ecs->ecs_running) {
        return;
    }

    QTAILQ_FOREACH(clii, &clients, next)
    {
        if (1 == clii->keep_alive) {
            LOG("get client fd %d - keep alive fail", clii->client_fd);
            //ecs_client_close(clii);
            continue;
        }
        LOG("set client fd %d - keep alive 1", clii->client_fd);
        clii->keep_alive = 1;
        ecs_json_emitter(clii, obj);
    }

    qemu_mod_timer(cs->alive_timer,
            qemu_get_clock_ns(vm_clock) + get_ticks_per_sec() * TIMER_ALIVE_S);
    */
}

static int socket_initialize(ECS_State *cs, QemuOpts *opts) {
    int fd = -1;
    Error *local_err = NULL;

    fd = inet_listen_opts(opts, 0, &local_err);
    if (0 > fd || error_is_set(&local_err)) {
        qerror_report_err(local_err);
        error_free(local_err);
        return -1;
    }

    LOG("Listen fd is %d", fd);

    qemu_set_nonblock(fd);

    cs->listen_fd = fd;

#ifdef CONFIG_LINUX
    epoll_init(cs);
#else
    FD_ZERO(&cs->reads);
    FD_SET(fd, &cs->reads);
#endif

    cs->alive_timer = qemu_new_timer_ns(vm_clock, alive_checker, cs);

    qemu_mod_timer(cs->alive_timer,
            qemu_get_clock_ns(vm_clock) + get_ticks_per_sec() * TIMER_ALIVE_S);

    return 0;
}

#ifdef CONFIG_LINUX
static int ecs_loop(ECS_State *cs) {
    int i, nfds;

    nfds = epoll_wait(cs->epoll_fd, cs->events, MAX_EVENTS, 100);
    if (0 == nfds) {
        return 0;
    }

    if (0 > nfds) {
        if (errno == EINTR)
            return 0;
        perror("epoll wait error");
        return -1;
    }

    for (i = 0; i < nfds; i++) {
        if (cs->events[i].data.fd == cs->listen_fd) {
            ecs_accept(cs);
            continue;
        }
        ecs_read(ecs_find_client(cs->events[i].data.fd));
    }

    return 0;
}
#elif defined(CONFIG_WIN32)
static int ecs_loop(ECS_State *cs)
{
    int index = 0;
    TIMEVAL timeout;
    fd_set temps = cs->reads;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (select(0, &temps, 0, 0, &timeout) < 0) {
        LOG("select error.");
        return -1;
    }

    for (index = 0; index < cs->reads.fd_count; index++) {
        if (FD_ISSET(cs->reads.fd_array[index], &temps)) {
            if (cs->reads.fd_array[index] == cs->listen_fd) {
                ecs_accept(cs);
                continue;
            }

            ecs_read(ecs_find_client(cs->reads.fd_array[index]));
        }
    }

    return 0;
}
#elif defined(CONFIG_DARWIN)
static int ecs_loop(ECS_State *cs)
{
    int index = 0;
    int res = 0;
    struct timeval timeout;
    fd_set temps = cs->reads;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if ((res = select(MAX_FD_NUM + 1, &temps, NULL, NULL, &timeout)) < 0) {
        LOG("select failed..");
        return -1;
    }

    for (index = 0; index < MAX_FD_NUM; index ++) {
        if (FD_ISSET(index, &temps)) {
            if (index == cs->listen_fd) {
                ecs_accept(cs);
                continue;
            }

            ecs_read(ecs_find_client(index));
        }
    }

    return 0;
}

#endif

static int check_port(int port) {
    int try = EMULATOR_SERVER_NUM;

    for (; try > 0; try--) {
        if (0 <= check_port_bind_listen(port)) {
            LOG("Listening port is %d", port);
            return port;
        }
        port++;
    }
    return -1;
}

int get_ecs_port(void) {
    if (port_setting < 0) {
        LOG("ecs port is not determined yet.");
        return 0;
    }
    return port;
}

static void* ecs_initialize(void* args) {
    int ret = 1;
    int index;
    ECS_State *cs = NULL;
    QemuOpts *opts = NULL;
    Error *local_err = NULL;
    Monitor* mon = NULL;
    char host_port[16];

    start_logging();
    LOG("ecs starts initializing.");

    opts = qemu_opts_create(qemu_find_opts(ECS_OPTS_NAME), ECS_OPTS_NAME, 1, &local_err);
    if (error_is_set(&local_err)) {
        qerror_report_err(local_err);
        error_free(local_err);
        return NULL;
    }

    port = check_port(HOST_LISTEN_PORT);
    if (port < 0) {
        LOG("None of port is available.");
        return NULL;
    }

    qemu_opt_set(opts, "host", HOST_LISTEN_ADDR);

    cs = g_malloc0(sizeof(ECS_State));
    if (NULL == cs) {
        LOG("ECS_State allocation failed.");
        return NULL;
    }

    for (index = 0; index < EMULATOR_SERVER_NUM; index ++) {
        sprintf(host_port, "%d", port);
        qemu_opt_set(opts, "port", host_port);
        ret = socket_initialize(cs, opts);
        if (0 > ret) {
            LOG("socket initialization failed with port %d. next trial", port);
            port ++;

            port = check_port(port);
            if (port < 0) {
                LOG("None of port is available.");
                break;
            }
        } else {
            break;
        }
    }

    if (0 > ret) {
        LOG("socket resource is full.");
        port = -1;
        return NULL;
    }

    port_setting = 1;

    mon = monitor_create();
    if (NULL == mon) {
        LOG("monitor initialization failed.");
        ecs_close(cs);
        return NULL;
    }

    cs->mon = mon;
    current_ecs = cs;
    cs->ecs_running = 1;

    LOG("ecs_loop entered.");
    while (cs->ecs_running) {
        ret = ecs_loop(cs);
        if (0 > ret) {
            ecs_close(cs);
            break;
        }
    }
    LOG("ecs_loop exited.");

    return NULL;
}

int stop_ecs(void) {
    LOG("ecs is closing.");
    if (NULL != current_ecs) {
        current_ecs->ecs_running = 0;
        ecs_close(current_ecs);
    }

    pthread_mutex_destroy(&mutex_clilist);

    return 0;
}

int start_ecs(void) {
    pthread_t thread_id;

    if (0 != pthread_create(&thread_id, NULL, ecs_initialize, NULL)) {
        LOG("pthread creation failed.");
        return -1;
    }
    return 0;
}

bool handle_protobuf_msg(ECS_Client* cli, char* data, int len)
{
    ECS__Master* master = ecs__master__unpack(NULL, (size_t)len, (const uint8_t*)data);
    if (!master)
        return false;

    if (master->type == ECS__MASTER__TYPE__INJECTOR_REQ)
    {
        ECS__InjectorReq* msg = master->injector_req;
        if (!msg)
            goto fail;
        msgproc_injector_req(cli, msg);
    }
    else if (master->type == ECS__MASTER__TYPE__MONITOR_REQ)
    {
        ECS__MonitorReq* msg = master->monitor_req;
        if (!msg)
            goto fail;
        msgproc_monitor_req(cli, msg);
    }
    else if (master->type == ECS__MASTER__TYPE__DEVICE_REQ)
    {
        ECS__DeviceReq* msg = master->device_req;
        if (!msg)
            goto fail;
        msgproc_device_req(cli, msg);
    }
    else if (master->type == ECS__MASTER__TYPE__NFC_REQ)
    {
        ECS__NfcReq* msg = master->nfc_req;
        if (!msg)
            goto fail;
        msgproc_nfc_req(cli, msg);
    }

    ecs__master__free_unpacked(master, NULL);
    return true;
fail:
    LOG("invalid message type");
    ecs__master__free_unpacked(master, NULL);
    return false;
} 

