/**
 * Really simple data compression based on the Move-to-Front transform and a
 * mixed type of nibble-based integer encoding.
 */

#ifndef _MI_H
#define _MI_H

#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

typedef size_t (*MIIoFunc)(void *context, void *buffer, size_t length);

const char *MICompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write);
const char *MIDecompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write);

#ifndef MI_NO_STDIO
#include <stdio.h>
const char *MICompressStream(FILE *input, FILE *output);
const char *MIDecompressStream(FILE *input, FILE *output);
#endif

#endif

#ifdef MI_IMPLEMENTATION

#define MI_NO_ERROR (MIErrorInfo){.success = true}

#define MI_BUFFER_SIZE 16384

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

#define MI_CHECKED_WRITE(CH) if (!MIWriteHd(output, CH)) { return "Write error"; }

static const char *MICompressUsingInternalStreams(MIReadStream *input, MIWriteStream *output) {
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
	
	if (!MIFlush(output)) {
		return "Write error";
	}
	
	return NULL;
}

#undef MI_CHECKED_WRITE

#define MI_CHECKED_WRITE(CH) if (!MIWriteCh(output, CH)) { return "Write error"; }

static const char *MIDecompressUsingInternalStreams(MIReadStream *input, MIWriteStream *output) {
	MIMtf mtf;
	
	MIMtfInit(&mtf);
	
	while (true) {
		char i;
		
		if (!MIReadHd(input, &i)) {
			return "Read error";
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
					return "Number is too long";
				}
				
				char tmp;
				
				if (!MIReadHd(input, &tmp)) {
					return "Read error";
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
	
	if (!MIFlush(output)) {
		return "Write error";
	}
	
	return NULL;
}

#undef MI_CHECKED_WRITE

const char *MICompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write) {
	MIReadStream input; MIReadStreamInit(&input, input_ctx, read);
	MIWriteStream output; MIWriteStreamInit(&output, output_ctx, write);
	return MICompressUsingInternalStreams(&input, &output);
}

const char *MIDecompress(void *input_ctx, MIIoFunc read, void *output_ctx, MIIoFunc write) {
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

const char *MICompressStream(FILE *input, FILE *output) {
	return MICompress(input, MIStdioRead, output, MIStdioWrite);
}

const char *MIDecompressStream(FILE *input, FILE *output) {
	return MIDecompress(input, MIStdioRead, output, MIStdioWrite);
}

#endif

#ifdef MI_COMPILE_MAIN

#ifdef MI_NO_STDIO
#error Cannot compile Mi main() with stdio not enabled.
#endif

void print_help(void) {
	fprintf(stderr, "Mi compression utility\n\nmi [args ...] <input> <output>\n\n\t-d, -D, --decompress: Enable decompress mode.\n\n");
}

int main(int argc, char *argv[]) {
	bool decompress = false;
	
	if (argc < 3) {
		fprintf(stderr, "Not enough arguments\n\n");
		print_help();
		return 1;
	}
	
	// Parse command line arguments
	for (size_t i = 1; i < argc - 2; i++) {
		if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "-D") || !strcmp(argv[i], "--decompress")) {
			decompress = true;
		}
		else {
			fprintf(stderr, "Unrecognised argument: %s\n\n", argv[0]);
			print_help();
			return 1;
		}
	}
	
	// Open input file
	bool using_stdin = !strcmp(argv[argc - 2], "-");
	FILE *input = using_stdin ? stdin : fopen(argv[argc - 2], "rb");
	
	if (!input) {
		fprintf(stderr, "Failed to open input stream\n");
		return 2;
	}
	
	// Open output file
	bool using_stdout = !strcmp(argv[argc - 1], "-");
	FILE *output = using_stdout ? stdout : fopen(argv[argc - 1], "wb");
	
	if (!output) {
		fprintf(stderr, "Failed to open output stream\n");
		return 2;
	}
	
	// Decompress
	if (decompress) {
		const char *error = MIDecompressStream(input, output);
		
		if (error) {
			fprintf(stderr, "Decompression failed: %s\n", error);
			return 3;
		}
	}
	// Compress
	else {
		const char *error = MICompressStream(input, output);
		
		if (error) {
			fprintf(stderr, "Compression failed: %s\n", error);
			return 3;
		}
	}
	
	// Close files (if not using stdin/stdout)
	if (!using_stdin) {
		fclose(input);
	}
	
	if (!using_stdout) {
		fclose(output);
	}
	
	return 0;
}
#endif


#undef MI_IMPLEMENTATION
#endif
