/**
 * Very basic compression algorithm
 */

#ifndef _KNOT_COMPRESS_H_
#define _KNOT_COMPRESS_H_

#include <stdlib.h>

typedef size_t (*KCReadWrite)(void *stream, void *buffer, size_t size);

#endif

#ifdef KNOT_COMPRESS_IMPLEMENTATION
#undef KNOT_COMPRESS_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifndef KNOT_COMPRESS_BUFSIZE
#define KNOT_COMPRESS_BUFSIZE 1024
#endif
#define KNOT_COMPRESS_BUFSIZE_BITS (8 * KNOT_COMPRESS_BUFSIZE)

// Bit write stream
typedef struct {
	KCReadWrite write;
	void *stream;
	char buffered[KNOT_COMPRESS_BUFSIZE];
	size_t head;
} BitWriteStream;

static bool InitBitWriteStream(BitWriteStream *self, KCReadWrite writefunc, void *stream) {
	self->write = writefunc;
	self->stream = stream;
	memset(self->buffered, 0, KNOT_COMPRESS_BUFSIZE);
	self->head = 0;
	return true;
}

static bool WriteBit(BitWriteStream *self, bool bit) {
	if (self->head == KNOT_COMPRESS_BUFSIZE_BITS) {
		size_t written = self->write(self->stream, self->buffered, KNOT_COMPRESS_BUFSIZE);
		
		if (written != KNOT_COMPRESS_BUFSIZE) {
			return false;
		}
		
		memset(self->buffered, 0, KNOT_COMPRESS_BUFSIZE);
		self->head = 0;
	}
	
	size_t i = self->head >> 3;
	self->buffered[i] |= bit << (7 - (self->head & 0b111));
	self->head++;
	
	return true;
}

static bool Flush(BitWriteStream *self) {
	// Byte align
	if (self->head % 8) {
		self->head += 8 - (self->head % 8);
	}
	
	size_t written = self->write(self->stream, self->buffered, self->head >> 3);
	
	if (written != (self->head >> 3)) {
		return false;
	}
	
	memset(self->buffered, 0, self->head >> 3);
	self->head = 0;
	
	return true;
}

#ifdef KNOT_TESTING
bool TestWriteStream(void *context, KCReadWrite func) {
	BitWriteStream bws;
	
	InitBitWriteStream(&bws, func, context);
	
	for (size_t i = 0; i < 256; i++) {
		for (size_t j = 0; j < i; j++) {
			if (!WriteBit(&bws, 1)) {
				return false;
			}
		}
		
		if (!WriteBit(&bws, 0)) {
			return false;
		}
	}
	
	if (!Flush(&bws)) {
		return false;
	}
	
	return true;
}
#endif

#endif
