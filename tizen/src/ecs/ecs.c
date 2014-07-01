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

#include "emulator.h"
#include "util/sdb.h"
#include "ecs.h"
#include "emul_state.h"

#include "genmsg/ecs.pb-c.h"

#include "debug_ch.h"
MULTI_DEBUG_CHANNEL(qemu, ecs);

#define DEBUG

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

static QTAILQ_HEAD(ECS_ClientHead, ECS_Client)
clients = QTAILQ_HEAD_INITIALIZER(clients);

static ECS_State *current_ecs;

static void* keepalive_buf;
static int payloadsize;

static int g_client_id = 1;

static QemuMutex mutex_clilist;
QemuMutex mutex_guest_connection;

static QemuThread ecs_thread_id;

static int suspend_state = 1;

void ecs_set_suspend_state(int state)
{
    suspend_state = state;
}

int ecs_get_suspend_state(void)
{
    return suspend_state;
}

int ecs_write(int fd, const uint8_t *buf, int len) {
    TRACE("write buflen : %d, buf : %s\n", len, (char*)buf);
    if (fd < 0) {
        return -1;
    }

    return send_all(fd, buf, len);
}

void ecs_client_close(ECS_Client* clii) {
    if (clii == NULL)
        return;

    qemu_mutex_lock(&mutex_clilist);

    if (clii->client_fd > 0) {
        INFO("ecs client closed with fd: %d\n", clii->client_fd);
        closesocket(clii->client_fd);
#ifndef CONFIG_LINUX
        FD_CLR(clii->client_fd, &clii->cs->reads);
#endif
        clii->client_fd = -1;
    }

    QTAILQ_REMOVE(&clients, clii, next);

    g_free(clii);
    clii = NULL;

    qemu_mutex_unlock(&mutex_clilist);
}

bool send_to_all_client(const char* data, const int len) {
    TRACE("data len: %d, data: %s\n", len, data);
    qemu_mutex_lock(&mutex_clilist);

    ECS_Client *clii,*next;

    QTAILQ_FOREACH_SAFE(clii, &clients, next, next)
    {
        send_to_client(clii->client_fd, data, len);
    }
    qemu_mutex_unlock(&mutex_clilist);

    return true;
}

void send_to_single_client(ECS_Client *clii, const char* data, const int len)
{
    qemu_mutex_lock(&mutex_clilist);
    send_to_client(clii->client_fd, data, len);
    qemu_mutex_unlock(&mutex_clilist);
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

void print_binary(const char* data, const int len) {
    int i;
    printf("[DATA: ");
    for(i = 0; i < len; i++) {
        if(i == len - 1) {
            printf("%02x]\n", data[i]);
        } else {
            printf("%02x,", data[i]);
        }
    }
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
        ERR("monitor allocation failed.\n");
        return NULL;
    }

    return mon;
}

static void ecs_close(ECS_State *cs) {
    ECS_Client *clii, *next;
    INFO("### Good bye! ECS ###\n");

    if (cs == NULL)
        return;

    if (0 <= cs->listen_fd) {
        INFO("close listen_fd: %d\n", cs->listen_fd);
        closesocket(cs->listen_fd);
        cs->listen_fd = -1;
    }

    if (cs->mon != NULL) {
        g_free(cs->mon);
        cs->mon = NULL;
    }

    if (keepalive_buf) {
        g_free(keepalive_buf);
    }

    if (cs->alive_timer != NULL) {
        timer_del(cs->alive_timer);
        cs->alive_timer = NULL;
    }

    QTAILQ_FOREACH_SAFE(clii, &clients, next, next)
    {
        ecs_client_close(clii);
    }

    g_free(cs);
    cs = NULL;
    current_ecs = NULL;

    qemu_mutex_destroy(&mutex_clilist);
    qemu_mutex_destroy(&mutex_guest_connection);
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

    if (cli == NULL)
    {
        ERR("client is null.\n");
        return;
    }
#ifndef __WIN32
    if (ioctl(cli->client_fd, FIONREAD, &to_read_bytes) < 0)
    {
        ERR("ioctl failed\n");
        return;
    }
#else
    unsigned long to_read_bytes_long = 0;
    if (ioctlsocket(cli->client_fd, FIONREAD, &to_read_bytes_long) < 0)
    {
        ERR("ioctl failed\n");
         return;
    }
    to_read_bytes = (int)to_read_bytes_long;
#endif

    if (to_read_bytes == 0) {
        TRACE("ioctl FIONREAD: 0\n");
        goto fail;
    }

    if (cli->sbuf._netlen == 0)
    {
        if (to_read_bytes < 4)
        {
            //LOG("insufficient data size to read\n");
            return;
        }

        long payloadsize = 0;
        read = ecs_recv(cli->client_fd, (char*) &payloadsize, 4);

        if (read < 4)
        {
            ERR("insufficient header size\n");
            goto fail;
        }

        payloadsize = ntohl(payloadsize);

        cli->sbuf._netlen = payloadsize;

        TRACE("payload size: %ld\n", payloadsize);

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
        ERR("Epoll control fails.in epoll_cli_add.\n");
    }
}
#endif

static ECS_Client *ecs_find_client(int fd) {
    ECS_Client *clii, *next;

    QTAILQ_FOREACH_SAFE(clii, &clients, next, next)
    {
        if (clii->client_fd == fd)
            return clii;
    }
    return NULL;
}

ECS_Client *find_client(unsigned char id, unsigned char type) {
    ECS_Client *clii, *next;

    QTAILQ_FOREACH_SAFE(clii, &clients, next, next)
    {
        if (clii->client_id == id && clii->client_type == type)
            return clii;
    }
    return NULL;
}

static int ecs_add_client(ECS_State *cs, int fd) {

    ECS_Client *clii = g_malloc0(sizeof(ECS_Client));
    if (NULL == clii) {
        ERR("ECS_Client allocation failed.\n");
        return -1;
    }

    reset_sbuf(&clii->sbuf);

    qemu_set_nonblock(fd);

    clii->client_fd = fd;
    clii->cs = cs;
    clii->client_type = TYPE_NONE;

    ecs_json_message_parser_init(&clii->parser, handle_qmp_command, clii);

#ifdef CONFIG_LINUX
    epoll_cli_add(cs, fd);
#else
    FD_SET(fd, &cs->reads);
#endif

    qemu_mutex_lock(&mutex_clilist);

    QTAILQ_INSERT_TAIL(&clients, clii, next);

    TRACE("Add an ecs client. fd: %d\n", fd);

    qemu_mutex_unlock(&mutex_clilist);

//    send_ecs_version_check(clii);

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
        ERR("failed to add client.\n");
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

static void send_keep_alive_msg(ECS_Client *clii) {
    send_to_single_client(clii, keepalive_buf, payloadsize);
}

static void make_keep_alive_msg(void) {
    int len_pack = 0;
    char msg [5] = {'s','e','l','f'};

    ECS__Master master = ECS__MASTER__INIT;
    ECS__KeepAliveReq req = ECS__KEEP_ALIVE_REQ__INIT;

    req.time_str = (char*) g_malloc(5);

    strncpy(req.time_str, msg, 4);

    master.type = ECS__MASTER__TYPE__KEEPALIVE_REQ;
    master.keepalive_req = &req;

    len_pack = ecs__master__get_packed_size(&master);
    payloadsize = len_pack + 4;

    keepalive_buf = g_malloc(len_pack + 4);
    if (!keepalive_buf) {
        ERR("keep alive message creation is failed.\n");
        return;
    }

    ecs__master__pack(&master, keepalive_buf + 4);

    len_pack = htonl(len_pack);
    memcpy(keepalive_buf, &len_pack, 4);
}

static void alive_checker(void *opaque) {

    ECS_Client *clii, *next;

    if (NULL != current_ecs && !current_ecs->ecs_running) {
        return;
    }

    QTAILQ_FOREACH_SAFE(clii, &clients, next, next)
    {
        if (1 == clii->keep_alive) {
            INFO("get client fd %d - keep alive fail\n", clii->client_fd);
            ecs_client_close(clii);
            continue;
        }
        TRACE("set client fd %d - keep alive 1\n", clii->client_fd);
        clii->keep_alive = 1;
        send_keep_alive_msg(clii);
    }

    if (current_ecs == NULL) {
        ERR("alive checking is failed because current ecs is null.\n");
        return;
    }

    timer_mod(current_ecs->alive_timer,
            qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + get_ticks_per_sec() * TIMER_ALIVE_S);

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

    INFO("Listen fd is %d\n", fd);

    qemu_set_nonblock(fd);

    cs->listen_fd = fd;

#ifdef CONFIG_LINUX
    epoll_init(cs);
#else
    FD_ZERO(&cs->reads);
    FD_SET(fd, &cs->reads);
#endif

    make_keep_alive_msg();

    cs->alive_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, alive_checker, cs);

    timer_mod(cs->alive_timer,
            qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + get_ticks_per_sec() * TIMER_ALIVE_S);

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
        ERR("select error.\n");
        return -1;
    }

    for (index = 0; index < cs->reads.fd_count; index++) {
        if (cs->reads.fd_array == NULL)
            continue;
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
        ERR("select failed..\n");
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

static void* ecs_initialize(void* args) {
    int ret = 1;
    ECS_State *cs = NULL;
    QemuOpts *opts = NULL;
    Error *local_err = NULL;
    Monitor* mon = NULL;
    char host_port[16];
    int port = 0;

    INFO("ecs starts initializing.\n");

    opts = qemu_opts_create(qemu_find_opts(ECS_OPTS_NAME), ECS_OPTS_NAME, 1, &local_err);
    if (error_is_set(&local_err)) {
        qerror_report_err(local_err);
        error_free(local_err);
        return NULL;
    }

    qemu_opt_set(opts, "host", HOST_LISTEN_ADDR);

    cs = g_malloc0(sizeof(ECS_State));
    if (NULL == cs) {
        ERR("ECS_State allocation failed.\n");
        return NULL;
    }
    port = get_emul_ecs_port();
    INFO("ecs port: %d\n", port);
    sprintf(host_port, "%d", port);

    qemu_opt_set(opts, "port", host_port);
    ret = socket_initialize(cs, opts);
    if (ret < 0) {
        ERR("Socket initialization is failed.\n");
        ecs_close(cs);
        return NULL;
    }

    mon = monitor_create();
    if (NULL == mon) {
        ERR("monitor initialization failed.\n");
        ecs_close(cs);
        return NULL;
    }

    cs->mon = mon;
    current_ecs = cs;
    cs->ecs_running = 1;

    qemu_mutex_init(&mutex_clilist);
    qemu_mutex_init(&mutex_guest_connection);

    TRACE("ecs_loop entered.\n");
    while (cs->ecs_running) {
        ret = ecs_loop(cs);
        if (0 > ret) {
            break;
        }
    }
    TRACE("ecs_loop exited.\n");

    ecs_close(cs);

    return NULL;
}

static int stop_ecs(void) {
    void *ret = NULL;

    INFO("ecs is closing.\n");
    if (NULL != current_ecs) {
        current_ecs->ecs_running = 0;
    }

    ret = qemu_thread_join(&ecs_thread_id);
    if (ret) {
        ERR("ecs is failed to join thread.\n");
        return -1;
    }

    return 0;
}

static void ecs_notify_exit(Notifier *notifier, void *data) {
    stop_ecs();
}
static Notifier ecs_exit = { .notify = ecs_notify_exit };

int start_ecs(void) {
    qemu_thread_create(&ecs_thread_id, "ecs", ecs_initialize, NULL, QEMU_THREAD_JOINABLE);

    emulator_add_exit_notifier(&ecs_exit);

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
        cli->client_type = TYPE_ECP;
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

        qemu_mutex_lock(&mutex_clilist);
        if(cli->client_type == TYPE_NONE) {
            if (!strncmp(msg->category, MSG_TYPE_NFC, 3)) {
                QTAILQ_REMOVE(&clients, cli, next);
                cli->client_type = TYPE_ECP;
                if(g_client_id > 255) {
                    g_client_id = 1;
                }
                cli->client_id = g_client_id++;

                QTAILQ_INSERT_TAIL(&clients, cli, next);
            }
            else if (!strncmp(msg->category, MSG_TYPE_SIMUL_NFC, 9)) {
                QTAILQ_REMOVE(&clients, cli, next);
                cli->client_type = TYPE_SIMUL_NFC;
                if(g_client_id > 255) {
                    g_client_id = 1;
                }
                cli->client_id = g_client_id++;
                QTAILQ_INSERT_TAIL(&clients, cli, next);
            }
            else {
                ERR("unsupported category is found: %s\n", msg->category);
                qemu_mutex_unlock(&mutex_clilist);
                goto fail;
            }
        }
        qemu_mutex_unlock(&mutex_clilist);

        msgproc_nfc_req(cli, msg);
    }
#if 0
    else if (master->type == ECS__MASTER__TYPE__CHECKVERSION_REQ)
    {
        ECS__CheckVersionReq* msg = master->checkversion_req;
        if (!msg)
            goto fail;
        msgproc_checkversion_req(cli, msg);
    }
#endif
    else if (master->type == ECS__MASTER__TYPE__KEEPALIVE_ANS)
    {
        ECS__KeepAliveAns* msg = master->keepalive_ans;
        if (!msg)
            goto fail;
        msgproc_keepalive_ans(cli, msg);
    }
    else if (master->type == ECS__MASTER__TYPE__TETHERING_REQ)
    {
        ECS__TetheringReq* msg = master->tethering_req;
        if (!msg)
            goto fail;
        msgproc_tethering_req(cli, msg);
    }

    ecs__master__free_unpacked(master, NULL);
    return true;
fail:
    ERR("invalid message type : %d\n", master->type);
    ecs__master__free_unpacked(master, NULL);
    return false;
}
