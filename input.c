/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "sysemu.h"
#include "net.h"
#include "monitor.h"
#include "console.h"
#include "qjson.h"

static QTAILQ_HEAD(, QEMUPutKbdEntry) kbd_handlers =
    QTAILQ_HEAD_INITIALIZER(kbd_handlers);
static QTAILQ_HEAD(, QEMUPutLEDEntry) led_handlers = QTAILQ_HEAD_INITIALIZER(led_handlers);
static QTAILQ_HEAD(, QEMUPutMouseEntry) mouse_handlers =
    QTAILQ_HEAD_INITIALIZER(mouse_handlers);
static NotifierList mouse_mode_notifiers = 
    NOTIFIER_LIST_INITIALIZER(mouse_mode_notifiers);

static void check_mode_change(void)
{
    static int current_is_absolute, current_has_absolute;
    int is_absolute;
    int has_absolute;

    is_absolute = kbd_mouse_is_absolute();
    has_absolute = kbd_mouse_has_absolute();

    if (is_absolute != current_is_absolute ||
        has_absolute != current_has_absolute) {
        notifier_list_notify(&mouse_mode_notifiers);
    }

    current_is_absolute = is_absolute;
    current_has_absolute = has_absolute;
}

QEMUPutKbdEntry *qemu_add_kbd_event_handler(QEMUPutKBDEvent *func,
                                            void *opaque,
                                            const char *name)
{
    static int index = 0;
    QEMUPutKbdEntry *s, *cursor;

    QTAILQ_FOREACH(cursor, &kbd_handlers, node) {
        if (cursor->qemu_put_kbd_event == func &&
            cursor->qemu_put_kbd_event_opaque == opaque) {
            return cursor;
        }
    }

    s = qemu_mallocz(sizeof(QEMUPutKbdEntry));

    s->qemu_put_kbd_event_opaque = opaque;
    s->qemu_put_kbd_event = func;
    s->qemu_put_kbd_name = qemu_strdup(name);
    s->index = index++;

    QTAILQ_INSERT_TAIL(&kbd_handlers, s, node);

    return s;
}

void qemu_remove_kbd_event_handler(QEMUPutKbdEntry *entry)
{
    QTAILQ_REMOVE(&kbd_handlers, entry, node);
    qemu_free(entry);
}

QEMUPutMouseEntry *qemu_add_mouse_event_handler(QEMUPutMouseEvent *func,
                                                void *opaque, int absolute,
                                                const char *name)
{
    QEMUPutMouseEntry *s;
    static int mouse_index = 0;

    s = qemu_mallocz(sizeof(QEMUPutMouseEntry));

    s->qemu_put_mouse_event = func;
    s->qemu_put_mouse_event_opaque = opaque;
    s->qemu_put_mouse_event_absolute = absolute;
    s->qemu_put_mouse_event_name = qemu_strdup(name);
    s->index = mouse_index++;

    QTAILQ_INSERT_TAIL(&mouse_handlers, s, node);

    check_mode_change();

    return s;
}

void qemu_activate_mouse_event_handler(QEMUPutMouseEntry *entry)
{
    QTAILQ_REMOVE(&mouse_handlers, entry, node);
    QTAILQ_INSERT_HEAD(&mouse_handlers, entry, node);

    check_mode_change();
}

void qemu_remove_mouse_event_handler(QEMUPutMouseEntry *entry)
{
    QTAILQ_REMOVE(&mouse_handlers, entry, node);

    qemu_free(entry->qemu_put_mouse_event_name);
    qemu_free(entry);

    check_mode_change();
}

QEMUPutLEDEntry *qemu_add_led_event_handler(QEMUPutLEDEvent *func,
                                            void *opaque)
{
    QEMUPutLEDEntry *s;

    s = qemu_mallocz(sizeof(QEMUPutLEDEntry));

    s->put_led = func;
    s->opaque = opaque;
    QTAILQ_INSERT_TAIL(&led_handlers, s, next);
    return s;
}

void qemu_remove_led_event_handler(QEMUPutLEDEntry *entry)
{
    if (entry == NULL)
        return;
    QTAILQ_REMOVE(&led_handlers, entry, next);
    qemu_free(entry);
}

void kbd_put_keycode(int keycode)
{
    QEMUPutKbdEntry *entry;
    QTAILQ_FOREACH(entry, &kbd_handlers, node) {
        if (!entry->qemu_put_kbd_event(entry->qemu_put_kbd_event_opaque,
                                       keycode)) {
            break;
        }
    }
}

void kbd_put_ledstate(int ledstate)
{
    QEMUPutLEDEntry *cursor;

    QTAILQ_FOREACH(cursor, &led_handlers, next) {
        cursor->put_led(cursor->opaque, ledstate);
    }
}

void kbd_mouse_event(int dx, int dy, int dz, int buttons_state)
{
    QEMUPutMouseEntry *entry;
    QEMUPutMouseEvent *mouse_event;
    void *mouse_event_opaque;
    int width;

    if (QTAILQ_EMPTY(&mouse_handlers)) {
        return;
    }

    entry = QTAILQ_FIRST(&mouse_handlers);

    mouse_event = entry->qemu_put_mouse_event;
    mouse_event_opaque = entry->qemu_put_mouse_event_opaque;

    if (mouse_event) {
        if (graphic_rotate) {
            if (entry->qemu_put_mouse_event_absolute)
                width = 0x7fff;
            else
                width = graphic_width - 1;
            mouse_event(mouse_event_opaque,
                        width - dy, dx, dz, buttons_state);
        } else
            mouse_event(mouse_event_opaque,
                        dx, dy, dz, buttons_state);
    }
}

int kbd_mouse_is_absolute(void)
{
    if (QTAILQ_EMPTY(&mouse_handlers)) {
        return 0;
    }

    return QTAILQ_FIRST(&mouse_handlers)->qemu_put_mouse_event_absolute;
}

int kbd_mouse_has_absolute(void)
{
    QEMUPutMouseEntry *entry;

    QTAILQ_FOREACH(entry, &mouse_handlers, node) {
        if (entry->qemu_put_mouse_event_absolute) {
            return 1;
        }
    }

    return 0;
}

static void info_mice_iter(QObject *data, void *opaque)
{
    QDict *mouse;
    Monitor *mon = opaque;

    mouse = qobject_to_qdict(data);
    monitor_printf(mon, "%c Mouse #%" PRId64 ": %s%s\n",
                  (qdict_get_bool(mouse, "current") ? '*' : ' '),
                   qdict_get_int(mouse, "index"), qdict_get_str(mouse, "name"),
                   qdict_get_bool(mouse, "absolute") ? " (absolute)" : "");
}

void do_info_mice_print(Monitor *mon, const QObject *data)
{
    QList *mice_list;

    mice_list = qobject_to_qlist(data);
    if (qlist_empty(mice_list)) {
        monitor_printf(mon, "No mouse devices connected\n");
        return;
    }

    qlist_iter(mice_list, info_mice_iter, mon);
}

void do_info_mice(Monitor *mon, QObject **ret_data)
{
    QEMUPutMouseEntry *cursor;
    QList *mice_list;
    int current;

    mice_list = qlist_new();

    if (QTAILQ_EMPTY(&mouse_handlers)) {
        goto out;
    }

    current = QTAILQ_FIRST(&mouse_handlers)->index;

    QTAILQ_FOREACH(cursor, &mouse_handlers, node) {
        QObject *obj;
        obj = qobject_from_jsonf("{ 'name': %s,"
                                 "  'index': %d,"
                                 "  'current': %i,"
                                 "  'absolute': %i }",
                                 cursor->qemu_put_mouse_event_name,
                                 cursor->index,
                                 cursor->index == current,
                                 !!cursor->qemu_put_mouse_event_absolute);
        qlist_append_obj(mice_list, obj);
    }

out:
    *ret_data = QOBJECT(mice_list);
}

void do_mouse_set(Monitor *mon, const QDict *qdict)
{
    QEMUPutMouseEntry *cursor;
    int index = qdict_get_int(qdict, "index");
    int found = 0;

    if (QTAILQ_EMPTY(&mouse_handlers)) {
        monitor_printf(mon, "No mouse devices connected\n");
        return;
    }

    QTAILQ_FOREACH(cursor, &mouse_handlers, node) {
        if (cursor->index == index) {
            found = 1;
            qemu_activate_mouse_event_handler(cursor);
            break;
        }
    }

    if (!found) {
        monitor_printf(mon, "Mouse at given index not found\n");
    }

    check_mode_change();
}

void qemu_add_mouse_mode_change_notifier(Notifier *notify)
{
    notifier_list_add(&mouse_mode_notifiers, notify);
}

void qemu_remove_mouse_mode_change_notifier(Notifier *notify)
{
    notifier_list_remove(&mouse_mode_notifiers, notify);
}

static void info_keyboard_iter(QObject *data, void *opaque)
{
    QDict *kbd;
    Monitor *mon = opaque;

    kbd = qobject_to_qdict(data);
    monitor_printf(mon, "%c Keyboard #%" PRId64 ": %s\n",
                  (qdict_get_bool(kbd, "current") ? '*' : ' '),
                  qdict_get_int(kbd, "index"), qdict_get_str(kbd, "name"));
}

void do_info_keyboard_print(Monitor *mon, const QObject *data)
{
    QList *kbd_list;

    kbd_list = qobject_to_qlist(data);
    if (qlist_empty(kbd_list)) {
        monitor_printf(mon, "No keyboard devices connected\n");
        return;
    }

    qlist_iter(kbd_list, info_keyboard_iter, mon);
}

void qemu_activate_keyboard_event_handler(QEMUPutKbdEntry *entry)
{
    QTAILQ_REMOVE(&kbd_handlers, entry, node);
    QTAILQ_INSERT_HEAD(&kbd_handlers, entry, node);
}

/*
 * do_info_keyboard(): Show VM keyboard information
 *
 * Each keyboard is represented by a QDict, the returned QObject is
 * a QList of all keyboards.
 *
 * The keyboard QDict contains the following:
 *
 * - "name": keyboard's name
 * - "index": keyboard's index
 * - "current": true if this keyboard is receiving events, false otherwise
 *
 * Example:
 *
 * [ { "name": "QEMU USB Keyboard", "index": 0, "current": false },
 *   { "name": "QEMU PS/2 Keyboard", "index": 1, "current": true } ]
 */
void do_info_keyboard(Monitor *mon, QObject **ret_data)
{
    QEMUPutKbdEntry *cursor;
    QList *kbd_list;
    int current;

    kbd_list = qlist_new();

    if (QTAILQ_EMPTY(&kbd_handlers)) {
        goto out;
    }

    current = QTAILQ_FIRST(&kbd_handlers)->index;
    QTAILQ_FOREACH(cursor, &kbd_handlers, node) {
        QObject *obj;
        obj = qobject_from_jsonf("{ 'name': %s,"
                                 "  'index': %d,"
                                 "  'current': %i }",
                                 cursor->qemu_put_kbd_name,
                                 cursor->index,
                                 current == cursor->index);
        qlist_append_obj(kbd_list, obj);
    }
out:
    *ret_data = QOBJECT(kbd_list);
}

/*
 * do_keyboard_set(): Set active keyboard
 *
 * Argument qdict contains
 * - "index": the keyboard index to set
 *
 * Example:
 *
 * { "index": "0" }
 */
int do_keyboard_set(Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    QEMUPutKbdEntry *cursor;
    int index = qdict_get_int(qdict, "index");
    int found = 0;

    if (QTAILQ_EMPTY(&kbd_handlers)) {
        qerror_report(QERR_DEVICE_NOT_FOUND, "keyboard");
        return -1;
    }

    QTAILQ_FOREACH(cursor, &kbd_handlers, node) {
        if (cursor->index == index) {
            qemu_activate_keyboard_event_handler(cursor);
            found = 1;
            break;
        }
    }

    if (!found) {
        qerror_report(QERR_INVALID_PARAMETER, "index");
        return -1;
    }

    return 0;
}
