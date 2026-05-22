package curl

//so:embed curl.h
var _ string

//so:embed curl.c
var _ string

//so:extern CURL
type CURL struct{}

// // NOTE: hostname must be prefixed with "http://"
// bool CurlCreateSocket(const char* hostname,
//                              CURL** curlRet,
//                              const char** errorOut);

// NOTE: hostname must be prefixed with "http://"
//
//so:extern CurlCreateSocket
func CreateSocket(hostname string, curlRet **CURL) error

// Closes the socket. Does not wait for all data to be sent.
//
//so:extern CurlCloseSocket
func CloseSocket(curl *CURL)

// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
//
//so:extern CurlReadFromSocket
func ReadFromSocket(curl *CURL, buffer any, size int) (int, error)

// This is non-blocking.
// Returns the number of bytes written. Can be 0. -1 means error.
//
//so:extern CurlWriteToSocket
func WriteToSocket(curl *CURL, buffer any, size int) (int, error)

//so:extern CurlInit
func curlGlobalInit() bool

func init() {
	curlGlobalInit()
}

//so:extern CurlDeInit
func DeInit()
