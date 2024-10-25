#pragma once

#include "hazel/common.h"
#include "hazel/errors.h"
#include "hazel/send_option.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** \defgroup Writer Message Writer
 * @{
 */

#define HAZEL_WRITER_CAPACITY_EXCEEDED -0xF001

typedef struct hazel_message_writer_message_start
    hazel_message_writer_message_start;

typedef struct hazel_message_writer
{
    uint8_t *data;

    size_t size;
    size_t position;

    enum hazel_send_option send_option;

    hazel_message_writer_message_start *_start_head;

    size_t _buffer_size;
} hazel_message_writer;

/**
 * Create the writer structure from an existing allocated buffer in memory.
 */
int hazel_message_writer_init(hazel_message_writer *writer, uint8_t *data,
                              size_t size);

/**
 * Use the allocator to allocate a buffer of \param size bytes and init the
 * writer structure from this buffer.
 */
int hazel_message_writer_init_malloc(hazel_message_writer *writer, size_t size);

/**
 * Free the internal buffer. Use in correlation with 
 * hazel_message_writer_init_malloc
 **/
int hazel_message_writer_free(hazel_message_writer *writer);

int hazel_message_writer_to_bytes(hazel_message_writer *writer, uint8_t *buffer,
                                  size_t buffer_size, bool include_header,
                                  size_t *out_size);

size_t hazel_message_writer_remaining(hazel_message_writer *writer);

int hazel_message_writer_start_message(hazel_message_writer *writer,
                                       uint8_t tag);
int hazel_message_writer_end_message(hazel_message_writer *writer);
int hazel_message_writer_cancel_message(hazel_message_writer *writer);

int hazel_message_writer_clear(hazel_message_writer *writer,
                               enum hazel_send_option send_option);

int hazel_message_writer_bool(hazel_message_writer *writer, bool value);
int hazel_message_writer_uint8(hazel_message_writer *writer, uint8_t value);
int hazel_message_writer_uint16(hazel_message_writer *writer, uint16_t value);
int hazel_message_writer_int16(hazel_message_writer *writer, int16_t value);
int hazel_message_writer_uint32(hazel_message_writer *writer, uint32_t value);
int hazel_message_writer_int32(hazel_message_writer *writer, int32_t value);
int hazel_message_writer_uint64(hazel_message_writer *writer, uint64_t value);
int hazel_message_writer_int64(hazel_message_writer *writer, int64_t value);

int hazel_message_writer_packed_uint32(hazel_message_writer *writer,
                                       uint32_t value);
int hazel_message_writer_packed_int32(hazel_message_writer *writer,
                                      int32_t value);

int hazel_message_writer_single(hazel_message_writer *writer, float value);

int hazel_message_writer_string(hazel_message_writer *writer,
                                const char *string);

int hazel_message_writer_bytes(hazel_message_writer *writer,
                               const uint8_t *buffer, size_t offset,
                               size_t length);
int hazel_message_writer_bytes_and_size(hazel_message_writer *writer,
                                        const uint8_t *buffer, size_t offset, 
                                        size_t length);

/** @}*/
