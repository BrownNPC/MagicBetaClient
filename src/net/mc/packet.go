package mc

import "solod.dev/so/io"

type Packet interface {
	Read(io.Reader) error
	Write(io.Writer) error
}
