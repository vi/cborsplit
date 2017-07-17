#include <string.h>

#include "cborsplit.h"

/*struct cbor_splitter_input_stream {
    const char* buffer; // advances as bytes are consumed
    size_t len; // decreses as bytes are consumed
};

struct cbor_splitter_output_stream {
    char* buffer; // advances as bytes are produced
    size_t capacity; // decreases as bytes are produced
};

enum cbor_splitter_state {
    CBSP_GETTING_TYPE = 0
};

// Implementation details
struct cbor_splitter_data {
    enum cbor_splitter_state state;
};

struct cbor_splitter {
    // public:
    struct cbor_splitter_input_stream cbor;
    struct cbor_splitter_output_stream control;
    struct cbor_splitter_output_stream data;
    // private:
    struct cbor_splitter_data _data[CBOR_SPLITTER_MAXDEPTH];
    int _current_level;
};
*/

int cbor_splitter_step(struct cbor_splitter* s)
{
    if (s->cbor.len == 0) return 0;
    if (s->cbor.buffer == NULL) return 1;
    if (s->control.capacity == 0) return 0;
    if (s->control.buffer == NULL) return 1;
    if (s->data.capacity == 0) return 0;
    if (s->data.buffer == NULL) return 1;
    
    #define COPY(stream, bytes) \
                    if (s->stream.capacity < bytes) { \
                        s->stream.capacity = 0; \
                        return 0; \
                    } \
                    memcpy(s->stream.buffer, s->cbor.buffer, bytes); \
                    s->stream.buffer   += bytes; \
                    s->stream.capacity -= bytes; \
                    s->cbor.buffer     += bytes; \
                    s->cbor.len        -= bytes;
    
    struct cbor_splitter_data* d = &s->_data[s->_current_level];
    switch (d->state) {
        case         CBSP_GETTING_TYPE: {
            unsigned char fb = s->cbor.buffer[0];
            unsigned char mjt = fb >> 5;
            unsigned char addi = fb & 0x1F;
            switch (mjt) {
                case 0: // unsigned int
                case 1: // signed int
                {
                    int bytes;
                    if      (addi <= 23) bytes=1;
                    else if (addi == 24) bytes=2;
                    else if (addi == 25) bytes=3;
                    else if (addi == 26) bytes=5;
                    else if (addi == 27) bytes=9;
                    else return 2;
                    
                    COPY(control, bytes);
                    
                    return 0;
                }
                    break;
                case 2: // byte string
                case 3: // text string
                case 4: // array
                case 5: // map
                case 6: // tagging
                case 7: // misc
                {}
            };
        }break; case CBSP_COPY_X_BYTES_TO_DATA_THEN_RETURN: {
            size_t bytes;
            
            bytes = d->x;
            if (bytes > s->cbor.len)      bytes = s->cbor.len;
            if (bytes > s->data.capacity) bytes = s->data.capacity;
            COPY(data, bytes);
            
            d->state = CBSP_GETTING_TYPE;
        }
    }
    
    return 0;
}
