#pragma once
#include <curl/curl.h>
#include <stdbool.h>
#include "so/builtin/builtin.h"
bool CurlInit();
void CurlDeInit();
// NOTE: hostname must be prefixed with "http://"
so_Error CurlCreateSocket(const char* hostname, CURL** curlRet);
// Closes the socket. Does not wait for all data to be sent.
void CurlCloseSocket(CURL* curl);
// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
so_R_int_err CurlReadFromSocket(CURL* curl, void* buffer, size_t buflen);
// This is non-blocking.
// Returns the number of bytes written. Can be 0. -1 means error.
so_R_int_err CurlWriteToSocket(CURL* curl, const void* buffer, size_t buflen);
