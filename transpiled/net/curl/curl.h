#pragma once
#include "so/builtin/builtin.h"
#include <curl/curl.h>
#include "sdl/sdl.h"
#include "so/c/c.h"
#include "so/time/time.h"

// -- Types --

typedef struct curl_CurlError curl_CurlError;

typedef struct curl_CurlError {
    CURLcode code;
} curl_CurlError;

// -- Functions and methods --
so_String curl_CurlError_Error(void* self);

// NOTE: hostname must be prefixed with "http://"
so_R_ptr_err curl_CreateSocket(so_String hostname);

// Closes the socket. Does not wait for all data to be sent.
void curl_CloseSocket(CURL* curl);

// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
so_R_int_err curl_ReadFromSocket(CURL* curl, so_byte* buffer, so_int size);

// WriteToSocket blocks until all bytes are written or an error occurs.
// Returns the number of bytes written before an error, if any.
so_R_int_err curl_WriteToSocket(CURL* curl, so_byte* buffer, so_int size);
