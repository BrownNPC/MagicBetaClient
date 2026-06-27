package curl

import (
	"mbc/sdl"
	"solod.dev/so/c"
	"solod.dev/so/time"
)

//so:include <curl/curl.h>

//so:extern CURL_GLOBAL_DEFAULT
const CURL_GLOBAL_DEFAULT = (1 << 0) | (1 << 1)

//so:extern CURLcode
type CURLcode int

//so:extern CURLE_AGAIN
const CURLE_AGAIN CURLcode = 1

//so:extern
func curl_global_init(int) CURLcode

//so:extern
func curl_easy_strerror(CURLcode) *c.ConstChar

//so:extern
func curl_easy_cleanup(*CURL)

//so:extern
func curl_easy_init() *CURL

//so:extern
func curl_easy_setopt(c *CURL, opt any, value any)

//so:extern
func curl_easy_perform(*CURL) CURLcode

//so:extern
type size_t int

//so:extern
func curl_easy_recv(curl *CURL, buf *byte, size int, n *size_t) CURLcode

//so:extern
func curl_easy_send(curl *CURL, buf *byte, size int, n *size_t) CURLcode

//so:extern CURLOPT_URL
const CURLOPT_URL = 0

//so:extern CURLOPT_CONNECT_ONLY
const CURLOPT_CONNECT_ONLY = 0

//so:extern CURLOPT_TCP_NODELAY
const CURLOPT_TCP_NODELAY = 0

//so:extern CURL
type CURL struct{}

type CurlError struct {
	code CURLcode
}

func (e *CurlError) Error() string {
	return c.String(curl_easy_strerror(e.code))
}

var _Error CurlError

// NOTE: hostname must be prefixed with "http://"
func CreateSocket(hostname string) (*CURL, error) {
	curl := curl_easy_init()
	curl_easy_setopt(curl, CURLOPT_URL, c.CString(hostname))
	curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, c.Long(1))
	curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, c.Long(1))

	code := curl_easy_perform(curl)
	if code != 0 {
		_Error.code = code
		curl_easy_cleanup(curl)
		return nil, &_Error
	}
	return curl, nil
}

// Closes the socket. Does not wait for all data to be sent.
func CloseSocket(curl *CURL) {
	curl_easy_cleanup(curl)
}

// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
func ReadFromSocket(curl *CURL, buffer *byte, size int) (int, error) {
	var n size_t
	code := curl_easy_recv(curl, buffer, size, &n)
	if code == CURLE_AGAIN {
		return int(n), nil
	} else {
	}
	if code != 0 {
		curl_easy_cleanup(curl)
		_Error.code = code
		return int(n), &_Error
	}
	return int(n), nil
}

// WriteToSocket blocks until all bytes are written or an error occurs.
// Returns the number of bytes written before an error, if any.
func WriteToSocket(curl *CURL, buffer *byte, size int) (int, error) {
	total := 0

	for total < size {
		var n size_t

		code := curl_easy_send(
			curl,
			buffer,
			size-total,
			&n,
		)

		total += int(n)

		switch code {
		case 0: // no error, keep writing.

		case CURLE_AGAIN:
			// Retry until writable.
			sdl.Delay(time.Millisecond)
			continue

		default:
			curl_easy_cleanup(curl)
			_Error.code = code
			return total, &_Error
		}
	}

	return total, nil
}

func init() {
	curl_global_init(CURL_GLOBAL_DEFAULT)
}
