#include "hazel/reader.h"

#include <stdlib.h>
#include <string.h>

int hazel_message_reader_init(hazel_message_reader* reader, uint8_t* data, 
                               size_t size, size_t offset)
{
    if (data == NULL || offset > size)
    {
        return HAZEL_ERR_READER_INVALID_ARGUMENTS;
    }
    
    reader->data = data;

    reader->size = size;
    reader->offset = offset;
    reader->read_head = offset;

    reader->tag = 0x00;
    reader->_position = 0x00;
    
    return 0;
}

void hazel_message_reader_free(hazel_message_reader* reader)
{
    free(reader->data);
}

size_t hazel_message_reader_get_position(hazel_message_reader* reader)
{
    return reader->_position;
}
void hazel_message_reader_set_position(hazel_message_reader* reader, size_t value)
{
    reader->_position = value;
    reader->read_head = value + reader->offset;
}

size_t hazel_message_reader_remaining(hazel_message_reader* reader)
{
    return reader->size - reader->_position;
}
bool hazel_message_reader_has_remaining(hazel_message_reader* reader,
                                        size_t amount)
{
    return hazel_message_reader_remaining(reader) >= amount;
} 

#define BUFFER_CHECK(reader, wanted_size)                         \
    if (!hazel_message_reader_has_remaining(reader, wanted_size)) \
    {                                                             \
        return HAZEL_READER_CAPACITY_EXCEEDED;                    \
    }

uint8_t hazel_message_reader_byte(hazel_message_reader* reader)
{
    reader->_position++;
    size_t pos = reader->read_head++;
    return reader->data[pos];
}

int hazel_message_reader_bool(hazel_message_reader* reader, bool* result)
{
    uint8_t tmp;
    int ret = hazel_message_reader_uint8(reader, &tmp);
    *result = tmp != 0;
    return ret;
}

int hazel_message_reader_uint8(hazel_message_reader* reader, uint8_t* result)
{
    BUFFER_CHECK(reader, sizeof(uint8_t));
    *result = hazel_message_reader_byte(reader);
    return 0;
}

int hazel_message_reader_uint16(hazel_message_reader* reader, uint16_t *result)
{
    BUFFER_CHECK(reader, sizeof(uint16_t));
    *result = hazel_message_reader_byte(reader) 
        | (hazel_message_reader_byte(reader) << 8);
    return 0;
}
int hazel_message_reader_int16(hazel_message_reader* reader, int16_t *result)
{
    return hazel_message_reader_uint16(reader, (uint16_t*) result);
}

int hazel_message_reader_uint32(hazel_message_reader* reader, uint32_t *result)
{
    BUFFER_CHECK(reader, sizeof(uint32_t));
    *result = hazel_message_reader_byte(reader)
        | (hazel_message_reader_byte(reader) << 8)
        | (hazel_message_reader_byte(reader) << 16)
        | (hazel_message_reader_byte(reader) << 24);
    return 0;
}
int hazel_message_reader_int32(hazel_message_reader* reader, int32_t *result)
{
    return hazel_message_reader_uint32(reader, (uint32_t*) result);
}

int hazel_message_reader_uint64(hazel_message_reader* reader, uint64_t *result)
{
    BUFFER_CHECK(reader, sizeof(uint64_t));
    *result = hazel_message_reader_byte(reader)
        | ((uint64_t)hazel_message_reader_byte(reader) << 8)
        | ((uint64_t)hazel_message_reader_byte(reader) << 16)
        | ((uint64_t)hazel_message_reader_byte(reader) << 24)
        | ((uint64_t)hazel_message_reader_byte(reader) << 32)
        | ((uint64_t)hazel_message_reader_byte(reader) << 40)
        | ((uint64_t)hazel_message_reader_byte(reader) << 48)
        | ((uint64_t)hazel_message_reader_byte(reader) << 56);
    return 0;
}
int hazel_message_reader_int64(hazel_message_reader* reader, int64_t *result)
{
    return hazel_message_reader_uint64(reader, (uint64_t*) result);
}

int hazel_message_reader_packed_uint32(hazel_message_reader* reader, uint32_t* result)
{
    int ret = 0;
    bool readMore = true;
    int32_t shift = 0;
    uint32_t output = 0;
    while (readMore) 
    {
        uint8_t b;
        if ((ret = hazel_message_reader_uint8(reader, &b)) != 0)
        {
            return ret;
        }
        if (b >= 0x80)
        {
            readMore = true;
            b ^= 0x80;
        }
        else
        {
            readMore = false;
        }
        output |= (uint32_t)(b << shift);
        shift += 7;
    }

    *result = output;

    return 0;
}
int hazel_message_reader_packed_int32(hazel_message_reader* reader, int32_t* result)
{
    return hazel_message_reader_packed_uint32(reader, (uint32_t*) result);
}

int hazel_message_reader_single(hazel_message_reader* reader, float* result)
{
    BUFFER_CHECK(reader, sizeof(float));
    memcpy(result, &reader->data[reader->read_head], sizeof(float));
    return 0;
}

int hazel_message_reader_string(hazel_message_reader* reader, 
                                char* output, size_t output_size)
{
    int32_t read_length;
    int ret = hazel_message_reader_packed_int32(reader, &read_length);
    if (ret != 0)
    {
        return ret;
    }

    if ((size_t)read_length + 1 > output_size)
    {
        return HAZEL_READER_INSUFFICIENT_BUFFER;
    }

    if (!hazel_message_reader_has_remaining(reader, read_length))
    {
        return HAZEL_READER_MALFORMED_INPUT;
    }

    memcpy(output, reader->data + reader->read_head, read_length);
    output[read_length] = 0x00;
    
    hazel_message_reader_set_position(
        reader,
        hazel_message_reader_get_position(reader) + read_length);

    return 0;
}

int hazel_message_reader_string_malloc(hazel_message_reader* reader,
                                       char** output)
{
    int32_t read_length;
    int ret = hazel_message_reader_packed_int32(reader, &read_length);
    if (ret != 0)
    {
        return ret;
    }

    char* str_buffer = malloc((read_length + 1) * sizeof(char));
    if (str_buffer == NULL)
    {
        return HAZEL_ERR_FAILED_ALLOC;
    }

    if (!hazel_message_reader_has_remaining(reader, read_length))
    {
        return HAZEL_READER_MALFORMED_INPUT;
    }

    memcpy(str_buffer, reader->data + reader->read_head, read_length);
    str_buffer[read_length] = 0x00;

    *output = str_buffer;

    hazel_message_reader_set_position(
        reader,
        hazel_message_reader_get_position(reader) + read_length);
    
    return 0;
}

int hazel_message_reader_string_view(
    hazel_message_reader* reader,
    hazel_message_reader_string_buffer_view* result)
{
    int ret;
    int32_t read_length;
    if ((ret = hazel_message_reader_packed_int32(reader, &read_length) != 0))
    {
        return ret;
    }

    if (!hazel_message_reader_has_remaining(reader, read_length))
    {
        return HAZEL_READER_MALFORMED_INPUT;
    }

    char* buffer_view = (char*)(reader->data + reader->read_head);

    result->length = read_length;
    result->buffer = buffer_view;

    hazel_message_reader_set_position(
        reader,
        hazel_message_reader_get_position(reader) + read_length);

    return 0;
}

bool hazel_message_reader_can_read_message(hazel_message_reader* reader)
{
    return hazel_message_reader_has_remaining(reader, 3);
}

int hazel_message_reader_read_message(hazel_message_reader* reader, hazel_message_reader* output)
{
    hazel_message_reader temp = {
        .data = reader->data,
        .size = reader->size,
        .offset = reader->read_head
    };

    int ret = 0;

    hazel_message_reader_set_position(&temp, 0);

    uint16_t size;
    uint8_t tag;
    ret |= hazel_message_reader_uint16(&temp, &size);
    ret |= hazel_message_reader_uint8(&temp, &tag);
    
    if (ret != 0)
    {
        return ret;
    }

    temp.size = size;
    temp.tag = tag;

    temp.offset += 3;
    hazel_message_reader_set_position(&temp, 0);

    if (!hazel_message_reader_has_remaining(reader, temp.size + 3))
    {
        return HAZEL_READER_CAPACITY_EXCEEDED;
    }
    hazel_message_reader_set_position(
        reader,
        hazel_message_reader_get_position(reader) + temp.size + 3);

    *output = temp;

    return ret;
}
