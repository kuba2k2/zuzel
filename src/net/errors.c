// Copyright (c) Kuba Szczodrzyński 2025-1-3.

#include "net.h"

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
	LT_F("%s", message);
	LocalFree(message);
	return true;
}
