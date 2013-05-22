
#include "hw/qdev.h"
#include "net.h"
#include "console.h"
#include "migration.h"

#include "qemu-common.h"
#include "qemu_socket.h"
#include "qemu-queue.h"
#include "qemu-option.h"
#include "qemu-config.h"
#include "qemu-timer.h"
#include "main-loop.h"
#include "ui/qemu-spice.h"
#include "qemu-char.h"
#include "sdb.h"
#include "qjson.h"

#include "json-parser.h"
#include "qmp-commands.h"
#include "qint.h"
#include "ecs.h"
#include "hw/maru_virtio_evdi.h"
#include <stdbool.h>
#include <pthread.h>



#define DEBUG

typedef struct mon_fd_t mon_fd_t;
struct mon_fd_t {
    char *name;
    int fd;
    QLIST_ENTRY(mon_fd_t) next;
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
        int  (*cmd_new)(Monitor *mon, const QDict *params, QObject **ret_data);
        int  (*cmd_async)(Monitor *mon, const QDict *params,
                          MonitorCompletion *cb, void *opaque);
    } mhandler;
    int flags;
} mon_cmd_t;

static QTAILQ_HEAD(ECS_ClientHead, ECS_Client) clients =
    QTAILQ_HEAD_INITIALIZER(clients);

static ECS_State *current_ecs;

static pthread_mutex_t mutex_clilist = PTHREAD_MUTEX_INITIALIZER;

static inline void start_logging(void)
{
#ifndef _WIN32
	char* home;
	char path [256];
	int fd = open("/dev/null", O_RDONLY);
	dup2(fd, 0);

	home = getenv(LOG_HOME);
	sprintf(path, "%s%s", home, LOG_PATH);
	fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0640);
	if(fd < 0) {
		fd = open("/dev/null", O_WRONLY);
	}
	dup2(fd, 1);
	dup2(fd, 2);
#endif
}

static int ecs_write(int fd, const uint8_t *buf, int len);

static void ecs_client_close(ECS_Client* clii)
{
	pthread_mutex_lock(&mutex_clilist);

	if (0 <= clii->client_fd) {
		LOG("ecs client closed with fd: %d", clii->client_fd);
		closesocket(clii->client_fd);
		clii->client_fd = -1;
	}
	QTAILQ_REMOVE(&clients, clii, next);
	if (NULL != clii) {
		g_free(clii);
	}

	pthread_mutex_unlock(&mutex_clilist);
}


bool send_to_all_client(const char* data, const int len)
{
	pthread_mutex_lock(&mutex_clilist);

	ECS_Client *clii;

	QTAILQ_FOREACH(clii, &clients, next)
	{
		send_to_client(clii->client_fd, data);
	}
	pthread_mutex_unlock(&mutex_clilist);

	return true;
}

void send_to_client(int fd, const char *str)
{
    char c;
	uint8_t outbuf[OUT_BUF_SIZE];
	int outbuf_index = 0;

    for(;;) {
        c = *str++;
		if (outbuf_index >= OUT_BUF_SIZE -1) {
			LOG("string is too long: overflow buffer.");
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

#define QMP_ACCEPT_UNKNOWNS 1
static void ecs_monitor_flush(ECS_Client *clii, Monitor *mon)
{
	int ret;

    if (clii && 0 < clii->client_fd && mon && mon->outbuf_index != 0) {
        ret = ecs_write(clii->client_fd, mon->outbuf, mon->outbuf_index);
        mon->outbuf_index = 0;
		if (ret < -1) {
			ecs_client_close(clii);
		}
    }
}

static void ecs_monitor_puts(ECS_Client *clii, Monitor *mon, const char *str)
{
    char c;

	if (!clii || !mon) {
		return;
	}

    for(;;) {
        c = *str++;
        if (c == '\0')
            break;
#ifndef _WIN32
        if (c == '\n')
            mon->outbuf[mon->outbuf_index++] = '\r';
#endif
		mon->outbuf[mon->outbuf_index++] = c;
        if (mon->outbuf_index >= (sizeof(mon->outbuf) - 1)
            || c == '\n')
            ecs_monitor_flush(clii, mon);
    }
}

void ecs_vprintf(const char *type, const char *fmt, va_list ap)
{
    char buf[READ_BUF_LEN];
	ECS_Client *clii;

    QTAILQ_FOREACH(clii, &clients, next) {
		vsnprintf(buf, sizeof(buf), fmt, ap);
		ecs_monitor_puts(clii, clii->cs->mon, buf);
    }
}

void ecs_printf(const char* type, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ecs_vprintf(type, fmt, ap);
    va_end(ap);
}

static inline int monitor_has_error(const Monitor *mon)
{
    return mon->error != NULL;
}

static QDict *build_qmp_error_dict(const QError *err)
{
    QObject *obj = qobject_from_jsonf("{ 'error': { 'class': %s, 'desc': %p } }",
                             ErrorClass_lookup[err->err_class],
                             qerror_human(err));

    return qobject_to_qdict(obj);
}

static void ecs_json_emitter(ECS_Client *clii, const QObject *data)
{
    QString *json;

    json = qobject_to_json(data);

    assert(json != NULL);

	qstring_append_chr(json, '\n');
  	ecs_monitor_puts(clii, clii->cs->mon, qstring_get_str(json));

    QDECREF(json);
}

static void ecs_protocol_emitter(ECS_Client *clii, const char* type, QObject *data)
{
    QDict *qmp;
	QObject *obj;

	LOG("ecs_protocol_emitter called.");
    trace_monitor_protocol_emitter(clii->cs->mon);

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


static void qmp_monitor_complete(void *opaque, QObject *ret_data)
{
 //   ecs_protocol_emitter(opaque, ret_data);
}

static int qmp_async_cmd_handler(ECS_Client *clii, 
								const mon_cmd_t *cmd, const QDict *params)
{
    return cmd->mhandler.cmd_async(clii->cs->mon, params, qmp_monitor_complete, clii);
}

static void qmp_call_cmd(ECS_Client *clii, Monitor *mon, const char* type, 
								const mon_cmd_t *cmd, const QDict *params)
{
    int ret;
    QObject *data = NULL;

    ret = cmd->mhandler.cmd_new(mon, params, &data);
    if (ret && !monitor_has_error(mon)) {
        qerror_report(QERR_UNDEFINED_ERROR);
    }
    ecs_protocol_emitter(clii, type, data);
    qobject_decref(data);
}

static inline bool handler_is_async(const mon_cmd_t *cmd)
{
    return cmd->flags & MONITOR_CMD_ASYNC;
}

static void monitor_user_noop(Monitor *mon, const QObject *data) 
{ 
}

static int do_screen_dump(Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    vga_hw_screen_dump(qdict_get_str(qdict, "filename"));
    return 0;
}

static int client_migrate_info(Monitor *mon, const QDict *qdict,
                               MonitorCompletion cb, void *opaque)
{
    return 0;
}

static int add_graphics_client(Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    return 0;
}

static int do_qmp_capabilities(Monitor *mon, const QDict *params,
                               QObject **ret_data)
{
    return 0;
}

static const mon_cmd_t qmp_cmds[] = {
#include "qmp-commands-old.h"
    { /* NULL */ },
};

static int check_mandatory_args(const QDict *cmd_args,
                                const QDict *client_args, int *flags)
{
    const QDictEntry *ent;

    for (ent = qdict_first(cmd_args); ent; ent = qdict_next(cmd_args, ent)) {
        const char *cmd_arg_name = qdict_entry_key(ent);
        QString *type = qobject_to_qstring(qdict_entry_value(ent));
        assert(type != NULL);

        if (qstring_get_str(type)[0] == 'O') {
            assert((*flags & QMP_ACCEPT_UNKNOWNS) == 0);
            *flags |= QMP_ACCEPT_UNKNOWNS;
        } else if (qstring_get_str(type)[0] != '-' &&
                   qstring_get_str(type)[1] != '?' &&
                   !qdict_haskey(client_args, cmd_arg_name)) {
            qerror_report(QERR_MISSING_PARAMETER, cmd_arg_name);
            return -1;
        }
    }

    return 0;
}

static int check_client_args_type(const QDict *client_args,
                                  const QDict *cmd_args, int flags)
{
    const QDictEntry *ent;

    for (ent = qdict_first(client_args); ent;ent = qdict_next(client_args,ent)){
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
            if (qobject_type(client_arg) != QTYPE_QINT &&
                qobject_type(client_arg) != QTYPE_QFLOAT) {
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

static QDict *qdict_from_args_type(const char *args_type)
{
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

out:
    return qdict;
}

static int qmp_check_client_args(const mon_cmd_t *cmd, QDict *client_args)
{
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

static QDict *qmp_check_input_obj(QObject *input_obj)
{
    const QDictEntry *ent;
    int has_exec_key = 0;
    QDict *input_dict;

    if (qobject_type(input_obj) != QTYPE_QDICT) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "object");
        return NULL;
    }

    input_dict = qobject_to_qdict(input_obj);

    for (ent = qdict_first(input_dict); ent; ent = qdict_next(input_dict, ent)){
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

static int compare_cmd(const char *name, const char *list)
{
    const char *p, *pstart;
    int len;
    len = strlen(name);
    p = list;
    for(;;) {
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
                                              const char *cmdname)
{
    const mon_cmd_t *cmd;

    for (cmd = disp_table; cmd->name != NULL; cmd++) {
        if (compare_cmd(cmdname, cmd->name)) {
            return cmd;
        }
    }

    return NULL;
}

static const mon_cmd_t *qmp_find_cmd(const char *cmdname)
{
    return search_dispatch_table(qmp_cmds, cmdname);
}

static void handle_qmp_command(ECS_Client *clii, const char* type_name, QObject *obj)
{
    int err;
    const mon_cmd_t *cmd;
    const char *cmd_name;
    QDict *input = NULL;
	QDict *args = NULL;

	input = qmp_check_input_obj(obj);
    if (!input) {
        qobject_decref(obj);
        goto err_out;
    }

    cmd_name = qdict_get_str(input, "execute");

	LOG("execute exists.");
    cmd = qmp_find_cmd(cmd_name);
    if (!cmd) {
        qerror_report(QERR_COMMAND_NOT_FOUND, cmd_name);
        goto err_out;
    }
	
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

	LOG("argument exists.");
    if (handler_is_async(cmd)) {
        err = qmp_async_cmd_handler(clii, cmd, args);
        if (err) {
            goto err_out;
        }
    } else {
		LOG("qmp_call_cmd called client fd: %d", clii->client_fd);
        qmp_call_cmd(clii, clii->cs->mon, type_name, cmd, args);
    }

    goto out;

err_out:
    ecs_protocol_emitter(clii, type_name, NULL);
out:
    QDECREF(input);
    QDECREF(args);

}

static int check_key(QObject *input_obj, const char *key)
{
    const QDictEntry *ent;
    QDict *input_dict;

    if (qobject_type(input_obj) != QTYPE_QDICT) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "object");
        return -1;
    }

    input_dict = qobject_to_qdict(input_obj);

    for (ent = qdict_first(input_dict); ent; ent = qdict_next(input_dict, ent)){
        const char *arg_name = qdict_entry_key(ent);
        if (!strcmp(arg_name, key)) {
        	return 1;
        }
    }

    return 0;
}

static QObject* get_data_object(QObject *input_obj)
{
    const QDictEntry *ent;
    QDict *input_dict;

    if (qobject_type(input_obj) != QTYPE_QDICT) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "object");
        return NULL;
    }

    input_dict = qobject_to_qdict(input_obj);

    for (ent = qdict_first(input_dict); ent; ent = qdict_next(input_dict, ent)){
        const char *arg_name = qdict_entry_key(ent);
        QObject *arg_obj = qdict_entry_value(ent);
        if (!strcmp(arg_name, COMMANDS_DATA)) {
        	return arg_obj;
        }
    }

    return NULL;
}


void read_val_short(const char* data, unsigned short* ret_val)
{
	memcpy(ret_val, data, sizeof(unsigned short));
}

void read_val_char(const char* data, unsigned char* ret_val)
{
	memcpy(ret_val, data, sizeof(unsigned char));
}

void read_val_str(const char* data, char* ret_val, int len)
{
	memcpy(ret_val, data, len);
}

void make_header(QDict* obj, type_length length, type_group group, type_action action)
{
	qdict_put(obj, "length",  qint_from_int((int64_t)length));
	qdict_put(obj, "group",  qint_from_int((int64_t)group));
	qdict_put(obj, "action",  qint_from_int((int64_t)action));
}


bool ntf_to_injector(const char* data, const int len)
{
	type_length length = 0;
	type_group group = 0;
	type_action action = 0;


	const int catsize = 10;
	char cat[catsize + 1];
	memset(cat, 0, catsize + 1);

	read_val_str(data, cat, catsize);
	read_val_short(data + catsize, &length);
	read_val_char(data + catsize + 2, &group);
	read_val_char(data + catsize +  2 + 1, &action);

	LOG("<< header cat = %s, length = %d, action=%d, group=%d",
			cat, length, action, group);

	const char* ijdata = (data + catsize +  2 + 1 + 1);

	QDict* obj_header = qdict_new();
	make_header(obj_header, length, group, action);

	QDict* objData = qdict_new();
	qobject_incref(QOBJECT(obj_header));

	qdict_put(objData, "cat",  qstring_from_str(cat));
	qdict_put(objData, "header", obj_header);
	qdict_put(objData, "ijdata", qstring_from_str(ijdata));


	QDict* objMsg = qdict_new();
	qobject_incref(QOBJECT(objData));

	qdict_put(objMsg, "type", qstring_from_str("injector"));
	qdict_put(objMsg, "result", qstring_from_str("success"));
	qdict_put(objMsg, "data", objData);

    QString *json;
    json =  qobject_to_json(QOBJECT(objMsg));

    assert(json != NULL);

    qstring_append_chr(json, '\n');
    const char* snddata = qstring_get_str(json);

    LOG("<< json str = %s", snddata);

	send_to_all_client(snddata, strlen(snddata));

	QDECREF(json);

	QDECREF(obj_header);
	QDECREF(objData);
	QDECREF(objMsg);

	return true;
}

bool ntf_to_control(const char* data, const int len)
{
	return true;
}

bool ntf_to_monitor(const char* data, const int len)
{
	return true;
}

static int ijcount = 0;

static bool injector_command_proc(ECS_Client *clii, QObject *obj)
{
	QDict* header = qdict_get_qdict(qobject_to_qdict(obj), "header");

	char cmd[10];
	memset(cmd, 0, 10);
	strcpy(cmd, qdict_get_str(header, "cat"));
	type_length length = (type_length) qdict_get_int(header, "length");
	type_group  group = (type_action) (qdict_get_int(header, "group") & 0xff);
	type_action action = (type_group) (qdict_get_int(header, "action") & 0xff);


	// get data
	const char* data = qdict_get_str(qobject_to_qdict(obj), COMMANDS_DATA);
	LOG(">> count= %d", ++ijcount);
	LOG(">> print len = %d, data\" %s\"", strlen(data), data);
	LOG(">> header = cmd = %s, length = %d, action=%d, group=%d",
			cmd, length, action, group);

	int datalen = strlen(data);
	int sndlen = datalen + 14;
	char* sndbuf = (char*) malloc(sndlen + 1);
	if (!sndbuf)
		return false;

	memset(sndbuf, 0, sndlen + 1);

	// set data
	memcpy(sndbuf, cmd, 10);
	memcpy(sndbuf + 10, &length, 2);
	memcpy(sndbuf + 12, &group, 1);
	memcpy(sndbuf + 13, &action, 1);

	memcpy(sndbuf + 14, data, datalen);

	send_to_evdi(route_ij, sndbuf, sndlen);

	free(sndbuf);

	return true;
}

static bool control_command_proc(ECS_Client *clii, QObject *obj)
{
	int64_t control_type = qdict_get_int(qobject_to_qdict(obj), "control_type");

	QDict* data = qdict_get_qdict(qobject_to_qdict(obj), "data");

	if (control_type == CONTROL_COMMAND_HOST_KEYBOARD_ONOFF_REQ)
	{
		control_host_keyboard_onoff_req(clii, data);
	}
	else if (control_type == CONTROL_COMMAND_SCREENSHOT_REQ)
	{

	}
	//LOG(">> control : feature = %s, action=%d, data=%s", feature, action, data);

	return true;
}

static void handle_ecs_command(JSONMessageParser *parser, QList *tokens, void *opaque)
{
	const char *type_name;
	int def_target = 0;
	int def_data = 0;
    QObject *obj;
	ECS_Client *clii = opaque;

	if (NULL == clii) {
		LOG("ClientInfo is null.");
		return;
	}

#ifdef DEBUG
	LOG("Handle ecs command.");
#endif

    obj = json_parser_parse(tokens, NULL);
    if (!obj) {
        qerror_report(QERR_JSON_PARSING);
    	ecs_protocol_emitter(clii, NULL, NULL);
		return;
    }

	def_target = check_key(obj, COMMANDS_TYPE);
#ifdef DEBUG
	LOG("check_key(COMMAND_TYPE): %d", def_target);
#endif
	if (0 > def_target) {
		LOG("def_target failed.");
		return;
	} else if (0 == def_target) {
#ifdef DEBUG
		LOG("call handle_qmp_command");
#endif
		handle_qmp_command(clii, NULL, obj);
		return;
	}

	type_name = qdict_get_str(qobject_to_qdict(obj), COMMANDS_TYPE);

	/*
	def_data = check_key(obj, COMMANDS_DATA);
	if (0 > def_data) {
		LOG("json format error: data.");
		return;
	} else if (0 == def_data) {
		LOG("data key is not found.");
		return;
	}
	*/
	
	if (!strcmp(type_name, TYPE_DATA_SELF)) {
		LOG("set client fd %d keep alive 0", clii->client_fd);
		clii->keep_alive = 0;
		return;
	}
	else if (!strcmp(type_name, COMMAND_TYPE_INJECTOR)) {
		injector_command_proc(clii, obj);
	}
	else if (!strcmp(type_name, COMMAND_TYPE_CONTROL)) {
		control_command_proc(clii, obj);
	}
	else if (!strcmp(type_name, COMMAND_TYPE_MONITOR)) {
		handle_qmp_command(clii, type_name, get_data_object(obj));
	}
	else if (!strcmp(type_name, ECS_MSG_STARTINFO_REQ)){
		ecs_startinfo_req(clii);
	}
	else
	{
		LOG("handler not found");
	}
}

static Monitor *monitor_create(void)
{
    Monitor *mon;

    mon = g_malloc0(sizeof(*mon));
	if (NULL == mon) {
		LOG("monitor allocation failed.");
		return NULL;
	}
	memset(mon, 0, sizeof(*mon));

	return mon;
}

static int device_initialize(void)
{
	// currently nothing to do with it.
	return 1;
}

static void ecs_close(ECS_State *cs)
{
	ECS_Client *clii;
	LOG("### Good bye! ECS ###");

	if (0 <= cs->listen_fd) {
		closesocket(cs->listen_fd);
	}

	if (NULL != cs->mon) {
		g_free(cs->mon);
	}

	if (NULL != cs->alive_timer) {
		qemu_del_timer(cs->alive_timer);
		cs->alive_timer = NULL;
	}

	pthread_mutex_lock(&mutex_clilist);

    QTAILQ_FOREACH(clii, &clients, next) {
		ecs_client_close(clii);
	}
    pthread_mutex_unlock(&mutex_clilist);

	//TODO: device close

	if (NULL != cs) {
		g_free(cs);
	}
}

static int ecs_write(int fd, const uint8_t *buf, int len)
{
	LOG("write buflen : %d, buf : %s", len, buf);
	if (fd < 0) {
		return -1;
	}

	return send_all(fd, buf, len);
}

#ifndef _WIN32
static ssize_t ecs_recv(int fd, char *buf, size_t len)
{
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

static void ecs_read (ECS_Client *clii)
{
	uint8_t buf[READ_BUF_LEN]; 
	int len, size;
	len = sizeof(buf);
	memset(buf, 0, READ_BUF_LEN);

	if (!clii || 0 > clii->client_fd) {
		LOG ("read client info is NULL.");
		return;
	}


	size = ecs_recv(clii->client_fd, (char*)buf, len);
	if (0 == size) {
		ecs_client_close(clii);
	} else if (0 < size) {
		LOG("read data: %s, len: %d, size: %d\n", buf, len, size);
		ecs_json_message_parser_feed(&clii->parser, (const char *) buf, size);
	}
}

static void epoll_cli_add(ECS_State *cs, int fd)
{
	struct epoll_event events;

	/* event control set for read event */
	events.events = EPOLLIN;
	events.data.fd = fd; 

	if( epoll_ctl(cs->epoll_fd, EPOLL_CTL_ADD, fd, &events) < 0 )
	{
		LOG("Epoll control fails.in epoll_cli_add.");
	}
}

static ECS_Client *ecs_find_client(int fd)
{
	ECS_Client *clii;

    QTAILQ_FOREACH(clii, &clients, next) {
        if (clii->client_fd == fd)
        	return clii;
    }
	return NULL;
}

static int ecs_add_client(ECS_State *cs, int fd) 
{
	const char* welcome;

	ECS_Client *clii = g_malloc0(sizeof(ECS_Client));
	if (NULL == clii) {
		LOG("ECS_Client allocation failed.");
		return -1;
	}
    
	socket_set_nonblock(fd);

	clii->client_fd = fd;
	clii->cs = cs;
    ecs_json_message_parser_init(&clii->parser, handle_ecs_command, clii);
	epoll_cli_add(cs, fd);

	pthread_mutex_lock(&mutex_clilist);

	QTAILQ_INSERT_TAIL(&clients, clii, next);

	LOG("Add an ecs client. fd: %d", fd);

	
	welcome = WELCOME_MESSAGE;

	send_to_client(fd, welcome);

	pthread_mutex_unlock(&mutex_clilist);

	return 0;
}

static void ecs_accept(ECS_State *cs)
{
    struct sockaddr_in saddr;
#ifndef _WIN32
    struct sockaddr_un uaddr;
#endif
    struct sockaddr *addr;
    socklen_t len;
    int fd;

    for(;;) {
#ifndef _WIN32
	if (cs->is_unix) {
	    len = sizeof(uaddr);
	    addr = (struct sockaddr *)&uaddr;
	} else
#endif
	{
	    len = sizeof(saddr);
	    addr = (struct sockaddr *)&saddr;
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

static void epoll_init(ECS_State *cs) 
{
	struct epoll_event events;

	cs->epoll_fd = epoll_create(MAX_EVENTS);
	if(cs->epoll_fd < 0)
	{
        closesocket(cs->listen_fd);
	}

	events.events = EPOLLIN;
	events.data.fd = cs->listen_fd;

	if( epoll_ctl(cs->epoll_fd, EPOLL_CTL_ADD, cs->listen_fd, &events) < 0 )
	{
		close(cs->listen_fd);
		close(cs->epoll_fd);
	}
}

static void alive_checker(void *opaque)
{
	ECS_State *cs = opaque;
	ECS_Client *clii;
    QObject *obj;

    obj = qobject_from_jsonf("{\"type\":\"self\"}");

	if (NULL != current_ecs && !current_ecs->ecs_running) {
		return;
	}

    QTAILQ_FOREACH(clii, &clients, next) {
		if (1 == clii->keep_alive) {
			LOG("get client fd %d - keep alive fail", clii->client_fd);
			ecs_client_close(clii);
			continue;
		}
		LOG("set client fd %d - keep alive 1", clii->client_fd);
		clii->keep_alive = 1;
    	ecs_json_emitter(clii, obj);
	}
	
	qemu_mod_timer(cs->alive_timer, qemu_get_clock_ns(vm_clock) + 
									get_ticks_per_sec() * TIMER_ALIVE_S);
}

static int socket_initialize(ECS_State *cs, QemuOpts *opts)
{
	int fd = -1;

    fd = inet_listen_opts(opts, 0, NULL);
	if (0 > fd) {
		return -1;
	}

    socket_set_nonblock(fd);

	cs->listen_fd = fd;
	epoll_init(cs);

	cs->alive_timer = qemu_new_timer_ns(vm_clock, alive_checker, cs);

	qemu_mod_timer(cs->alive_timer, qemu_get_clock_ns(vm_clock) + 
									get_ticks_per_sec() * TIMER_ALIVE_S);

	return 0;
}

static int ecs_loop(ECS_State *cs)
{
	int i,nfds;

	nfds = epoll_wait(cs->epoll_fd,cs->events,MAX_EVENTS,0);
	if (0 == nfds){
		return 0;
	}

	if (0 > nfds) {
		LOG("epoll wait error:%d.", nfds);
		return -1;
	} 

	for( i = 0 ; i < nfds ; i++ )
	{
		if (cs->events[i].data.fd == cs->listen_fd) {
			ecs_accept(cs);
			continue;
		}
		ecs_read(ecs_find_client(cs->events[i].data.fd));
	}

	return 0;
}

static int check_port(void)
{
	int port = HOST_LISTEN_PORT;
	int try = EMULATOR_SERVER_NUM;

	for (;try > 0; try--) {
		if (0 <= check_port_bind_listen(port)) {
			LOG("Listening port is %d", port);
			return port;
		}
		port++;
	}
	return -1;
}

static void* ecs_initialize(void* args) 
{
	int ret = 1;
	ECS_State *cs = NULL;
	QemuOpts *opts = NULL;
	Error *local_err = NULL;
	Monitor* mon = NULL;
	int port;
	char host_port[16];

	start_logging();
	LOG("ecs starts initializing.");

	opts = qemu_opts_create(qemu_find_opts("ecs"), "ECS", 1, &local_err);
    if (error_is_set(&local_err)) {
        qerror_report_err(local_err);
        error_free(local_err);
        return NULL;
    }

	port = check_port();
	if (port < 0) {
		LOG("None of port is available.");
		return NULL;
	}
	sprintf(host_port, "%d", port);
    
	qemu_opt_set(opts, "host", HOST_LISTEN_ADDR);
    qemu_opt_set(opts, "port", host_port);

	cs = g_malloc0(sizeof(ECS_State));
	if (NULL == cs) {
		LOG("ECS_State allocation failed.");
		return NULL;
	}

	ret = socket_initialize(cs, opts);
	if (0 > ret) {
		LOG("socket initialization failed.");
		return NULL;
	}

	mon = monitor_create();
	if (NULL == mon) {
		LOG("monitor initialization failed.");
		ecs_close(cs);
		return NULL;
	}

	cs->mon = mon;
	ret = device_initialize();
	if (0 > ret) {
		LOG("device initialization failed.");
		ecs_close(cs);
		return NULL;
	}

	current_ecs = cs;
	cs->ecs_running = 1;

	while(cs->ecs_running) {
		ret = ecs_loop(cs);
		if (0 > ret) {
			ecs_close(cs);
			break;
		}
	}

	return (void*)ret;
}

int stop_ecs(void)
{
	LOG("ecs is closing.");
	if (NULL != current_ecs) {
		current_ecs->ecs_running = 0;
		ecs_close(current_ecs);
	}

	pthread_mutex_destroy(&mutex_clilist);

	return 0;
}

int start_ecs(void)
{
	pthread_t thread_id;

	if (0 != pthread_create(&thread_id, NULL, ecs_initialize, NULL)) {
		LOG("pthread creation failed.");
		return -1;
	}
	return 0;
}
