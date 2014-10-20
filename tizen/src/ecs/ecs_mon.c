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

#include "qemu-common.h"
#include "qemu/queue.h"
#include "qemu/sockets.h"
#include "qemu/option.h"

#include <monitor/monitor.h>
#include "qmp-commands.h"
#include "qapi/qmp/qjson.h"
#include "qapi/qmp/json-parser.h"

#include "ecs.h"
#include "hw/virtio/maru_virtio_evdi.h"
#include "hw/virtio/maru_virtio_sensor.h"
#include "hw/virtio/maru_virtio_nfc.h"

#include "debug_ch.h"
MULTI_DEBUG_CHANNEL(qemu, ecs);

typedef struct mon_fd_t mon_fd_t;
struct mon_fd_t {
    char *name;
    int fd;
    QLIST_ENTRY(mon_fd_t)
    next;
};

typedef struct mon_cmd_t {
    const char *name;
    const char *args_type;
    const char *params;
    const char *help;
    void (*user_print)(Monitor *mon, const QObject *data);
    union {
        void (*info)(Monitor *mon);
        void (*cmd)(Monitor *mon, const QDict *qdict);
        int (*cmd_new)(Monitor *mon, const QDict *params, QObject **ret_data);
        int (*cmd_async)(Monitor *mon, const QDict *params,
                MonitorCompletion *cb, void *opaque);
    } mhandler;
    int flags;
} mon_cmd_t;

/*
void send_to_client(int fd, const char* data, const int len)
{
    char c;
    uint8_t outbuf[OUT_BUF_SIZE];
    int outbuf_index = 0;

    for (;;) {
        c = *data++;
        if (outbuf_index >= OUT_BUF_SIZE - 1) {
            INFO("string is too long: overflow buffer.");
            return;
        }
#ifndef _WIN32
        if (c == '\n') {
            outbuf[outbuf_index++] = '\r';
        }
#endif
        outbuf[outbuf_index++] = c;
        if (c == '\0') {
            break;
        }
    }
    ecs_write(fd, outbuf, outbuf_index);
}
*/

#define QMP_ACCEPT_UNKNOWNS 1

bool send_monitor_ntf(const char* data, int size)
{
    ECS__Master master = ECS__MASTER__INIT;
    ECS__MonitorNtf ntf = ECS__MONITOR_NTF__INIT;

    TRACE("data size : %d, data : %s\n", size, data);

    ntf.command = (char*) g_malloc(size + 1);
    memcpy(ntf.command, data, size);

    master.type = ECS__MASTER__TYPE__MONITOR_NTF;
    master.monitor_ntf = &ntf;

    pb_to_all_clients(&master);

    if (ntf.command)
        g_free(ntf.command);

    return true;
}

static void ecs_monitor_flush(ECS_Client *clii, Monitor *mon) {
    int ret;

    if (clii && 0 < clii->client_fd && mon && mon->outbuf_index != 0) {
        //ret = ecs_write(clii->client_fd, mon->outbuf, mon->outbuf_index);
        ret = send_monitor_ntf((char*)mon->outbuf, mon->outbuf_index);
        mon->outbuf_index = 0;
        if (ret < -1) {
            ecs_client_close(clii);
        }
    }
}

static void ecs_monitor_puts(ECS_Client *clii, Monitor *mon, const char *str) {
    char c;

    if (!clii || !mon) {
        return;
    }

    for (;;) {
        c = *str++;
        if (c == '\0')
            break;
#ifndef _WIN32
        if (c == '\n')
            mon->outbuf[mon->outbuf_index++] = '\r';
#endif
        mon->outbuf[mon->outbuf_index++] = c;
        if (mon->outbuf_index >= (sizeof(mon->outbuf) - 1) || c == '\n')
            ecs_monitor_flush(clii, mon);
    }
}

static inline int monitor_has_error(const Monitor *mon) {
    return mon->error != NULL;
}

static QDict *build_qmp_error_dict(const QError *err) {
    QObject *obj = qobject_from_jsonf(
            "{ 'error': { 'class': %s, 'desc': %p } }",
            ErrorClass_lookup[err->err_class], qerror_human(err));

    return qobject_to_qdict(obj);
}

static void ecs_json_emitter(ECS_Client *clii, const QObject *data) {
    QString *json;

    json = qobject_to_json(data);

    assert(json != NULL);

    qstring_append_chr(json, '\n');
    ecs_monitor_puts(clii, clii->cs->mon, qstring_get_str(json));

    QDECREF(json);
}

static void ecs_protocol_emitter(ECS_Client *clii, const char* type,
        QObject *data) {
    QDict *qmp;
    QObject *obj;

    TRACE("ecs_protocol_emitter called.\n");
    //trace_monitor_protocol_emitter(clii->cs->mon);

    if (!monitor_has_error(clii->cs->mon)) {
        /* success response */
        qmp = qdict_new();
        if (data) {
            qobject_incref(data);
            qdict_put_obj(qmp, "return", data);
        } else {
            /* return an empty QDict by default */
            qdict_put(qmp, "return", qdict_new());
        }

        if (type == NULL) {
            obj = qobject_from_jsonf("%s", "unknown");
        } else {
            obj = qobject_from_jsonf("%s", type);
        }
        qdict_put_obj(qmp, "type", obj);

    } else {
        /* error response */
        qmp = build_qmp_error_dict(clii->cs->mon->error);
        QDECREF(clii->cs->mon->error);
        clii->cs->mon->error = NULL;
    }

    ecs_json_emitter(clii, QOBJECT(qmp));
    QDECREF(qmp);
}

static void qmp_monitor_complete(void *opaque, QObject *ret_data) {
    //   ecs_protocol_emitter(opaque, ret_data);
}

static int ecs_qmp_async_cmd_handler(ECS_Client *clii, const mon_cmd_t *cmd,
        const QDict *params) {
    return cmd->mhandler.cmd_async(clii->cs->mon, params, qmp_monitor_complete,
            clii);
}

static void ecs_qmp_call_cmd(ECS_Client *clii, Monitor *mon, const char* type,
        const mon_cmd_t *cmd, const QDict *params) {
    int ret;
    QObject *data = NULL;

    ret = cmd->mhandler.cmd_new(mon, params, &data);
    if (ret && !monitor_has_error(mon)) {
        qerror_report(QERR_UNDEFINED_ERROR);
    }
    ecs_protocol_emitter(clii, type, data);
    qobject_decref(data);
}

static inline bool handler_is_async(const mon_cmd_t *cmd) {
    return cmd->flags & MONITOR_CMD_ASYNC;
}

static void monitor_user_noop(Monitor *mon, const QObject *data) {
}

static int client_migrate_info(Monitor *mon, const QDict *qdict,
        MonitorCompletion cb, void *opaque) {
    return 0;
}

static int do_device_add(Monitor *mon, const QDict *qdict, QObject **ret_data) {
    return 0;
}

static int do_qmp_capabilities(Monitor *mon, const QDict *params,
        QObject **ret_data) {
    return 0;
}

static const mon_cmd_t qmp_cmds[] = {
#include "qmp-commands-old.h"
        { /* NULL */}, };

static int check_mandatory_args(const QDict *cmd_args, const QDict *client_args,
        int *flags) {
    const QDictEntry *ent;

    for (ent = qdict_first(cmd_args); ent; ent = qdict_next(cmd_args, ent)) {
        const char *cmd_arg_name = qdict_entry_key(ent);
        QString *type = qobject_to_qstring(qdict_entry_value(ent));
        assert(type != NULL);

        if (qstring_get_str(type)[0] == 'O') {
            assert((*flags & QMP_ACCEPT_UNKNOWNS) == 0);
            *flags |= QMP_ACCEPT_UNKNOWNS;
        } else if (qstring_get_str(type)[0] != '-'
                && qstring_get_str(type)[1] != '?'
                && !qdict_haskey(client_args, cmd_arg_name)) {
            qerror_report(QERR_MISSING_PARAMETER, cmd_arg_name);
            return -1;
        }
    }

    return 0;
}

static int check_client_args_type(const QDict *client_args,
        const QDict *cmd_args, int flags) {
    const QDictEntry *ent;

    for (ent = qdict_first(client_args); ent;
            ent = qdict_next(client_args, ent)) {
        QObject *obj;
        QString *arg_type;
        const QObject *client_arg = qdict_entry_value(ent);
        const char *client_arg_name = qdict_entry_key(ent);

        obj = qdict_get(cmd_args, client_arg_name);
        if (!obj) {
            if (flags & QMP_ACCEPT_UNKNOWNS) {
                continue;
            }
            qerror_report(QERR_INVALID_PARAMETER, client_arg_name);
            return -1;
        }

        arg_type = qobject_to_qstring(obj);
        assert(arg_type != NULL);

        switch (qstring_get_str(arg_type)[0]) {
        case 'F':
        case 'B':
        case 's':
            if (qobject_type(client_arg) != QTYPE_QSTRING) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                        "string");
                return -1;
            }
            break;
        case 'i':
        case 'l':
        case 'M':
        case 'o':
            if (qobject_type(client_arg) != QTYPE_QINT) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                        "int");
                return -1;
            }
            break;
        case 'T':
            if (qobject_type(client_arg) != QTYPE_QINT
                    && qobject_type(client_arg) != QTYPE_QFLOAT) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                        "number");
                return -1;
            }
            break;
        case 'b':
        case '-':
            if (qobject_type(client_arg) != QTYPE_QBOOL) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                        "bool");
                return -1;
            }
            break;
        case 'O':
            assert(flags & QMP_ACCEPT_UNKNOWNS);
            break;
        case 'q':
            break;
        case '/':
        case '.':
        default:
            abort();
        }
    }

    return 0;
}

static QDict *qdict_from_args_type(const char *args_type) {
    int i;
    QDict *qdict;
    QString *key, *type, *cur_qs;

    assert(args_type != NULL);

    qdict = qdict_new();

    if (args_type == NULL || args_type[0] == '\0') {
        goto out;
    }

    key = qstring_new();
    type = qstring_new();

    cur_qs = key;

    for (i = 0;; i++) {
        switch (args_type[i]) {
        case ',':
        case '\0':
            qdict_put(qdict, qstring_get_str(key), type);
            QDECREF(key);
            if (args_type[i] == '\0') {
                goto out;
            }
            type = qstring_new();
            cur_qs = key = qstring_new();
            break;
        case ':':
            cur_qs = type;
            break;
        default:
            qstring_append_chr(cur_qs, args_type[i]);
            break;
        }
    }

    out: return qdict;
}

static int qmp_check_client_args(const mon_cmd_t *cmd, QDict *client_args) {
    int flags, err;
    QDict *cmd_args;

    cmd_args = qdict_from_args_type(cmd->args_type);

    flags = 0;
    err = check_mandatory_args(cmd_args, client_args, &flags);
    if (err) {
        goto out;
    }

    err = check_client_args_type(client_args, cmd_args, flags);

    out:
    QDECREF(cmd_args);
    return err;
}

static QDict *qmp_check_input_obj(QObject *input_obj) {
    const QDictEntry *ent;
    int has_exec_key = 0;
    QDict *input_dict;

    if (qobject_type(input_obj) != QTYPE_QDICT) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "object");
        return NULL;
    }

    input_dict = qobject_to_qdict(input_obj);

    for (ent = qdict_first(input_dict); ent;
            ent = qdict_next(input_dict, ent)) {
        const char *arg_name = qdict_entry_key(ent);
        const QObject *arg_obj = qdict_entry_value(ent);

        if (!strcmp(arg_name, "execute")) {
            if (qobject_type(arg_obj) != QTYPE_QSTRING) {
                qerror_report(QERR_QMP_BAD_INPUT_OBJECT_MEMBER, "execute",
                        "string");
                return NULL;
            }
            has_exec_key = 1;
        } else if (!strcmp(arg_name, "arguments")) {
            if (qobject_type(arg_obj) != QTYPE_QDICT) {
                qerror_report(QERR_QMP_BAD_INPUT_OBJECT_MEMBER, "arguments",
                        "object");
                return NULL;
            }
        } else if (!strcmp(arg_name, "id")) {
        } else if (!strcmp(arg_name, "type")) {
        } else {
            qerror_report(QERR_QMP_EXTRA_MEMBER, arg_name);
            return NULL;
        }
    }

    if (!has_exec_key) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "execute");
        return NULL;
    }

    return input_dict;
}

static int compare_cmd(const char *name, const char *list) {
    const char *p, *pstart;
    int len;
    len = strlen(name);
    p = list;
    for (;;) {
        pstart = p;
        p = strchr(p, '|');
        if (!p)
            p = pstart + strlen(pstart);
        if ((p - pstart) == len && !memcmp(pstart, name, len))
            return 1;
        if (*p == '\0')
            break;
        p++;
    }
    return 0;
}

static const mon_cmd_t *search_dispatch_table(const mon_cmd_t *disp_table,
        const char *cmdname) {
    const mon_cmd_t *cmd;

    for (cmd = disp_table; cmd->name != NULL; cmd++) {
        if (compare_cmd(cmdname, cmd->name)) {
            return cmd;
        }
    }

    return NULL;
}

static const mon_cmd_t *qmp_find_cmd(const char *cmdname) {
    return search_dispatch_table(qmp_cmds, cmdname);
}

void handle_qmp_command(JSONMessageParser *parser, QList *tokens,
        void *opaque) {
    int err;
    QObject *obj;
    QDict *input, *args;
    const mon_cmd_t *cmd;
    const char *cmd_name;
    const char *type_name;
    Monitor *mon = cur_mon;
    ECS_Client *clii = opaque;

    args = input = NULL;

    obj = json_parser_parse(tokens, NULL);
    if (!obj) {
        // FIXME: should be triggered in json_parser_parse()
        qerror_report(QERR_JSON_PARSING);
        goto err_out;
    }

    input = qmp_check_input_obj(obj);
    if (!input) {
        qobject_decref(obj);
        goto err_out;
    }

    cmd_name = qdict_get_str(input, "execute");
    //trace_handle_qmp_command(mon, cmd_name);

    cmd = qmp_find_cmd(cmd_name);
    if (!cmd) {
        qerror_report(QERR_COMMAND_NOT_FOUND, cmd_name);
        goto err_out;
    }

    type_name = qdict_get_str(qobject_to_qdict(obj), COMMANDS_TYPE);

    obj = qdict_get(input, "arguments");
    if (!obj) {
        args = qdict_new();
    } else {
        args = qobject_to_qdict(obj);
        QINCREF(args);
    }

    err = qmp_check_client_args(cmd, args);
    if (err < 0) {
        goto err_out;
    }

    if (handler_is_async(cmd)) {
        err = ecs_qmp_async_cmd_handler(clii, cmd, args);
        if (err) {
            /* emit the error response */
            goto err_out;
        }
    } else {
        ecs_qmp_call_cmd(clii, mon, type_name, cmd, args);
    }

    goto out;

err_out:
    ecs_protocol_emitter(clii, NULL, NULL);
out:
    QDECREF(input);
    QDECREF(args);
}

bool msgproc_monitor_req(ECS_Client *ccli, ECS__MonitorReq* msg)
{
    TRACE(">> monitor req: data = %s\n", msg->command);
    ecs_json_message_parser_feed(&(ccli->parser), (const char *) msg->command, strlen(msg->command));
    return true;
}

