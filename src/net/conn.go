package net

import (
	"betamine/net/curl"
	"betamine/sdl"

	"solod.dev/so/errors"
	"solod.dev/so/time"
)

var ErrResolveFailed = errors.New("Could not resolve hostname.")
var ErrConnectionFailed = errors.New("TCP connection failed.")
var ErrConnectionClosed = errors.New("Connection closed.")

// Read blocks until bytes are read or there is an error.
// It does not guarantee that the full buffer is used.
func (conn *Conn) Read(b []byte) (int, error) {
	if conn.closed { // already errored.
		return 0, ErrConnectionClosed
	}

	if len(b) == 0 {
		return 0, nil
	}
	for {
		n, err := curl.ReadFromSocket(conn.sock, &b[0], len(b))
		if err != nil {
			// conn.Close()
			return 0, err
		}
		if n != 0 {
			return n, nil
		}
		sdl.Delay(time.Millisecond * 50)
	}
}

// Write blocks until all bytes from the buffer have been written.
func (conn *Conn) Write(b []byte) (int, error) {
	if conn.closed {
		return 0, ErrConnectionClosed
	}

	total := 0
	for total < len(b) {
		n, err := curl.WriteToSocket(conn.sock, &b[total], len(b)-total)
		if err != nil {
			conn.Close()
			return total, err
		}

		if n == 0 {
			// avoid tight spin
			sdl.Delay(time.Millisecond * 50)
			continue
		}

		total += n
	}

	return total, nil
}

// Free Closes the connection and frees memory.
func (conn *Conn) Close() {
	if conn.closed {
		return
	}
	conn.closed = true
	curl.CloseSocket(conn.sock)
}

// Conn is a TCP client connection.
type Conn struct {
	closed bool
	sock   *curl.CURL
}
type ConnResult struct {
	val Conn
	err error
}

// Dial dials the connection with a default timeout.
func Dial(host string) (Conn, error) {
	host = "http://" + host
	conn := Conn{}
	err := curl.CreateSocket(host, &conn.sock)
	return conn, err
}
