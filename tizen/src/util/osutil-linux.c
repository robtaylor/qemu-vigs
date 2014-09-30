/*
 * Emulator
 *
 * Copyright (C) 2012 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

/**
  @file     osutil-linux.c
  @brief    Collection of utilities for linux
 */

#include <png.h>
#include "osutil.h"
#include "emulator.h"
#include "emul_state.h"
#include "debug_ch.h"
#include "maru_err_table.h"
#include "sdb.h"

#ifndef CONFIG_LINUX
#error
#endif

#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#ifdef CONFIG_SPICE
#include <dirent.h>
#endif

MULTI_DEBUG_CHANNEL(emulator, osutil);

static int g_shmid;
static char *g_shared_memory;
static int gproxytool = GSETTINGS;

extern char tizen_target_img_path[];

/* Getting proxy commands */
static const char* gproxycmds[][2] = {
    { "gconftool-2 -g /system/proxy/mode" , "gsettings get org.gnome.system.proxy mode" },
    { "gconftool-2 -g /system/proxy/autoconfig_url", "gsettings get org.gnome.system.proxy autoconfig-url" },
    { "gconftool-2 -g /system/http_proxy/host", "gsettings get org.gnome.system.proxy.http host" },
    { "gconftool-2 -g /system/http_proxy/port", "gsettings get org.gnome.system.proxy.http port"},
    { "gconftool-2 -g /system/proxy/secure_host", "gsettings get org.gnome.system.proxy.https host" },
    { "gconftool-2 -g /system/proxy/secure_port", "gsettings get org.gnome.system.proxy.https port" },
    { "gconftool-2 -g /system/proxy/ftp_host", "gsettings get org.gnome.system.proxy.ftp host" },
    { "gconftool-2 -g /system/proxy/ftp_port", "gsettings get org.gnome.system.proxy.ftp port" },
    { "gconftool-2 -g /system/proxy/socks_host", "gsettings get org.gnome.system.proxy.socks host" },
    { "gconftool-2 -g /system/proxy/socks_port", "gsettings get org.gnome.system.proxy.socks port" },
};

void check_vm_lock_os(void)
{
    int shm_id;
    void *shm_addr;
    uint32_t port;
    int val;
    struct shmid_ds shm_info;

    for (port = 26100; port < 26200; port += 10) {
        shm_id = shmget((key_t)port, 0, 0);
        if (shm_id != -1) {
            shm_addr = shmat(shm_id, (void *)0, 0);
            if ((void *)-1 == shm_addr) {
                ERR("error occured at shmat()\n");
                break;
            }

            val = shmctl(shm_id, IPC_STAT, &shm_info);
            if (val != -1) {
                INFO("count of process that use shared memory : %d\n",
                    shm_info.shm_nattch);
                if ((shm_info.shm_nattch > 0) &&
                    g_strcmp0(tizen_target_img_path, (char *)shm_addr) == 0) {
                    if (check_port_bind_listen(port + 1) > 0) {
                        shmdt(shm_addr);
                        continue;
                    }
                    shmdt(shm_addr);
                    maru_register_exit_msg(MARU_EXIT_UNKNOWN,
                                        "Can not execute this VM.\n"
                                        "The same name is running now.");
                    exit(0);
                } else {
                    shmdt(shm_addr);
                }
            }
        }
    }
}

void make_vm_lock_os(void)
{
    int base_port;

    base_port = get_emul_vm_base_port();

    g_shmid = shmget((key_t)base_port, getpagesize(), 0666|IPC_CREAT);
    if (g_shmid == -1) {
        ERR("shmget failed\n");
        perror("osutil-linux: ");
        return;
    }

    g_shared_memory = shmat(g_shmid, (char *)0x00, 0);
    if (g_shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        perror("osutil-linux: ");
        return;
    }

    g_sprintf(g_shared_memory, "%s", tizen_target_img_path);
    INFO("shared memory key: %d value: %s\n",
        base_port, (char *)g_shared_memory);

    if (shmdt(g_shared_memory) == -1) {
        ERR("shmdt failed\n");
        perror("osutil-linux: ");
    }
}

void remove_vm_lock_os(void)
{
    if (shmctl(g_shmid, IPC_RMID, 0) == -1) {
        ERR("shmctl failed\n");
        perror("osutil-linux: ");
    }
}


void set_bin_path_os(char const *const exec_argv)
{
    gchar link_path[PATH_MAX] = { 0, };
    char *file_name = NULL;

    ssize_t len = readlink("/proc/self/exe", link_path, sizeof(link_path) - 1);

    if (len < 0 || len > (sizeof(link_path) - 1)) {
        perror("set_bin_path error : ");
        return;
    }

    link_path[len] = '\0';

    file_name = g_strrstr(link_path, "/");
    g_strlcpy(bin_path, link_path, strlen(link_path) - strlen(file_name) + 1);

    g_strlcat(bin_path, "/", PATH_MAX);

#ifdef CONFIG_SPICE
    g_strlcpy(remote_bin_path, link_path, strlen(link_path) - strlen(file_name) - 2);
    g_strlcat(remote_bin_path, "remote/bin/", PATH_MAX);
#endif
}

int get_number_of_processors(void)
{
    int num_processors = 0;

    num_processors = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_processors < 1) {
        num_processors = 1;
    }
    TRACE("Number of processors : %d\n", num_processors);

    return num_processors;
}

void print_system_info_os(void)
{
    INFO("* Linux\n");

    INFO("* LibPNG Version : %s\n", PNG_LIBPNG_VER_STRING);

    /* depends on building */
    INFO("* QEMU build machine linux kernel version : (%d, %d, %d)\n",
        LINUX_VERSION_CODE >> 16,
        (LINUX_VERSION_CODE >> 8) & 0xff,
        LINUX_VERSION_CODE & 0xff);

     /* depends on launching */
    struct utsname host_uname_buf;
    if (uname(&host_uname_buf) == 0) {
        INFO("* Host machine uname : %s %s %s %s %s\n",
            host_uname_buf.sysname, host_uname_buf.nodename,
            host_uname_buf.release, host_uname_buf.version,
            host_uname_buf.machine);
    }

    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        INFO("* Total Ram : %llu kB, Free: %llu kB\n",
            sys_info.totalram * (unsigned long long)sys_info.mem_unit / 1024,
            sys_info.freeram * (unsigned long long)sys_info.mem_unit / 1024);
    }

    /* get linux distribution information */
    INFO("* Linux distribution infomation :\n");
    char const *const lsb_release_cmd = "lsb_release -d -r -c";
    gchar *buffer = NULL;
    gint buffer_size = strlen(lsb_release_cmd) + 1;

    buffer = g_malloc(buffer_size);

    g_snprintf(buffer, buffer_size, "%s", lsb_release_cmd);

    if (system(buffer) < 0) {
        INFO("system function command '%s' \
                returns error !", buffer);
    }
    g_free(buffer);

    /* pci device description */
    INFO("* Host PCI devices :\n");
    char const *const lspci_cmd = "lspci";
    buffer_size = strlen(lspci_cmd) + 1;

    buffer = g_malloc(buffer_size);

    g_snprintf(buffer, buffer_size, "%s", lspci_cmd);

    fflush(stdout);
    if (system(buffer) < 0) {
        INFO("system function command '%s' \
                returns error !", buffer);
    }
    g_free(buffer);
}

static void process_string(char *buf)
{
    char tmp_buf[DEFAULTBUFLEN];

    /* remove single quotes of strings gotten by gsettings */
    if (gproxytool == GSETTINGS) {
        remove_string(buf, tmp_buf, "\'");
        memset(buf, 0, DEFAULTBUFLEN);
        strncpy(buf, tmp_buf, strlen(tmp_buf)-1);
    }
}

static int get_auto_proxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char type[DEFAULTBUFLEN];
    char proxy[DEFAULTBUFLEN];
    char line[DEFAULTBUFLEN];
    FILE *fp_pacfile;
    char *p = NULL;
    FILE *output;
    char buf[DEFAULTBUFLEN];

    output = popen(gproxycmds[GNOME_PROXY_AUTOCONFIG_URL][gproxytool], "r");
    if (fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        INFO("pac address: %s\n", buf);
        download_url(buf);
    }

    pclose(output);
    fp_pacfile = fopen(pac_tempfile, "r");
    if (fp_pacfile != NULL) {
        while (fgets(line, DEFAULTBUFLEN, fp_pacfile) != NULL) {
            if ((strstr(line, "return") != NULL) && (strstr(line, "if") == NULL)) {
                INFO("line found %s", line);
                sscanf(line, "%*[^\"]\"%s %s", type, proxy);
            }
        }

        if (g_str_has_prefix(type, DIRECT)) {
            INFO("auto proxy is set to direct mode\n");
            fclose(fp_pacfile);
        } else if (g_str_has_prefix(type, PROXY)) {
            INFO("auto proxy is set to proxy mode\n");
            INFO("type: %s, proxy: %s\n", type, proxy);

            p = strtok(proxy, "\";");
            if (p != NULL) {
                INFO("auto proxy to set: %s\n",p);
                strcpy(http_proxy, p);
                strcpy(https_proxy, p);
                strcpy(ftp_proxy, p);
                strcpy(socks_proxy, p);
            }
            fclose(fp_pacfile);
        } else {
            ERR("pac file is not wrong! It could be the wrong pac address or pac file format\n");
            fclose(fp_pacfile);
        }
    } else {
        ERR("fail to get pacfile fp\n");
        return -1;
    }

    if (remove(pac_tempfile) < 0) {
        WARN("fail to remove the temporary pacfile\n");
    }

    return 0;
}

static void get_proxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char buf[DEFAULTBUFLEN] = {0,};
    char buf_port[MAXPORTLEN] = {0,};
    char buf_proxy[DEFAULTBUFLEN] = {0,};
    char *buf_proxy_bak;
    char *proxy;
    FILE *output;
    int MAXPROXYLEN = DEFAULTBUFLEN + MAXPORTLEN;

    output = popen(gproxycmds[GNOME_PROXY_HTTP_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, DEFAULTBUFLEN, "%s", buf);
    }
    pclose(output);

    output = popen(gproxycmds[GNOME_PROXY_HTTP_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf_port) <= 0) {
        //for abnormal case: if can't find the key of http port, get from environment value.
        buf_proxy_bak = getenv("http_proxy");
        INFO("http_proxy from env: %s\n", buf_proxy_bak);
        if(buf_proxy_bak != NULL) {
            proxy = malloc(DEFAULTBUFLEN);
            remove_string(buf_proxy_bak, proxy, HTTP_PREFIX);
            strncpy(http_proxy, proxy, strlen(proxy)-1);
            INFO("final http_proxy value: %s\n", http_proxy);
            free(proxy);
        }
        else {
            INFO("http_proxy is not set on env.\n");
            pclose(output);
            return;
        }

    }
    else {
        snprintf(http_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf_port);
        memset(buf_proxy, 0, DEFAULTBUFLEN);
        INFO("http_proxy: %s\n", http_proxy);
    }
    pclose(output);

    memset(buf, 0, DEFAULTBUFLEN);

    output = popen(gproxycmds[GNOME_PROXY_HTTPS_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, DEFAULTBUFLEN, "%s", buf);
    }
    pclose(output);

    output = popen(gproxycmds[GNOME_PROXY_HTTPS_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(https_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    memset(buf, 0, DEFAULTBUFLEN);
    memset(buf_proxy, 0, DEFAULTBUFLEN);
    INFO("https_proxy : %s\n", https_proxy);

    output = popen(gproxycmds[GNOME_PROXY_FTP_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, DEFAULTBUFLEN, "%s", buf);
    }
    pclose(output);

    output = popen(gproxycmds[GNOME_PROXY_FTP_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(ftp_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    memset(buf, 0, DEFAULTBUFLEN);
    memset(buf_proxy, 0, DEFAULTBUFLEN);
    INFO("ftp_proxy : %s\n", ftp_proxy);

    output = popen(gproxycmds[GNOME_PROXY_SOCKS_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, DEFAULTBUFLEN, "%s", buf);
    }
    pclose(output);

    output = popen(gproxycmds[GNOME_PROXY_SOCKS_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(socks_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    INFO("socks_proxy : %s\n", socks_proxy);
}


void get_host_proxy_os(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char buf[DEFAULTBUFLEN];
    FILE *output;
    int ret;

    output = popen(gproxycmds[GNOME_PROXY_MODE][gproxytool], "r");
    ret = fscanf(output, "%s", buf);
    if (ret <= 0) {
        pclose(output);
        INFO("Try to use gsettings to get proxy\n");
        gproxytool = GSETTINGS;
        output = popen(gproxycmds[GNOME_PROXY_MODE][gproxytool], "r");
        ret = fscanf(output, "%s", buf);
    }
    if (ret > 0) {
        process_string(buf);
        //priority : auto > manual > none
        if (strcmp(buf, "auto") == 0) {
            INFO("AUTO PROXY MODE\n");
            get_auto_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        }
        else if (strcmp(buf, "manual") == 0) {
            INFO("MANUAL PROXY MODE\n");
            get_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        }
        else if (strcmp(buf, "none") == 0) {
            INFO("DIRECT PROXY MODE\n");
        }
    }
    pclose(output);
}

#ifdef CONFIG_SPICE
#define PID_MAX_COUNT 256
const char *execution_file_websocket = "websockify.py";
const char *execution_file_node = "node";
const char *node_proc_name = "emulator-x86-web";

void get_process_id(char const *process_name, char *first_param, int *pid, int *pscount)
{
    char cmdline[2048], dir_name[255];
    int total_len = 0, current_len = 0;
    struct dirent *dir_entry_p;
    DIR *dir_p;
    FILE *fp;
    char *mptr;

    dir_p = opendir("/proc/");
    while (NULL != (dir_entry_p = readdir(dir_p))) {
        /* Checking for numbered directories */
        if (strspn(dir_entry_p->d_name, "0123456789") == strlen(dir_entry_p->d_name)) {
            strcpy(dir_name, "/proc/");
            strcat(dir_name, dir_entry_p->d_name);
            strcat(dir_name, "/cmdline");

            fp = fopen(dir_name, "rb");
            if (fp == NULL) {
                continue;
            }

            total_len = 0;
            memset(cmdline, 0, sizeof(cmdline));
            while (!feof(fp)) {
                cmdline[total_len++] = fgetc(fp);
            }

            fclose(fp);
            current_len = strlen(cmdline);
            mptr = cmdline;
            do {
                if (strstr(mptr, process_name) != NULL) {
                    if (!first_param || strstr(&cmdline[current_len + 1], first_param) != NULL) {
                        if (sizeof(pid) < *pscount + 1) {
                            WARN("PID array size is not enough.\n");
                            return;
                        }
                        pid[*pscount] = atoi(dir_entry_p->d_name);
                        INFO("get_process_id(%s %s) :Found. id = %d\n", process_name, first_param, pid[*pscount]);
                        (*pscount)++;
                    }
                    break;
                }

                mptr = &cmdline[current_len + 1];
                current_len += strlen(mptr) + 1;
            } while (current_len < total_len);
        }
    }

    closedir(dir_p);
    if (*pscount == 0) {
        INFO("get_process_id(%s %s) : id = 0 (could not find process)\n", process_name, first_param);
    }
}

void execute_websocket(int port)
{
    char const *remote_bin_dir = get_remote_bin_path();
    char const *relative_path = "../websocket/";
    char websocket_path[strlen(remote_bin_dir) + strlen(execution_file_websocket) + strlen(relative_path) + 1];
    int ret = -1;
    char local_port[32];
    char websocket_port[16];

    memset(websocket_port, 0, sizeof(websocket_port));
    sprintf(websocket_port, "%d", port);

    memset(local_port, 0, sizeof(local_port));
    sprintf(local_port, "localhost:%d", get_emul_spice_port());

    memset(websocket_path, 0, sizeof(websocket_path));
    sprintf(websocket_path, "%s%s%s", remote_bin_dir, relative_path, execution_file_websocket);

    INFO("Exec [%s %s %s]\n", websocket_path, websocket_port, local_port);

    ret = execl(websocket_path, execution_file_websocket, websocket_port, local_port, (char *)0);
    if (ret == 127) {
        WARN("Can't execute websocket.\n");
    } else if (ret == -1) {
        WARN("Fork error!\n");
    }
}

void execute_nodejs(void)
{
    char const *remote_bin_dir = get_remote_bin_path();
    char const *relative_path = "../web-viewer/bin/emul";
    char webviewer_script[strlen(remote_bin_dir) + strlen(relative_path) + 1];
    char nodejs_path[strlen(remote_bin_dir) + strlen(execution_file_node) + 1];
    int ret = -1;

    memset(webviewer_script, 0, sizeof(webviewer_script));
    sprintf(webviewer_script, "%s%s", remote_bin_dir, relative_path);

    memset(nodejs_path, 0, sizeof(nodejs_path));
    sprintf(nodejs_path, "%s%s", remote_bin_dir, execution_file_node);

    INFO("Exec [%s %s]\n", nodejs_path, webviewer_script);

    ret = execl(nodejs_path, execution_file_node, webviewer_script, (char *)0);
    if (ret == 127) {
        WARN("Can't execute node server.\n");
    } else if (ret == -1) {
        WARN("Fork error!\n");
    }
}

void clean_websocket_port(int signal)
{
    char websocket_port[16];
    memset(websocket_port, 0, sizeof(websocket_port));
    sprintf(websocket_port, "%d", get_emul_websocket_port());

    int pscount = 0, i = 0;
    int pid[PID_MAX_COUNT];

    memset(pid, 0, PID_MAX_COUNT);
    get_process_id(execution_file_websocket, websocket_port, pid, &pscount);
    if (pscount > 0) {
        for (i = 0; i < pscount; i++) {
            INFO("Will be killed PID: %d\n", pid[i]);
            kill(pid[i], signal);
        }
    }
}

static void websocket_notify_exit(Notifier *notifier, void *data)
{
    clean_websocket_port(SIGTERM);
}

static void nodejs_notify_exit(Notifier *notifier, void *data)
{
    int pscount = 0, i = 0;
    int pid[PID_MAX_COUNT];

    memset(pid, 0, sizeof(pid));
    get_process_id("spicevmc", NULL, pid, &pscount);
    if (pscount == 1) {
        INFO("Detected the last spice emulator.\n");
        pid[0] = 0;
        pscount = 0;
        get_process_id(node_proc_name, NULL, pid, &pscount);
        for (i = 0; i < pscount; i++) {
            INFO("Will be killed %s, PID: %d\n", node_proc_name, pid[i]);
            kill(pid[i], SIGTERM);
        }
    }
}

static Notifier websocket_exit = { .notify = websocket_notify_exit };
static Notifier nodejs_exit = { .notify = nodejs_notify_exit };

void websocket_init(void)
{
    int pscount = 0;
    char websocket_port[16];
    int pid[PID_MAX_COUNT];

    memset(websocket_port, 0, sizeof(websocket_port));
    sprintf(websocket_port, "%d", get_emul_websocket_port());

    memset(pid, 0, sizeof(pid));
    get_process_id(execution_file_websocket, websocket_port, pid, &pscount);
    emulator_add_exit_notifier(&websocket_exit);

    if (pscount == 0) {
        int pid = fork();
        if (pid == 0) {
            setsid();
            execute_websocket(get_emul_websocket_port());
        }
    } else {
       INFO("Aleady running websokify %s localhost:%d\n", websocket_port, get_emul_spice_port());
    }
}

void nodejs_init(void)
{
    int pscount = 0;
    int pid[PID_MAX_COUNT];

    memset(pid, 0, sizeof(pid));
    get_process_id(node_proc_name, NULL, pid, &pscount);
    emulator_add_exit_notifier(&nodejs_exit);

    if (pscount == 0) {
        int pid = fork();
        if (pid == 0) {
            setsid();
            execute_nodejs();
        }
    } else {
       INFO("Aleady running node server.\n");
    }
}
#endif
