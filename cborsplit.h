#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef CBOR_SPLITTER_MAXDEPTH
    #define CBOR_SPLITTER_MAXDEPTH 64
#endif


struct cbor_splitter_input_stream {
    const uint8_t* buffer; // advances as bytes are consumed
    size_t len; // decreses as bytes are consumed
};

struct cbor_splitter_output_stream {
    uint8_t* buffer; // advances as bytes are produced
    size_t capacity; // decreases as bytes are produced
};

enum cbor_splitter_state {
    CBSP_GETTING_TYPE = 0,
    CBSP_COPY_X_BYTES_TO_DATA_THEN_RETURN = 1
};

// Implementation details
struct cbor_splitter_data {
    enum cbor_splitter_state state;
    uint64_t x;
};

/* Splits CBOR stream into boilerplace and data parts for better compressibility */
/* Can also be used for JSON, as it is convertible to/from CBOR */
/* The structure should be initialized with memset('\0') */
struct cbor_splitter {
    // public:
    struct cbor_splitter_input_stream cbor;
    struct cbor_splitter_output_stream control;
    struct cbor_splitter_output_stream data;
    // private:
    struct cbor_splitter_data _data[CBOR_SPLITTER_MAXDEPTH];
    int _current_level;
};

// zero ret => no error
int cbor_splitter_step(struct cbor_splitter* s);
