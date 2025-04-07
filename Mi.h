/**
 * Really simple data compression based on the Move-to-Front transform and a
 * mixed type of nibble-based integer encoding.
 */

#ifndef _MTF_IX_H
#define _MTF_IX_H

#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

typedef size_t (*MIIoFunc)(void *context, void *buffer, size_t length);

bool MICompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write);
bool MIDecompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write);

#ifndef MI_NO_STDIO
#include <stdio.h>
bool MICompressStream(FILE *input, FILE *output);
bool MIDecompressStream(FILE *input, FILE *output);
#endif

#endif

#ifdef MI_IMPLEMENTATION

#define MI_BUFFER_SIZE 2048

typedef struct MIWriteStream {
	void *context;
	MIIoFunc write;
	size_t position;
	char buffer[MI_BUFFER_SIZE];
} MIWriteStream;

static void MIWriteStreamInit(MIWriteStream *self, void *write_context, MIIoFunc write) {
	memset(self, 0, sizeof *self);
	self->write = write;
	self->context = write_context;
}

static bool MIFlush(MIWriteStream *self) {
	size_t to_write = (self->position >> 1) + (self->position & 1);
	
	size_t amount_written = self->write(self->context, self->buffer, to_write);
	
	if (amount_written != to_write) {
		return false;
	}
	
	memset(self->buffer, 0, to_write);
	self->position = 0;
	
	return true;
}

static bool MIWriteHd(MIWriteStream *self, char hexdigit) {
	/**
	 * Write a hexdigit to a hexdigit write stream
	 */
	
	if (self->position >= (2 * MI_BUFFER_SIZE)) {
		if (!MIFlush(self)) {
			return false;
		}
	}
	
	// Write hexdigit
	self->buffer[(self->position >> 1)] |= hexdigit << ((self->position & 1) ? 0 : 4);
	
	// Move to next hexdigit
	self->position++;
	
	return true;
}

static bool MIWriteCh(MIWriteStream *self, char ch) {
	/**
	 * Write a character to a character write stream
	 */
	
	if (self->position >= (2 * MI_BUFFER_SIZE)) {
		if (!MIFlush(self)) {
			return false;
		}
	}
	
	self->buffer[(self->position >> 1)] = ch;
	self->position += 2;
	
	return true;
}

typedef struct MIReadStream {
	void *context;
	MIIoFunc read;
	size_t position;
	size_t available;
	char buffer[MI_BUFFER_SIZE];
} MIReadStream;

static void MIReadStreamInit(MIReadStream *self, void *context, MIIoFunc read) {
	memset(self, 0, sizeof *self);
	self->read = read;
	self->context = context;
}

static bool MIEnsureDataIsAvailable(MIReadStream *self) {
	// If there is nothing in the buffer, try to read more
	if (self->available - self->position == 0) {
		size_t amount_read = self->read(self->context, self->buffer, MI_BUFFER_SIZE);
		self->available = amount_read << 1;
		self->position = 0;
		
		// Still nothing
		if (self->available == 0) {
			return false;
		}
	}
	
	return true;
}

static bool MIReadHd(MIReadStream *self, char *ch) {
	if (!MIEnsureDataIsAvailable(self)) {
		return false;
	}
	
	*ch = (self->buffer[(self->position >> 1)] >> ((self->position & 1) ? 0 : 4)) & 0xf;
	self->position++;
	
	return true;
}

static bool MIReadCh(MIReadStream *self, char *ch) {
	if (!MIEnsureDataIsAvailable(self)) {
		return false;
	}
	
	*ch = self->buffer[(self->position >> 1)];
	self->position += 2;
	
	return true;
}

typedef struct MIMtf {
	char stack[256];
} MIMtf;

static void MIMtfInit(MIMtf *mtf) {
	for (size_t i = 0; i < 256; i++) {
		mtf->stack[i] = i;
	}
}

static uint8_t MIMtfIndexForCh(MIMtf *mtf, char ch) {
	// Get MTF index for character
	uint16_t i = 0;
	
	// Find index
	for (; i < 256; i++) {
		if (mtf->stack[i] == ch) {
			break;
		}
	}
	
	// Move to front (roll credits etc.)
	memmove(mtf->stack + 1, mtf->stack, i);
	mtf->stack[0] = ch;
	
	// Return index
	return i;
}

static char MIMtfChForIndex(MIMtf *mtf, uint8_t i) {
	// Get character for MTF index
	char ch = mtf->stack[i];
	
	// Move to front
	memmove(mtf->stack + 1, mtf->stack, i);
	mtf->stack[0] = ch;
	
	// Return char
	return ch;
}

#define MI_CHECKED_WRITE(CH) if (!MIWriteHd(output, CH)) { return false; }

static bool MICompressUsingInternalStreams(MIReadStream *input, MIWriteStream *output) {
	MIMtf mtf;
	
	MIMtfInit(&mtf);
	
	while (true) {
		char ch;
		
		if (!MIReadCh(input, &ch)) {
			// HACK we just assume we're done until we have proper eof handling
			break;
		}
		
		// Get mtf index
		uint8_t i = MIMtfIndexForCh(&mtf, ch);
		
		// Is less than 15? We can just use one hexdigit then
		if (i < 15) {
			MI_CHECKED_WRITE(i);
		}
		else {
			// Write indicator that we use the long type
			MI_CHECKED_WRITE(0xf);
			
			// Subtract 15. Makes some representations smaller
			i -= 15;
			
			// Write LEB8 encoded integer
			do {
				MI_CHECKED_WRITE((i & 0x7) | ((i >> 3) ? 0x8 : 0));
				i = i >> 3;
			} while (i != 0);
		}
	}
	
	// End of message (where raw index is > 240)
	MI_CHECKED_WRITE(0b1111);
	MI_CHECKED_WRITE(0b1111);
	MI_CHECKED_WRITE(0b1111);
	MI_CHECKED_WRITE(0b0011);
	
	return MIFlush(output);
}

#undef MI_CHECKED_WRITE

#define MI_CHECKED_WRITE(CH) if (!MIWriteCh(output, CH)) { return false; }

static bool MIDecompressUsingInternalStreams(MIReadStream *input, MIWriteStream *output) {
	MIMtf mtf;
	
	MIMtfInit(&mtf);
	
	while (true) {
		char i;
		
		if (!MIReadHd(input, &i)) {
			return false;
		}
		
		if (i != 15) {
			// Common indicies
			char ch = MIMtfChForIndex(&mtf, i);
			MI_CHECKED_WRITE(ch);
		}
		else {
			// We need this to be zeroed first, lest we have Big Problems!
			i = 0;
			
			// Read LEB8 integer
			for (size_t j = 0;; j++) {
				// Valid numbers can't realistically be more than 3 nibbles.
				if (j > 2) {
					return false;
				}
				
				char tmp;
				
				if (!MIReadHd(input, &tmp)) {
					return false;
				}
				
				// Decode bits of byte
				i |= (tmp & 0x7) << (3 * j);
				
				// Break if bit is no longer set
				if (!(tmp & 0x8)) {
					break;
				}
			}
			
			// An extended symbol that has a value > 240 wouldn't have a valid
			// index (255 - 15 = 240) and is instead used to indicate the end of
			// the message.
			if ((uint8_t)i > 240) {
				break;
			}
			
			i += 15;
			
			// Finally
			char ch = MIMtfChForIndex(&mtf, i);
			MI_CHECKED_WRITE(ch);
		}
	}
	
	return MIFlush(output);
}

#undef MI_CHECKED_WRITE

bool MICompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write) {
	MIReadStream input; MIReadStreamInit(&input, input_ctx, read);
	MIWriteStream output; MIWriteStreamInit(&output, output_ctx, write);
	return MICompressUsingInternalStreams(&input, &output);
}

bool MIDecompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write) {
	MIReadStream input; MIReadStreamInit(&input, input_ctx, read);
	MIWriteStream output; MIWriteStreamInit(&output, output_ctx, write);
	return MIDecompressUsingInternalStreams(&input, &output);
}

#ifndef MI_NO_STDIO

static size_t MIStdioRead(void *f, void *buf, size_t size) {
	// Only here to emphasise that 0 is used to end reading
	if (feof((FILE *) f)) {
		return 0;
	}
	
	return fread(buf, 1, size, (FILE *) f);
}

static size_t MIStdioWrite(void *f, void *buf, size_t size) {
	return fwrite(buf, 1, size, (FILE *) f);
}

bool MICompressStream(FILE *input, FILE *output) {
	return MICompress(input, MIStdioRead, output, MIStdioWrite);
}

bool MIDecompressStream(FILE *input, FILE *output) {
	return MIDecompress(input, MIStdioRead, output, MIStdioWrite);
}

#endif

#undef MI_IMPLEMENTATION
#endif
