
// CBOR splitter/joiner. Text and bytestrings goes to one stream, everything else the other.
// Designed and implemented by Vitaly "_Vi" Shukela in 2017.
// License is MIT + Apache 2

#include <string.h>

#include "cborsplit.h"


//#define DEBUG

static uint64_t read_num(uint8_t addi, const uint8_t *buf)
{
    #define B(i) ((uint64_t)buf[i])
    if      (addi <= 23) return  addi;
    else if (addi == 24) return  B(0);
    else if (addi == 25) return (B(0)<<8)  | (B(1));
    else if (addi == 26) return (B(0)<<24) | (B(1) << 16) | (B(2)<<8 ) | (B(3)      );
    else if (addi == 27) return (B(0)<<56) | (B(1) << 48) | (B(2)<<40) | (B(3) << 32)
                              | (B(4)<<24) | (B(5) << 16) | (B(6)<<8 ) | (B(7)      );
    return 0xDEADBEEF;
    #undef B
}

#ifdef DEBUG
#include <stdio.h>
#endif

enum cbsp_step_result cbsp_splitter_step(struct cbsp_splitter* s)
{
    if (s->cbor.len == 0) return CBSP_MOREDATA;
    if (s->cbor.buffer == NULL) return CBSP_INVAL;
    if (s->misc.capacity == 0) return CBSP_MOREDATA;
    if (s->misc.buffer == NULL) return CBSP_INVAL;
    if (s->text.capacity == 0) return CBSP_MOREDATA;
    if (s->text.buffer == NULL) return CBSP_INVAL;
    
    #define COPY(from, to, bytes) \
                    if (bytes == -1) return CBSP_DATAERR; \
                    if (s->to.capacity < bytes) { \
                        return CBSP_MOREDATA; \
                    } \
                    if (s->from.len < bytes) { \
                        return CBSP_MOREDATA; \
                    } \
                    memcpy(s->to.buffer, s->from.buffer, bytes); \
                    s->to.buffer   += bytes; \
                    s->to.capacity -= bytes; \
                    s->from.buffer     += bytes; \
                    s->from.len        -= bytes;
    
    struct _cbsp_data* d = &s->_data;
    switch (d->state) {
        case _CBSP_GETTING_TYPE: {
            uint8_t fb = s->cbor.buffer[0];
            uint8_t mjt = fb >> 5;
            uint8_t addi = fb & 0x1F;
            
            int bytes = -1;
            if      (addi <= 23) bytes=1;
            else if (addi == 24) bytes=2;
            else if (addi == 25) bytes=3;
            else if (addi == 26) bytes=5;
            else if (addi == 27) bytes=9;
            
#ifdef DEBUG
            fprintf(stderr, "%d %d (%02X)\n", mjt, addi, fb);
#endif
            switch (mjt) {
                case 0: // unsigned int
                case 1: // signed int
                case 6: // tagging
                case 7: // misc
                {
                    COPY(cbor, misc, bytes);
                    return CBSP_OK;
                }
                    break;
                case 2: // byte string
                case 3: // text string
                {
                    if ( addi == 31 ) { // stream
                        // just copy this byte and continue, interpreting
                        // chunks as like nothing special happened
                        bytes = 1;
                        COPY(cbor, misc, bytes);
                        return CBSP_OK;
                    } else {
                        const uint8_t* buf = s->cbor.buffer;
                        COPY(cbor, misc, bytes);
                        d->x = read_num(addi, buf+1);
#ifdef DEBUG
                        fprintf(stderr, "l=%llu\n", d->x);
#endif
                        d->state = _CBSP_COPY_X_BYTES_TO_TEXT_THEN_RETURN;
                        return CBSP_OK;
                    }
                } break;
                case 4: // array
                case 5: // map
                {
                    if ( addi == 31 ) { // stream
                        bytes = 1;
                        COPY(cbor, misc, bytes);
                        return CBSP_OK;
                    } else {
                        COPY(cbor, misc, bytes);
                        return CBSP_OK;
                    }
                }; break;
                default: return CBSP_DATAERR;
            };
        }break;
        case _CBSP_COPY_X_BYTES_TO_TEXT_THEN_RETURN: {
            size_t bytes;
            
            bytes = d->x;
            if (bytes > s->cbor.len)      bytes = s->cbor.len;
            if (bytes > s->text.capacity) bytes = s->text.capacity;
            
#ifdef DEBUG
            fprintf(stderr, "Copying data: ");
            fwrite(s->cbor.buffer, 1, bytes, stderr);
            fprintf(stderr, "\n");
#endif
            
            COPY(cbor, text, bytes);
            d->x -= bytes;
            
#ifdef DEBUG
            fprintf(stderr, "Remaining %llu bytes\n", d->x);
#endif
            
            if (d->x == 0) {
                d->state = _CBSP_GETTING_TYPE;
            }
            return CBSP_OK;
        } break;
        default: return CBSP_INVAL;
    }
}







enum cbsp_step_result cbsp_merger_step(struct cbsp_merger* s)
{
    if (s->cbor.capacity == 0) return CBSP_MOREDATA;
    if (s->cbor.buffer == NULL) return CBSP_INVAL;
    if (s->misc.buffer == NULL) return CBSP_INVAL;
    if (s->text.buffer == NULL) return CBSP_INVAL;
    
    struct _cbsp_data* d = &s->_data;
    switch (d->state) {
        case _CBSP_GETTING_TYPE: {
            if (s->misc.len == 0) return CBSP_MOREDATA;
            uint8_t fb = s->misc.buffer[0];
            uint8_t mjt = fb >> 5;
            uint8_t addi = fb & 0x1F;
            
            int bytes = -1;
            if      (addi <= 23) bytes=1;
            else if (addi == 24) bytes=2;
            else if (addi == 25) bytes=3;
            else if (addi == 26) bytes=5;
            else if (addi == 27) bytes=9;
            
#ifdef DEBUG
            fprintf(stderr, "%d %d (%02X)\n", mjt, addi, fb);
#endif
            switch (mjt) {
                case 0: // unsigned int
                case 1: // signed int
                case 6: // tagging
                case 7: // misc
                {
                    COPY(misc, cbor, bytes);
                    return CBSP_OK;
                }
                    break;
                case 2: // byte string
                case 3: // text string
                {
                    if ( addi == 31 ) { // stream
                        // just copy this byte and continue, interpreting
                        // chunks as like nothing special happened
                        bytes = 1;
                        COPY(misc, cbor, bytes);
                        return CBSP_OK;
                    } else {
                        const uint8_t* buf = s->misc.buffer;
                        COPY(misc, cbor, bytes);
                        d->x = read_num(addi, buf+1);
#ifdef DEBUG
                        fprintf(stderr, "l=%llu\n", d->x);
#endif
                        d->state = _CBSP_COPY_X_BYTES_TO_TEXT_THEN_RETURN;
                        return CBSP_OK;
                    }
                } break;
                case 4: // array
                case 5: // map
                {
                    if ( addi == 31 ) { // stream
                        bytes = 1;
                        COPY(misc, cbor, bytes);
                        return CBSP_OK;
                    } else {
                        COPY(misc, cbor, bytes);
                        return CBSP_OK;
                    }
                }; break;
                default: return CBSP_DATAERR;
            };
        }break;
        case _CBSP_COPY_X_BYTES_TO_TEXT_THEN_RETURN: {
            if (s->text.len == 0) return CBSP_MOREDATA_TEXT;
            size_t bytes;
            
            bytes = d->x;
            if (bytes > s->text.len)      bytes = s->text.len;
            if (bytes > s->cbor.capacity) bytes = s->cbor.capacity;
            
#ifdef DEBUG
            fprintf(stderr, "Copying data: ");
            fwrite(s->text.buffer, 1, bytes, stderr);
            fprintf(stderr, "\n");
#endif
            
            COPY(text, cbor, bytes);
            d->x -= bytes;
            
#ifdef DEBUG
            fprintf(stderr, "Remaining %llu bytes\n", d->x);
#endif
            
            if (d->x == 0) {
                d->state = _CBSP_GETTING_TYPE;
            }
            return CBSP_OK;
        } break;
        default: return CBSP_INVAL;
    }
}
