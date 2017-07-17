#pragma once

#include <stddef.h>
#include <stdint.h>

struct cbsp_input_stream {
    const uint8_t* buffer; // advances as bytes are consumed
    size_t len; // decreses as bytes are consumed
};

struct cbsp_output_stream {
    uint8_t* buffer; // advances as bytes are produced
    size_t capacity; // decreases as bytes are produced
};

// Implementation details
enum _cbsp_state {
    _CBSP_GETTING_TYPE = 0,
    _CBSP_COPY_X_BYTES_TO_TEXT_THEN_RETURN = 1
};

// Implementation details
struct _cbsp_data {
    enum _cbsp_state state;
    uint64_t x;
};

/* Splits CBOR stream into misc and text parts for better compressibility */
/* Can also be used for JSON, as it is convertible to/from CBOR */
/* The structure should be initialized with memset('\0') */
struct cbsp_splitter {
    // public:
    struct cbsp_input_stream cbor;
    struct cbsp_output_stream misc;
    struct cbsp_output_stream text;
    // private:
    struct _cbsp_data _data;
};

enum cbsp_step_result {
    CBSP_INVAL, // NULL pointers encoutered
    CBSP_DATAERR, // error in data
    CBSP_OK, // go on
    CBSP_MOREDATA // need more data for a step
};

enum cbsp_step_result cbsp_splitter_step(struct cbsp_splitter* s);
