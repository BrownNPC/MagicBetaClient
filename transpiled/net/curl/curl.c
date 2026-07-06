#include "curl.h"

// -- Variables and constants --
static curl_CurlError _Error = {0};

// -- Implementation --

so_String curl_CurlError_Error(void* self) {
    curl_CurlError* e = self;
    return c_String(const char, (curl_easy_strerror(e->code)));
}

// NOTE: hostname must be prefixed with "http://"
so_R_ptr_err curl_CreateSocket(so_String hostname) {
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, so_cstr(hostname));
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, (long)(1));
    CURLcode code = curl_easy_perform(curl);
    if (code != 0) {
        _Error.code = code;
        curl_easy_cleanup(curl);
        return (so_R_ptr_err){.val = NULL, .err = (so_Error){.self = &_Error, .Error = curl_CurlError_Error}};
    }
    return (so_R_ptr_err){.val = curl, .err = (so_Error){0}};
}

// Closes the socket. Does not wait for all data to be sent.
void curl_CloseSocket(CURL* curl) {
    curl_easy_cleanup(curl);
}

// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
so_R_int_err curl_ReadFromSocket(CURL* curl, so_byte* buffer, so_int size) {
    size_t n = 0;
    CURLcode code = curl_easy_recv(curl, buffer, size, &n);
    if (code == CURLE_AGAIN) {
        return (so_R_int_err){.val = (so_int)(n), .err = (so_Error){0}};
    }
    if (code != 0) {
        curl_easy_cleanup(curl);
        _Error.code = code;
        return (so_R_int_err){.val = (so_int)(n), .err = (so_Error){.self = &_Error, .Error = curl_CurlError_Error}};
    }
    return (so_R_int_err){.val = (so_int)(n), .err = (so_Error){0}};
}

// WriteToSocket blocks until all bytes are written or an error occurs.
// Returns the number of bytes written before an error, if any.
so_R_int_err curl_WriteToSocket(CURL* curl, so_byte* buffer, so_int size) {
    so_int total = 0;
    for (; total < size;) {
        size_t n = 0;
        CURLcode code = curl_easy_send(curl, (so_byte*)(unsafe_Add((void*)(buffer), total)), size - total, &n);
        total += (so_int)(n);
        if (code == (0)) {
        } else if (code == (CURLE_AGAIN)) {
            // Retry until writable.
            SDL_DelayNS(time_Millisecond);
            continue;
        } else {
            curl_easy_cleanup(curl);
            _Error.code = code;
            return (so_R_int_err){.val = total, .err = (so_Error){.self = &_Error, .Error = curl_CurlError_Error}};
        }
    }
    return (so_R_int_err){.val = total, .err = (so_Error){0}};
}

static void __attribute__((constructor)) curl_init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}
