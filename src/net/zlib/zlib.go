package zlib

import "solod.dev/so/errors"

//so:include "miniz.h"

//so:extern
func tinfl_decompress_mem_to_mem(out any, outsize int, src any, srcLen int, flags int) int

var TinflDecompressFailed = errors.New("Decompress failed")

// decompressed buffer must have enough space to hold the decompressed data.
func DecompressData(compressed []byte, decompressed []byte) (int, error) {
	n := tinfl_decompress_mem_to_mem(
		&decompressed[0], len(decompressed),
		&compressed[0], len(compressed),
		TINFL_FLAG_PARSE_ZLIB_HEADER,
	)
	if n == -1 {
		return 0, TinflDecompressFailed
	}
	return n, nil
}

const (
	/* This flags indicates the inflator needs 1 or more input bytes to make forward progress, but the caller is indicating that no more are available. The compressed data */
	/* is probably corrupted. If you call the inflator again with more bytes it'll try to continue processing the input but this is a BAD sign (either the data is corrupted or you called it incorrectly). */
	/* If you call it again with no input you'll just get TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS again. */
	TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS = -4

	/* This flag indicates that one or more of the input parameters was obviously bogus. (You can try calling it again, but if you get this error the calling code is wrong.) */
	TINFL_STATUS_BAD_PARAM = -3

	/* This flags indicate the inflator is finished but the adler32 check of the uncompressed data didn't match. If you call it again it'll return TINFL_STATUS_DONE. */
	TINFL_STATUS_ADLER32_MISMATCH = -2

	/* This flags indicate the inflator has somehow failed (bad code, corrupted input, etc.). If you call it again without resetting via tinfl_init() it it'll just keep on returning the same status failure code. */
	TINFL_STATUS_FAILED = -1

	/* Any status code less than TINFL_STATUS_DONE must indicate a failure. */

	/* This flag indicates the inflator has returned every byte of uncompressed data that it can, has consumed every byte that it needed, has successfully reached the end of the deflate stream, and */
	/* if zlib headers and adler32 checking enabled that it has successfully checked the uncompressed data's adler32. If you call it again you'll just get TINFL_STATUS_DONE over and over again. */
	TINFL_STATUS_DONE = 0

	/* This flag indicates the inflator MUST have more input data (even 1 byte) before it can make any more forward progress, or you need to clear the TINFL_FLAG_HAS_MORE_INPUT */
	/* flag on the next call if you don't have any more source data. If the source data was somehow corrupted it's also possible (but unlikely) for the inflator to keep on demanding input to */
	/* proceed, so be sure to properly set the TINFL_FLAG_HAS_MORE_INPUT flag. */
	TINFL_STATUS_NEEDS_MORE_INPUT = 1

	/* This flag indicates the inflator definitely has 1 or more bytes of uncompressed data available, but it cannot write this data into the output buffer. */
	/* Note if the source compressed data was corrupted it's possible for the inflator to return a lot of uncompressed data to the caller. I've been assuming you know how much uncompressed data to expect */
	/* (either exact or worst case) and will stop calling the inflator and fail after receiving too much. In pure streaming scenarios where you have no idea how many bytes to expect this may not be possible */
	/* so I may need to add some code to address this. */
	TINFL_STATUS_HAS_MORE_OUTPUT = 2
)
const (
	TINFL_FLAG_PARSE_ZLIB_HEADER             = 1
	TINFL_FLAG_HAS_MORE_INPUT                = 2
	TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4
	TINFL_FLAG_COMPUTE_ADLER32               = 8
)
