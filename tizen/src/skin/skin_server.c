/*
 * socket server for emulator skin
 *
 * Copyright (C) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include "skin_server.h"
#include "skin_operation.h"

#define READ_HEADER_SIZE 16
#define SEND_HEADER_SIZE 10

#define HEART_BEAT_INTERVAL 2
#define HEART_BEAT_FAIL_COUNT 5
#define HEART_BEAT_EXPIRE_COUNT 5

enum {
    RECV_START = 1,
    RECV_MOUSE_EVENT = 10,
    RECV_KEY_EVENT = 11,
    RECV_CHANGE_LCD_STATE = 12,
    RECV_HEART_BEAT = 900,
    RECV_RESPONSE_HEART_BEAT = 901,
    RECV_CLOSE = 998,
    RECV_RESPONSE_SHUTDOWN = 999
};

enum {
    SEND_HEART_BEAT = 1,
    SEND_HEART_BEAT_RESPONSE = 2,
    SEND_SHUTDOWN = 999
};

static uint16_t svr_port = 0;
static int server_sock = 0;
static int client_sock = 0;
static int stop = 0;

static int stop_heartbeat = 0;
static int recv_heartbeat_count = 0;
static pthread_t thread_id_heartbeat = 0;
static pthread_mutex_t mutex_heartbeat = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_heartbeat = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_recv_heartbeat_count = PTHREAD_MUTEX_INITIALIZER;

static void* run_skin_server( void* args );
static int send_skin( int client_sock, short send_cmd );
static void* do_heart_beat( void* args );
static int start_heart_beat( int client_sock );
static void stop_heart_beat();

pthread_t start_skin_server( uint16_t default_svr_port, int argc, char** argv ) {

    svr_port = default_svr_port;

    pthread_t thread_id = -1;

    if ( 0 != pthread_create( &thread_id, NULL, run_skin_server, NULL ) ) {
        printf( "fail to create skin_server pthread.\n" );
    }
    return thread_id;

}

void shutdown_skin_server() {
    if ( client_sock ) {
        if ( 0 > send_skin( client_sock, SEND_SHUTDOWN ) ) {
            printf( "fail to send shutdown to skin.\n" );
            stop = 1;
            // force close
            close( client_sock );
            if ( server_sock ) {
                close( server_sock );
            }
        } else {
            // skin sent RECV_RESPONSE_SHUTDOWN.
        }
    }
}

static void* run_skin_server( void* args ) {

    uint16_t port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

#ifdef __MINGW32__
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(2,0), &wsadata) == SOCKET_ERROR) {
        ERR("Error creating socket.\n");
        return NULL;
    }
#endif

    port = svr_port;

    if ( ( server_sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 ) {
        printf( "create listen socket error: " );
        perror( "socket" );
        goto cleanup;
    }

    memset( &server_addr, '\0', sizeof( server_addr ) );

    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    server_addr.sin_port = htons( port );

    int opt = 1;
    setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) );

    if ( 0 > bind( server_sock, (struct sockaddr *) &server_addr, sizeof( server_addr ) ) ) {
        //TODO rebind in case of port
        printf( "skin server bind error: " );
        perror( "bind" );
        goto cleanup;
    } else {
        printf( "success to bind port[127.0.0.1:%d/tcp] for skin_server in host \n", port );
    }

    if ( listen( server_sock, 4 ) < 0 ) {
        printf( "skin_server listen error: " );
        perror( "listen" );
        goto cleanup;
    }

    char readbuf[READ_HEADER_SIZE];

    printf( "skin server start...port:%d\n", port );

    while ( 1 ) {

        if ( stop ) {
            break;
        }

        //TODO receive client retry ?

        printf( "start accepting socket...\n" );

        if ( 0 > ( client_sock = accept( server_sock, (struct sockaddr *) &client_addr, &client_len ) ) ) {
            printf( "skin_servier accept error: " );
            perror( "accept" );
            continue;
        }

        while ( 1 ) {

            if ( stop ) {
                printf( "stop reading this socket.\n" );
                break;
            }

            stop_heartbeat = 0;
            memset( &readbuf, 0, READ_HEADER_SIZE );

            int read_cnt = read( client_sock, readbuf, READ_HEADER_SIZE );

            if ( 0 > read_cnt ) {

                perror( "error : skin_server read" );
                break;

            } else {

                if ( 0 == read_cnt ) {
                    printf( "read_cnt is 0.\n" );
                    break;
                }

                printf( "================= recv =================\n" );
                printf( "read_cnt:%d\n", read_cnt );

                int pid = 0;
                long long req_id = 0;
                short cmd = 0;
                short length = 0;

                char* p = readbuf;

                memcpy( &pid, p, sizeof( pid ) );
                p += sizeof( pid );
                memcpy( &req_id, p, sizeof( req_id ) );
                p += sizeof( req_id );
                memcpy( &cmd, p, sizeof( cmd ) );
                p += sizeof( cmd );
                memcpy( &length, p, sizeof( length ) );

                pid = ntohl( pid );
                req_id = be64toh(req_id);
                cmd = ntohs( cmd );
                length = ntohs( length );

                //TODO check identification with pid

                printf( "pid:%d\n", pid );
                printf( "req_id:%lld\n", req_id );
                printf( "cmd:%d\n", cmd );
                printf( "length:%d\n", length );

                if ( 0 < length ) {

                    memset( &readbuf, 0, length );

                    int read_cnt = read( client_sock, readbuf, length );

                    printf( "data read_cnt:%d\n", read_cnt );

                    if ( 0 > read_cnt ) {
                        perror( "error : skin_server read data" );
                        break;
                    } else if ( 0 == read_cnt ) {
                        printf( "data read_cnt is 0.\n" );
                        break;
                    } else if ( read_cnt != length ) {
                        printf( "data read_cnt is not equal to length.\n" );
                        break;
                    }

                }

                printf( "----------------------------------------\n" );

                switch ( cmd ) {
                case RECV_START: {
                    printf( "RECV_START\n" );
                    if ( 0 >= length ) {
                        printf( "there is no data looking at 0 length." );
                        continue;
                    }

                    int handle_id = 0;

                    memcpy( &handle_id, readbuf, sizeof( handle_id ) );
                    handle_id = ntohl( handle_id );

                    printf( "handle_id:%d\n", handle_id );

                    if ( start_heart_beat( client_sock ) ) {
                        start_display( handle_id );
                    } else {
                        stop = 1;
                    }
                    break;
                }
                case RECV_MOUSE_EVENT: {

                    printf( "RECV_MOUSE_EVENT\n" );
                    if ( 0 >= length ) {
                        printf( "there is no data looking at 0 length." );
                        continue;
                    }

                    int event_type = 0;
                    int x = 0;
                    int y = 0;

                    char* p = readbuf;
                    memcpy( &event_type, p, sizeof( event_type ) );
                    p += sizeof( event_type );
                    memcpy( &x, p, sizeof( x ) );
                    p += sizeof( x );
                    memcpy( &y, p, sizeof( y ) );

                    event_type = ntohl( event_type );
                    x = ntohl( x );
                    y = ntohl( y );

                    do_mouse_event( event_type, x, y );
                    break;
                }
                case RECV_KEY_EVENT: {

                    printf( "RECV_KEY_EVENT\n" );
                    if ( 0 >= length ) {
                        printf( "there is no data looking at 0 length." );
                        continue;
                    }

                    int event_type = 0;
                    int keycode = 0;

                    char* p = readbuf;
                    memcpy( &event_type, p, sizeof( event_type ) );
                    p += sizeof( event_type );
                    memcpy( &keycode, p, sizeof( keycode ) );

                    event_type = ntohl( event_type );
                    keycode = ntohl( keycode );

                    do_key_event( event_type, keycode );
                    break;
                }
                case RECV_CHANGE_LCD_STATE: {
                    printf( "RECV_CHANGE_LCD_STATE\n" );
                    if ( 0 >= length ) {
                        printf( "there is no data looking at 0 length." );
                        continue;
                    }

                    short direction = 0;
                    short scale = 0;

                    char* p = readbuf;
                    memcpy( &direction, p, sizeof( direction ) );
                    p += sizeof( direction );
                    memcpy( &scale, p, sizeof( scale ) );

                    direction = ntohs( direction );
                    scale = ntohs( scale );

                    change_lcd_state( direction, scale );
                    break;
                }
                case RECV_HEART_BEAT: {
                    printf( "RECV_HEART_BEAT\n" );
                    if ( 0 > send_skin( client_sock, SEND_HEART_BEAT_RESPONSE ) ) {
                        printf( "Fail to send a response of heartbeat to skin.\n" );
                    }
                    break;
                }
                case RECV_RESPONSE_HEART_BEAT: {
                    printf( "RECV_RESPONSE_HEART_BEAT\n" );
                    pthread_mutex_lock( &mutex_recv_heartbeat_count );
                    recv_heartbeat_count = 0;
                    pthread_mutex_unlock( &mutex_recv_heartbeat_count );
                    break;
                }
                case RECV_CLOSE: {
                    printf( "RECV_CLOSE\n" );
                    request_close();
                    //XXX
                    shutdown_skin_server();
                    break;
                }
                case RECV_RESPONSE_SHUTDOWN: {
                    printf( "RECV_RESPONSE_SHUTDOWN\n" );
                    stop = 1;
                    break;
                }
                default: {
                    printf( "!!! unknown command : %d\n", cmd );
                    break;
                }
                }

                printf( "========================================\n" );

            }

        }

        stop_heart_beat();

        if ( client_sock ) {
            close( client_sock );
        }

    }

    cleanup:
#ifdef __MINGW32__
    if(listen_s)
    closesocket(listen_s);
#else
    if ( server_sock ) {
        close( server_sock );
    }
#endif

    return NULL;
}

static int send_skin( int client_sock, short send_cmd ) {

    char sendbuf[SEND_HEADER_SIZE];
    memset( &sendbuf, 0, SEND_HEADER_SIZE );

    long long request_id = (long long) rand();
    printf( "send skin request_id:%lld, send_cmd:%d\n", request_id, send_cmd );

    request_id = htobe64( request_id );

    short data = send_cmd;
    data = htons( data );

    char* p = sendbuf;
    memcpy( p, &request_id, sizeof( request_id ) );
    p += sizeof( request_id );
    memcpy( p, &data, sizeof( data ) );

    ssize_t write_count = write( client_sock, sendbuf, SEND_HEADER_SIZE );

    return write_count;

}

static void* do_heart_beat( void* args ) {

    int client_sock = *(int*) args;

    int fail_count = 0;
    int shutdown = 0;

    while ( 1 ) {

        struct timeval current;
        gettimeofday( &current, NULL );

        struct timespec ts_heartbeat;
        ts_heartbeat.tv_sec = current.tv_sec + HEART_BEAT_INTERVAL;
        ts_heartbeat.tv_nsec = current.tv_usec * 1000;

        pthread_mutex_lock( &mutex_heartbeat );
        pthread_cond_timedwait( &cond_heartbeat, &mutex_heartbeat, &ts_heartbeat );
        pthread_mutex_unlock( &mutex_heartbeat );

        if ( stop_heartbeat ) {
            printf( "stop heart beat.\n" );
            break;
        }

        printf( "send heartbeat to skin...\n" );
        if ( 0 > send_skin( client_sock, SEND_HEART_BEAT ) ) {
            fail_count++;
        } else {
            fail_count = 0;
        }

        if ( HEART_BEAT_FAIL_COUNT < fail_count ) {
            printf( "fail to write heart beat to skin. fail count:%d\n", HEART_BEAT_FAIL_COUNT );
            shutdown = 1;
            break;
        }

        int count = 0;
        pthread_mutex_lock( &mutex_recv_heartbeat_count );
        recv_heartbeat_count++;
        count = recv_heartbeat_count;
        printf( "recv_heartbeat_count:%d\n", recv_heartbeat_count );
        pthread_mutex_unlock( &mutex_recv_heartbeat_count );

        if ( HEART_BEAT_EXPIRE_COUNT < count ) {
            printf( "received heartbeat count is expired.\n" );
            shutdown = 1;
            break;
        }

    }

    if ( shutdown ) {
        printf( "shutdown skin_server by heartbeat thread.\n" );
        shutdown_skin_server();
    }

    return NULL;

}

static int start_heart_beat( int client_sock ) {
    if ( 0 != pthread_create( &thread_id_heartbeat, NULL, do_heart_beat, (void*) &client_sock ) ) {
        printf( "fail to create heartbean pthread.\n" );
        return 0;
    } else {
        return 1;
    }
}

static void stop_heart_beat() {
    pthread_mutex_lock( &mutex_heartbeat );
    stop_heartbeat = 1;
    pthread_cond_signal( &cond_heartbeat );
    pthread_mutex_unlock( &mutex_heartbeat );
}

#if 0 //XXX for test
int main( int argc, char** argv ) {

    int thread_return;

    pthread_t thread_id = start_skin_server( 11111, argc, argv );

    if( -1 == thread_id ) {
        return -1;
    }

    pthread_join( thread_id, (void**)&thread_return );

    printf( "exit program...thread_return:%d\n", thread_return );

    return 0;

}
#endif
