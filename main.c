#ifndef NO_FDOPEN
#define _POSIX_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include "cborsplit.h"

uint8_t buf_cbor[65536];
uint8_t buf_misc[65536];
uint8_t buf_text[65536];

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  cborsplit {-s|--split} in out1 out2\n");
        fprintf(stderr, "  cborsplit {-m|--merge} in1 in2 out\n");
        fprintf(stderr, "    '-' instead of in/out means stdin/stdout\n");
#ifndef NO_FDOPEN
        fprintf(stderr, "    '-' instead of out1/out2/in1/in2 means fd 3/fd 4/fd 3/fd 4\n");
#endif
        return 1;
    }
    
    if (!strcmp(argv[1], "-s") || !strcmp(argv[1], "--split")) {
        
        FILE* f_cbor;
        FILE* f_misc;
        FILE* f_text;
        
        if (!strcmp(argv[2], "-")) {
            f_cbor=stdin;
        } else {
            f_cbor=fopen(argv[2], "rb");
            if (!f_cbor) { perror("fopen"); return 2; }
        }
        
        if (!strcmp(argv[3], "-")) {
#ifndef NO_FDOPEN
            f_misc = fdopen(3, "wb");
            if (!f_misc) { perror("fdopen"); return 1; }
#else
            { fprintf(stderr, "fdopen support not compiled in\n"); return 1; }
#endif
        } else {
            f_misc=fopen(argv[3], "wb");
            if (!f_misc) { perror("fopen"); return 2; }
        }
        
        if (!strcmp(argv[4], "-")) {
#ifndef NO_FDOPEN
            f_text = fdopen(4, "wb");
            if (!f_text) { perror("fdopen"); return 1; }
#else
            { fprintf(stderr, "fdopen support not compiled in\n"); return 1; }
#endif
        } else {
            f_text=fopen(argv[4], "wb");
            if (!f_text) { perror("fopen"); return 2; }
        }
        
        struct cbsp_splitter s;
        
        memset(&s, 0, sizeof(s));
        
        s.cbor.buffer = buf_cbor;
        s.cbor.len = 0;
        s.misc.buffer = buf_misc;
        s.misc.capacity = sizeof buf_misc;
        s.text.buffer = buf_text;
        s.text.capacity = sizeof buf_text;
        
        int reading_finished = 0;
        int force_write = 0;
        
        uint64_t bytes_processed = 0;
        
        for(;;) {
            if (s.cbor.len < 12 && !reading_finished) {
                memmove(buf_cbor, s.cbor.buffer, s.cbor.len);
                size_t ret = fread(
                                    buf_cbor+s.cbor.len,
                                    1, 
                                    sizeof buf_cbor - s.cbor.len, 
                                    f_cbor);
                if (ret < sizeof buf_cbor - s.cbor.len) {
                    reading_finished = 1;
                }
                s.cbor.buffer = buf_cbor;
                s.cbor.len += ret;
            }
            
            if (s.misc.capacity < 12 || force_write) {
                size_t ret = fwrite(buf_misc, 1, s.misc.buffer - buf_misc, f_misc);
                if (ret < s.misc.buffer - buf_misc) {
                    perror("fwrite short write");
                    return 3;
                }
                s.misc.capacity += ret;
                s.misc.buffer -= ret;
            }
            
            if (s.text.capacity < 64 || force_write) {
                size_t ret = fwrite(buf_text, 1, s.text.buffer - buf_text, f_text);
                if (ret < s.text.buffer - buf_text) {
                    perror("fwrite short write");
                    return 3;
                }
                s.text.capacity += ret;
                s.text.buffer -= ret;
            }
            
            const uint8_t *saved_buf = s.cbor.buffer;
            
#ifdef DEBUG
            fprintf(stderr, "Processing at offset %llu\n", bytes_processed);
#endif
            
            enum cbsp_step_result res = cbsp_splitter_step(&s);
            
            bytes_processed += s.cbor.buffer - saved_buf;
            
            switch (res) {
                case CBSP_INVAL:
                    // shoud not happen
                    return 5;
                case CBSP_DATAERR:
                    fprintf(stderr, "Error in data at pos %llu\n", (long long unsigned)bytes_processed);
                    return 4;
                case CBSP_OK:
                    // OK
                    if (reading_finished && s.cbor.len == 0) {
                        force_write = 1;
                    }
                    break;
                case CBSP_MOREDATA:
                    if (reading_finished && s.cbor.len > 0) {
                        fprintf(stderr, "Trimmed data\n");
                        return 4;
                    }
                    break;
            }
            
            if (force_write && s.text.buffer == buf_text && s.misc.buffer == buf_misc) {
                break;
            }
        } // for(;;)
        
    } else
    if (!strcmp(argv[1], "-m") || !strcmp(argv[1], "--merge") || !strcmp(argv[1], "-j")) {
        
    } else {
        fprintf(stderr, "First argument should be --split (-s) or --merge (-m)\n");
        return 1;
    }
    
    return 0;
}
