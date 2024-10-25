#pragma once

#include "hazel/common.h"
#include "hazel/errors.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** \defgroup Reader Message Reader
 * @{
 */

#define HAZEL_ERR_READER_INVALID_ARGUMENTS -0xE001
#define HAZEL_READER_CAPACITY_EXCEEDED -0xE002
#define HAZEL_READER_MALFORMED_INPUT -0xE003
#define HAZEL_READER_INSUFFICIENT_BUFFER -0xE004

typedef struct hazel_message_reader
{
    uint8_t *data;

    size_t size;
    size_t offset;
    size_t read_head;

    uint8_t tag;

    size_t _position;
} hazel_message_reader;

/**
 * Initialise the message_reader with the provided data buffer.
 */
int hazel_message_reader_init(hazel_message_reader* reader, uint8_t* data, 
                               size_t size, size_t offset);

/**
 * Free the message_reader. This will also consequently free the data pointer.
 */
void hazel_message_reader_free(hazel_message_reader* reader);

size_t hazel_message_reader_get_position(hazel_message_reader* reader);
void hazel_message_reader_set_position(hazel_message_reader* reader, size_t value);

/**
 * \brief Get the number of bytes remaining to be read.
 * 
 * This function returns the number of bytes that are left which are able to be
 * read from the message_reader.
 *
 * \param reader A pointer to the hazel_message_reader structure.
 * \return The number of bytes remaining which can be read.
 */
size_t hazel_message_reader_remaining(hazel_message_reader* reader);

/**
 * \brief Check whether there are enough bytes left to read n amount.
 * 
 * This function checks if the hazel_message_reader has at least the specified
 * number of bytes remaining to be read.
 *
 * \param reader A pointer to the hazel_message_reader structure.
 * \param amount The number of bytes to check for availability.
 * \return true if there are at least 'amount' bytes remaining, false otherwise.
 */
bool hazel_message_reader_has_remaining(hazel_message_reader* reader, size_t amount);

int hazel_message_reader_bool  (hazel_message_reader* reader, bool* result);
int hazel_message_reader_uint8 (hazel_message_reader* reader, uint8_t *result);
int hazel_message_reader_uint16(hazel_message_reader* reader, uint16_t *result);
int hazel_message_reader_int16 (hazel_message_reader* reader, int16_t *result);
int hazel_message_reader_uint32(hazel_message_reader* reader, uint32_t *result);
int hazel_message_reader_int32 (hazel_message_reader* reader, int32_t *result);
int hazel_message_reader_uint64(hazel_message_reader* reader, uint64_t *result);
int hazel_message_reader_int64 (hazel_message_reader* reader, int64_t *result);

int hazel_message_reader_packed_uint32(hazel_message_reader* reader, uint32_t* result);
int hazel_message_reader_packed_int32(hazel_message_reader* reader, int32_t* result);

int hazel_message_reader_single(hazel_message_reader* reader, float* result);

int hazel_message_reader_string(hazel_message_reader* reader, 
                                char* output, size_t output_size);

/**
 * Allocates memory into output and copies a string into it. You should free the
 * memory manually yourself after use.
 * \brief Read a string into a new null-terminated buffer.
*/
int hazel_message_reader_string_malloc(hazel_message_reader* reader,
                                       char** output);

typedef struct hazel_message_reader_string_buffer_view {
    int32_t length;
    const char* buffer;
} hazel_message_reader_string_buffer_view;

/**
 * \brief Reads a string, returning a view into the buffer.
 *
 * This function returns a view of a string within the buffer managed by the 
 * reader. Rather than returning a null-terminated string, it returns a struct
 * containing the length and a pointer inside the buffer.
 *
 * \return A hazel_message_reader_string_buffer_view struct containing the 
 *         string length and a pointer to the start of the string within the 
 *         buffer.
 */
int hazel_message_reader_string_view(
    hazel_message_reader* reader,
    hazel_message_reader_string_buffer_view* result);

bool hazel_message_reader_can_read_message(hazel_message_reader* reader);
int hazel_message_reader_read_message(hazel_message_reader* reader, hazel_message_reader* output);

/** @}*/