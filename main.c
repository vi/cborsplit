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
                case CBSP_MOREDATA_TEXT:
                    // should not happen
                    return 5;
            }
            
            if (force_write && s.text.buffer == buf_text && s.misc.buffer == buf_misc) {
                break;
            }
        } // for(;;)
              
        
        
    } else
    if (!strcmp(argv[1], "-m") || !strcmp(argv[1], "--merge") || !strcmp(argv[1], "-j")) {
        
        
        
        FILE* f_misc;
        FILE* f_text;
        FILE* f_cbor;
        
        if (!strcmp(argv[2], "-")) {
#ifndef NO_FDOPEN
            f_misc = fdopen(3, "rb");
            if (!f_misc) { perror("fdopen"); return 1; }
#else
            { fprintf(stderr, "fdopen support not compiled in\n"); return 1; }
#endif
        } else {
            f_misc=fopen(argv[2], "rb");
            if (!f_misc) { perror("fopen"); return 2; }
        }
        
        if (!strcmp(argv[3], "-")) {
#ifndef NO_FDOPEN
            f_text = fdopen(4, "rb");
            if (!f_text) { perror("fdopen"); return 1; }
#else
            { fprintf(stderr, "fdopen support not compiled in\n"); return 1; }
#endif
        } else {
            f_text=fopen(argv[3], "rb");
            if (!f_text) { perror("fopen"); return 2; }
        }
        
        
        if (!strcmp(argv[4], "-")) {
            f_cbor=stdout;
        } else {
            f_cbor=fopen(argv[4], "wb");
            if (!f_cbor) { perror("fopen"); return 2; }
        }
        
        
        
        struct cbsp_merger s;
        
        memset(&s, 0, sizeof(s));
        
        s.misc.buffer = buf_misc;
        s.misc.len = 0;
        s.text.buffer = buf_text;
        s.text.len = 0;
        s.cbor.buffer = buf_cbor;
        s.cbor.capacity = sizeof buf_cbor;
        
        int reading_finished_misc = 0;
        int reading_finished_text = 0;
        int force_write = 0;
        
        uint64_t bytes_processed = 0;
        
        for(;;) {
            if (s.misc.len < 12 && !reading_finished_misc) {
                memmove(buf_misc, s.misc.buffer, s.misc.len);
                size_t ret = fread(
                                    buf_misc+s.misc.len,
                                    1, 
                                    sizeof buf_misc - s.misc.len, 
                                    f_misc);
                if (ret < sizeof buf_misc - s.misc.len) {
                    reading_finished_misc = 1;
                }
                s.misc.buffer = buf_misc;
                s.misc.len += ret;
            }
            
            if (s.text.len < 64 && !reading_finished_text) {
                memmove(buf_text, s.text.buffer, s.text.len);
                size_t ret = fread(
                                    buf_text+s.text.len,
                                    1, 
                                    sizeof buf_text - s.text.len, 
                                    f_text);
                if (ret < sizeof buf_text - s.text.len) {
                    reading_finished_text = 1;
                }
                s.text.buffer = buf_text;
                s.text.len += ret;
            }
            
            if (s.cbor.capacity < 64 || force_write) {
                size_t ret = fwrite(buf_cbor, 1, s.cbor.buffer - buf_cbor, f_cbor);
                if (ret < s.cbor.buffer - buf_cbor) {
                    perror("fwrite short write");
                    return 3;
                }
                s.cbor.capacity += ret;
                s.cbor.buffer -= ret;
            }
            
            
            const uint8_t *saved_buf = s.misc.buffer;
            
#ifdef DEBUG
            fprintf(stderr, "Processing at offset %llu\n", bytes_processed);
            fprintf(stderr, "Currently there are %d bytes in misc, %d bytes in text and %d available\n", 
                            (int)s.misc.len, (int)s.text.len, (int)s.cbor.capacity
                   );
#endif
            
            enum cbsp_step_result res = cbsp_merger_step(&s);
            
            bytes_processed += s.misc.buffer - saved_buf;
            
            switch (res) {
                case CBSP_INVAL:
                    // shoud not happen
                    return 5;
                case CBSP_DATAERR:
                    fprintf(stderr, "Error in data at pos %llu in first input\n", (long long unsigned)bytes_processed);
                    return 4;
                case CBSP_OK:
                    // OK
                    if (reading_finished_misc && s.misc.len == 0) {
                        force_write = 1;
                    }
                    break;
                case CBSP_MOREDATA:
                    if (reading_finished_misc && s.misc.len > 0) {
                        fprintf(stderr, "Trimmed misc data\n");
                        return 4;
                    }
                    break;
                case CBSP_MOREDATA_TEXT:
                    if (reading_finished_text) {
                        fprintf(stderr, "Trimmed text data\n");
                        return 4;
                    }
                    break;
            }
            
            if (force_write && s.cbor.buffer == buf_cbor) {
                break;
            }
            
        }//for(;;)
        
    } else {
        fprintf(stderr, "First argument should be --split (-s) or --merge (-m)\n");
        return 1;
    }
    
    return 0;
}
