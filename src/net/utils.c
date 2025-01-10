// Copyright (c) Kuba Szczodrzyński 2025-1-3.

#include "net.h"

#if WIN32
bool net_error_print() {
	char *message = NULL;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char *)&message,
		0,
		NULL
	);
	size_t len = strlen(message);
	if (message[len - 1] == '\n')
		message[--len] = '\0';
	if (message[len - 1] == '\r')
		message[--len] = '\0';
	LT_F("%s", message);
	LocalFree(message);
	return true;
}
#else
bool net_error_print() {
	return false;
}
#endif

static bool net_getaddrinfo(const char *address, struct addrinfo **info) {
	struct addrinfo hints = {
		.ai_family	 = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};
	LT_D("Resolving address '%s'", address);
	int gai_error = getaddrinfo(address, NULL, &hints, info);
#if WIN32
	WSASetLastError(gai_error); // fix for WSANOTINITIALISED not being delivered otherwise
#endif
	if (gai_error != 0) {
		SOCK_ERROR("getaddrinfo()", );
		LT_D("getaddrinfo() returned %s", gai_strerror(gai_error));
		return false;
	}
	return true;
}

bool net_resolve_ip(const char *address, struct in_addr *addr) {
	struct addrinfo *info = NULL;
	if (!net_getaddrinfo(address, &info))
		return false;
	*addr = ((struct sockaddr_in *)info->ai_addr)->sin_addr;
	freeaddrinfo(info);
	return true;
}

char *net_get_local_ips() {
#if WIN32
	char hostname[128];
	if (gethostname(hostname, sizeof(hostname)) != 0)
		return NULL;
	struct addrinfo *infos, *info;
	if (!net_getaddrinfo(hostname, &infos))
		return false;
	int addr_count;
	LL_COUNT2(infos, info, addr_count, ai_next);
#else
	struct ifaddrs *ifaddrs, *ifaddr;
	if (getifaddrs(&ifaddrs) == -1)
		SOCK_ERROR("getifaddrs()", return false);
	int addr_count;
	LL_COUNT2(ifaddrs, ifaddr, addr_count, ifa_next);
#endif

	char *addrs = NULL;
	MALLOC(addrs, (sizeof("255.255.255.255\n") - 1) * addr_count + sizeof('\0'), goto error);
	char *addrs_ptr = addrs;

#if WIN32
	LL_FOREACH2(infos, info, ai_next) {
		const char *addr = inet_ntoa(((struct sockaddr_in *)info->ai_addr)->sin_addr);
#else
	LL_FOREACH2(ifaddrs, ifaddr, ifa_next) {
		const char *addr = inet_ntoa(((struct sockaddr_in *)ifaddr->ifa_addr)->sin_addr);
		if (ifaddr->ifa_addr->sa_family != AF_INET)
			continue;
#endif
		if (strncmp(addr, "169.254.", 8) == 0 || strcmp(addr, "127.0.0.1") == 0 || strcmp(addr, "0.0.0.0") == 0)
			// skip link-local and localhost
			continue;
		if (strncmp(addr, "192.168.56.", 11) == 0 || strncmp(addr, "192.168.99.", 11) == 0)
			// skip VirtualBox addresses
			continue;
		size_t addr_len = strlen(addr);
		if (addrs_ptr != addrs)
			*addrs_ptr++ = '\n';
		memcpy(addrs_ptr, addr, addr_len);
		addrs_ptr += addr_len;
	}
	*addrs_ptr = '\0';

#if WIN32
	freeaddrinfo(info);
#else
	freeifaddrs(ifaddrs);
#endif
	return addrs;

error:
	free(addrs);
#if WIN32
	freeaddrinfo(info);
#else
	freeifaddrs(ifaddrs);
#endif
	return NULL;
}
