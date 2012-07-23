/* 
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * HyunJun Son <hj79.son@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
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

/**
  @file	option.c
  @brief	collection of dialog function
 */

#include "option.h"
#include "emulator.h"
#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#else
#ifdef WINVER < 0x0501
#undef WINVER
#define WINVER 0x0501
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <winreg.h>
#endif
#endif

#include "debug_ch.h"

#define HTTP_PROTOCOL "http="
#define HTTPS_PROTOCOL "https="
#define FTP_PROTOCOL "ftp="
#define SOCKS_PROTOCOL "socks="

//DEFAULT_DEBUG_CHANNEL(tizen);
MULTI_DEBUG_CHANNEL(tizen, option);

/**
  @brief	get host DNS server address
  @param	dns1: return value (first dns server address)
  @param	dns2: return value (second dns server address)
  @return always 0
 */
int gethostDNS(char *dns1, char *dns2)
{
#ifndef _WIN32
	FILE *resolv;
	char buf[255];
	memset(buf, 0, sizeof(char)*255);

	resolv = fopen("/etc/resolv.conf", "r");
	if (resolv <= 0) {
		ERR( "Cann't open \"/etc/resolv.conf.\"\n");
		return 1;
	}

	while(fscanf(resolv , "%s", buf) != EOF) {
		if(strcmp(buf, "nameserver") == 0)
		{
			fscanf(resolv , "%s", dns1);
			break;
		}
	}

	while(fscanf(resolv , "%s", buf) != EOF) {
		if(strcmp(buf, "nameserver") == 0)
		{
			fscanf(resolv , "%s", dns2);
			break;
		}
	}

	fclose(resolv);
#else
	PIP_ADAPTER_ADDRESSES pAdapterAddr;
	PIP_ADAPTER_ADDRESSES pAddr;
	PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsAddr;
	unsigned long dwResult;
	unsigned long nBufferLength = sizeof(IP_ADAPTER_ADDRESSES);
	pAdapterAddr = (PIP_ADAPTER_ADDRESSES)malloc(nBufferLength);
	memset(pAdapterAddr, 0x00, nBufferLength);

	while ((dwResult = GetAdaptersAddresses(AF_INET, 0, NULL, pAdapterAddr, &nBufferLength))
			== ERROR_BUFFER_OVERFLOW) {
		free(pAdapterAddr);
		pAdapterAddr = (PIP_ADAPTER_ADDRESSES)malloc(nBufferLength);
		memset(pAdapterAddr, 0x00, nBufferLength);
	}

	pAddr = pAdapterAddr;
	for (; pAddr != NULL; pAddr = pAddr->Next) {
		pDnsAddr = pAddr->FirstDnsServerAddress;
		for (; pDnsAddr != NULL; pDnsAddr = pDnsAddr->Next) {
			struct sockaddr_in *pSockAddr = (struct sockaddr_in*)pDnsAddr->Address.lpSockaddr;
			if(*dns1 == 0) {
				strcpy(dns1, inet_ntoa(pSockAddr->sin_addr));
				continue;
			}
			if(*dns2 == 0) {
				strcpy(dns2, inet_ntoa(pSockAddr->sin_addr));
				continue;
			}
		}
	}
	free(pAdapterAddr);
#endif

	// by caramis... change DNS address if localhost has DNS server or DNS cache.
	if(!strncmp(dns1, "127.0.0.1", 9) || !strncmp(dns1, "localhost", 9)) {
		strncpy(dns1, "10.0.2.2", 9);
	}
	if(!strncmp(dns2, "127.0.0.1", 9) || !strncmp(dns2, "localhost", 9)) {
		strncpy(dns2, "10.0.2.2", 9);
	}

	return 0;
}

void remove_protocol(char *src, char *dst, const char *protocol)
{
    int len = strlen(protocol);
    int i, j;
    int max_len = strlen(src);
    
    for(i = len, j = 0; i < max_len; i++)
    {
        dst[j++] = src[i];
    }
    
    dst[j] = '\0';
}

void getlinuxproxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char buf[MAXLEN];
	FILE *output;
    memset(buf, 0, MAXLEN);
    
    output = popen("gconftool-2 --get /system/http_proxy/host", "r");
	fscanf(output , "%s", buf);
	sprintf(http_proxy, "%s", buf);
    pclose(output);

	output = popen("gconftool-2 --get /system/http_proxy/port", "r");
	fscanf(output , "%s", buf);
	sprintf(http_proxy, "%s:%s", http_proxy, buf);
	pclose(output);
    memset(buf, 0, MAXLEN);
    INFO("http_proxy : %s\n", http_proxy);

    output = popen("gconftool-2 --get /system/proxy/secure_host", "r");
	fscanf(output , "%s", buf);
	sprintf(https_proxy, "%s", buf);
	pclose(output);
        
    output = popen("gconftool-2 --get /system/proxy/secure_port", "r");
	fscanf(output , "%s", buf);
	sprintf(https_proxy, "%s:%s", https_proxy, buf);
	pclose(output);
    memset(buf, 0, MAXLEN);
    INFO("https_proxy : %s\n", https_proxy);

    output = popen("gconftool-2 --get /system/proxy/ftp_host", "r");
	fscanf(output , "%s", buf);
	sprintf(ftp_proxy, "%s", buf);
    pclose(output);
        
    output = popen("gconftool-2 --get /system/proxy/ftp_port", "r");
	fscanf(output , "%s", buf);
	sprintf(ftp_proxy, "%s:%s", ftp_proxy, buf);
	pclose(output);
    memset(buf, 0, MAXLEN);
    INFO("ftp_proxy : %s\n", ftp_proxy);
    
    output = popen("gconftool-2 --get /system/proxy/socks_host", "r");
	fscanf(output , "%s", buf);
	sprintf(socks_proxy, "%s", buf);
	pclose(output);
  
    output = popen("gconftool-2 --get /system/proxy/socks_port", "r");
	fscanf(output , "%s", buf);
	sprintf(socks_proxy, "%s:%s", socks_proxy, buf);
	pclose(output);
    INFO("socks_proxy : %s\n", socks_proxy);
}

/**
  @brief	get host proxy server address
  @param	proxy: return value (proxy server address)
  @return always 0
 */
int gethostproxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
#ifndef _WIN32
	char buf[255];
	FILE *output;

	output = popen("gconftool-2 --get /system/proxy/mode", "r");
	fscanf(output, "%s", buf);
	pclose(output);

	if (strcmp(buf, "manual") == 0){
        getlinuxproxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
			}else if (strcmp(buf, "auto") == 0){
		INFO( "Emulator can't support automatic proxy currently. starts up with normal proxy.\n");
		//can't support proxy auto setting
//		output = popen("gconftool-2 --get /system/proxy/autoconfig_url", "r");
//		fscanf(output , "%s", buf);
//		sprintf(proxy, "%s", buf);
//		pclose(output);
	}

#else
	HKEY hKey;
	int nRet;
	LONG lRet;
	BYTE *proxyenable, *proxyserver;
    char *p;
	char *real_proxy;

	DWORD dwLength = 0;
	nRet = RegOpenKeyEx(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
			0, KEY_QUERY_VALUE, &hKey);
	if (nRet != ERROR_SUCCESS) {
		fprintf(stderr, "Failed to open registry from %s\n",
				"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
		return 0;
	}
	lRet = RegQueryValueEx(hKey, "ProxyEnable", 0, NULL, NULL, &dwLength);
	if (lRet != ERROR_SUCCESS && dwLength == 0) {
		fprintf(stderr, "Failed to query value from from %s\n",
				"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
		RegCloseKey(hKey);
		return 0;
	}
	proxyenable = (BYTE*)malloc(dwLength);
	if (proxyenable == NULL) {
		fprintf(stderr, "Failed to allocate a buffer\n");
		RegCloseKey(hKey);
		return 0;
	}

	lRet = RegQueryValueEx(hKey, "ProxyEnable", 0, NULL, proxyenable, &dwLength);
	if (lRet != ERROR_SUCCESS) {
		free(proxyenable);
		fprintf(stderr, "Failed to query value from from %s\n",
				"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
		RegCloseKey(hKey);
		return 0;
	}
	if (*(char*)proxyenable == 0) {
		free(proxyenable);
		RegCloseKey(hKey);		
		return 0;
	}

	dwLength = 0;
	lRet = RegQueryValueEx(hKey, "ProxyServer", 0, NULL, NULL, &dwLength);
	if (lRet != ERROR_SUCCESS && dwLength == 0) {
		fprintf(stderr, "Failed to query value from from %s\n",
				"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
		RegCloseKey(hKey);		
		return 0;
	}

	proxyserver = (BYTE*)malloc(dwLength);
	if (proxyserver == NULL) {
		fprintf(stderr, "Failed to allocate a buffer\n");
		RegCloseKey(hKey);		
		return 0;
	}

	memset(proxyserver, 0x00, dwLength);
	lRet = RegQueryValueEx(hKey, "ProxyServer", 0, NULL, proxyserver, &dwLength);
	if (lRet != ERROR_SUCCESS) {
		free(proxyserver);
		fprintf(stderr, "Failed to query value from from %s\n",
				"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
		RegCloseKey(hKey);
		return 0;
	}
    if((char*)proxyserver != NULL) {
        INFO("proxy value: is %s\n", (char*)proxyserver);
        for(p = strtok((char*)proxyserver, ";"); p; p = strtok(NULL, ";")){
            real_proxy = malloc(MAXLEN);
            if(strstr(p, HTTP_PROTOCOL)) {
                remove_protocol(p, real_proxy, HTTP_PROTOCOL);
                strcpy(http_proxy, real_proxy);
            }
            else if(strstr(p, HTTPS_PROTOCOL)) {
                remove_protocol(p, real_proxy, HTTPS_PROTOCOL);
                strcpy(https_proxy, real_proxy);
            }
            else if(strstr(p, FTP_PROTOCOL)) {
                remove_protocol(p, real_proxy, FTP_PROTOCOL);
                strcpy(ftp_proxy, real_proxy);
            }
            else if(strstr(p, SOCKS_PROTOCOL)) {
                remove_protocol(p, real_proxy, SOCKS_PROTOCOL);
                strcpy(socks_proxy, real_proxy);
            }
            else {
                fprintf(stderr, "%s is wrong proxy format\n", p);
            }
        }
	}
    else {
        fprintf(stderr, "proxy is null\n");
        return 0;
    }
	RegCloseKey(hKey);
#endif
	return 0;
}
