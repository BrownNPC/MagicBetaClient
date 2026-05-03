#include <SDL3/SDL_iostream.h>
#include <curl/curl.h>
#include "core.h"

bool CurlInit();
void CurlDeInit();

// NOTE: hostname must be prefixed with "http://"
ResultDef(CurlCreateSocket, CURL*, error);
CurlCreateSocketResult CurlCreateSocket(string hostname);

// Closes the socket. Does not wait for all data to be sent.
void CurlCloseSocket(CURL* curl);
// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
// Error means no more bytes can be read/written. and you should clean up.
int CurlReadFromSocket(CURL* curl, void* buffer, size_t buflen);
// This is non-blocking.
// Returns the number of bytes written. Can be 0. -1 means error.
// Error means no more bytes can be read/written. and you should clean up.
int CurlWriteToSocket(CURL* curl, const void* buffer, size_t buflen);

ResultDef(ConnDial, SDL_IOStream*, error);
ConnDialResult ConnDial(EM* em, string hostname);
