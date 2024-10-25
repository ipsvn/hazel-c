#include "hazel/writer.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

typedef struct hazel_message_writer_message_start
{
    int position;
    struct hazel_message_writer_message_start *next;
} hazel_message_writer_message_start;
int hazel_message_writer_message_start_push(
    hazel_message_writer_message_start **head, int position)
{
    hazel_message_writer_message_start *node =
        malloc(sizeof(hazel_message_writer_message_start));

    if (node == NULL)
    {
        return -1;
    }

    node->position = position;
    node->next = *head;
    *head = node;

    return 0;
}
int hazel_message_writer_message_start_pop(
    hazel_message_writer_message_start **head, int *position)
{
    hazel_message_writer_message_start *node = *head;
    if (node == NULL)
    {
        return -1;
    }

    *position = node->position;
    *head = node->next;
    free(node);

    return 0;
}
int hazel_message_writer_message_start_clear(
    hazel_message_writer_message_start **head)
{
    while (*head != NULL)
    {
        hazel_message_writer_message_start *next = (*head)->next;
        free(*head);
        *head = next;
    }
    return 0;
}

int hazel_message_writer_init(hazel_message_writer *writer, uint8_t *data,
                              size_t size)
{
    if (data == NULL)
    {
        // TODO proper return codes
        return HAZEL_ERR_INVALID_ARGUMENTS;
    }

    writer->data = data;
    writer->_buffer_size = size;
    writer->size = 0;
    writer->position = 0;
    writer->_start_head = NULL;

    return 0;
}

int hazel_message_writer_init_malloc(hazel_message_writer *writer, size_t size)
{
    uint8_t *data = malloc(size);
    if (data == NULL)
    {
        return HAZEL_ERR_FAILED_ALLOC;
    }
    return hazel_message_writer_init(writer, data, size);
}

int hazel_message_writer_free(hazel_message_writer *writer)
{
    free(writer->data);
    return 0;
}

int _hazel_message_writer_to_bytes_offsets(hazel_message_writer *writer,
                                           bool include_header, uint8_t *out)
{
    if (include_header)
    {
        *out = 0;
        return 0;
    }
    switch (writer->send_option)
    {
    case HAZEL_SEND_OPTION_RELIABLE:
        *out = 3;
        return 0;
    case HAZEL_SEND_OPTION_UNRELIABLE:
        *out = 1;
        return 0;
        break;
    default:
        return -2;
    }
}

int hazel_message_writer_to_bytes(hazel_message_writer *writer, uint8_t *buffer,
                                  size_t buffer_size, bool include_header,
                                  size_t *out_size)
{
    uint8_t offsets = 0;
    int result = _hazel_message_writer_to_bytes_offsets(writer,
                                                        include_header,
                                                        &offsets);

    if (result != 0)
    {
        return result;
    }

    size_t sz = writer->size - offsets;

    if (out_size != NULL)
    {
        *out_size = sz;
    }

    if (buffer_size < sz)
    {
        // TODO proper return codes
        return -1;
    }

    memcpy(buffer, writer->data + offsets, sz);
    return 0;
}

size_t hazel_message_writer_remaining(hazel_message_writer *writer)
{
    return writer->_buffer_size - writer->size;
}

int hazel_message_writer_start_message(hazel_message_writer *writer,
                                       uint8_t tag)
{
    int ret;
    int start = writer->position;
    
    ret = hazel_message_writer_message_start_push(&writer->_start_head, start);
    if (ret != 0)
    {
        return ret;
    }

    writer->data[start] = 0;
    writer->data[start + 1] = 0;
    writer->position += 2;
    
    ret = hazel_message_writer_uint8(writer, tag);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}
int hazel_message_writer_end_message(hazel_message_writer *writer)
{
    int ret = 0;
    int last_message_start = 0;

    ret = hazel_message_writer_message_start_pop(
        &writer->_start_head, &last_message_start);
    if (ret != 0)
    {
        return ret;
    }

    uint16_t length = (uint16_t)(writer->position - last_message_start - 3);
    writer->data[last_message_start] = (uint8_t)(length);
    writer->data[last_message_start + 1] = (uint8_t)(length >> 8);
    return 0;
}
int hazel_message_writer_cancel_message(hazel_message_writer *writer)
{
    int ret = 0;
    int position = 0;

    if ((ret = hazel_message_writer_message_start_pop(&writer->_start_head,
                                                      &position)) != 0)
    {
        return ret;
    }

    writer->position = position;
    writer->size = position;

    return ret;
}

int hazel_message_writer_clear(hazel_message_writer *writer,
                                   enum hazel_send_option send_option)
{
    hazel_message_writer_message_start_clear(&writer->_start_head);

    writer->send_option = send_option;
    writer->data[0] = (uint8_t)send_option;
    switch (send_option)
    {
    default:
    case HAZEL_SEND_OPTION_UNRELIABLE:
        writer->size = writer->position = 1;
        break;
    case HAZEL_SEND_OPTION_RELIABLE:
        writer->size = writer->position = 3;
        break;
    }
    return 0;
}

#define BUFFER_CHECK(writer, wanted_size)                       \
    if (writer->_buffer_size < writer->position + wanted_size)  \
    {                                                           \
        return HAZEL_WRITER_CAPACITY_EXCEEDED;                  \
    }

#define LENGTH_CHECK(writer)             \
    if (writer->position > writer->size) \
    writer->size = writer->position

int hazel_message_writer_bool(hazel_message_writer *writer, bool value)
{
    return hazel_message_writer_uint8(writer, value);
}
int hazel_message_writer_uint8(hazel_message_writer *writer, uint8_t value)
{
    BUFFER_CHECK(writer, sizeof(uint8_t));
    writer->data[writer->position++] = value;
    LENGTH_CHECK(writer);
    return 0;
}
int hazel_message_writer_uint16(hazel_message_writer *writer, uint16_t value)
{
    BUFFER_CHECK(writer, sizeof(uint16_t));
    writer->data[writer->position++] = value;
    writer->data[writer->position++] = value >> 8;
    LENGTH_CHECK(writer);
    return 0;
}
int hazel_message_writer_int16(hazel_message_writer *writer, int16_t value)
{
    BUFFER_CHECK(writer, sizeof(int16_t));
    writer->data[writer->position++] = value;
    writer->data[writer->position++] = value >> 8;
    LENGTH_CHECK(writer);
    return 0;
}
int hazel_message_writer_uint32(hazel_message_writer *writer, uint32_t value)
{
    BUFFER_CHECK(writer, sizeof(uint32_t));
    writer->data[writer->position++] = value;
    writer->data[writer->position++] = value >> 8;
    writer->data[writer->position++] = value >> 16;
    writer->data[writer->position++] = value >> 24;
    LENGTH_CHECK(writer);
    return 0;
}
int hazel_message_writer_int32(hazel_message_writer *writer, int32_t value)
{
    BUFFER_CHECK(writer, sizeof(int32_t));
    writer->data[writer->position++] = value;
    writer->data[writer->position++] = value >> 8;
    writer->data[writer->position++] = value >> 16;
    writer->data[writer->position++] = value >> 24;
    LENGTH_CHECK(writer);
    return 0;
}
int hazel_message_writer_uint64(hazel_message_writer *writer, uint64_t value)
{
    BUFFER_CHECK(writer, sizeof(uint64_t));
    writer->data[writer->position++] = value;
    writer->data[writer->position++] = value >> 8;
    writer->data[writer->position++] = value >> 16;
    writer->data[writer->position++] = value >> 24;
    writer->data[writer->position++] = value >> 32;
    writer->data[writer->position++] = value >> 40;
    writer->data[writer->position++] = value >> 48;
    writer->data[writer->position++] = value >> 56;
    LENGTH_CHECK(writer);
    return 0;
}
int hazel_message_writer_int64(hazel_message_writer *writer, int64_t value)
{
    BUFFER_CHECK(writer, sizeof(int64_t));
    writer->data[writer->position++] = value;
    writer->data[writer->position++] = value >> 8;
    writer->data[writer->position++] = value >> 16;
    writer->data[writer->position++] = value >> 24;
    writer->data[writer->position++] = value >> 32;
    writer->data[writer->position++] = value >> 40;
    writer->data[writer->position++] = value >> 48;
    writer->data[writer->position++] = value >> 56;
    LENGTH_CHECK(writer);
    return 0;
}

int hazel_message_writer_packed_uint32(hazel_message_writer *writer,
                                       uint32_t value)
{
    do
    {
        uint8_t b = (uint8_t)(value & 0xFF);
        if (value >= 0x80)
            b |= 0x80;

        int ret = hazel_message_writer_uint8(writer, b);
        if (ret != 0)
        {
            return ret;
        }

        value >>= 7;
    } while (value > 0);

    return 0;
}
int hazel_message_writer_packed_int32(hazel_message_writer *writer,
                                      int32_t value)
{
    return hazel_message_writer_packed_uint32(writer, (uint32_t)value);
}

int hazel_message_writer_single(hazel_message_writer *writer, float value)
{
    BUFFER_CHECK(writer, sizeof(float));
    memcpy(&writer->data[writer->position], &value, sizeof(float));
    writer->position += sizeof(float);
    LENGTH_CHECK(writer);
    return 0;
}

int hazel_message_writer_string(hazel_message_writer *writer, 
                                const char *string)
{
    size_t length = strlen(string);
    return hazel_message_writer_bytes_and_size(
        writer, (uint8_t *)string, 0, length);
}

int hazel_message_writer_bytes(hazel_message_writer *writer,
                               const uint8_t *buffer, size_t offset,
                               size_t length)
{
    BUFFER_CHECK(writer, length);
    memcpy(&writer->data[writer->position], &buffer[offset], length);
    writer->position += length;
    LENGTH_CHECK(writer);
    return 0;
}
int hazel_message_writer_bytes_and_size(hazel_message_writer *writer,
                                        const uint8_t *buffer, size_t offset, 
                                        size_t length)
{
    int ret = hazel_message_writer_packed_uint32(writer, (uint32_t)length);
    if (ret != 0)
    {
        return ret;
    }

    return hazel_message_writer_bytes(writer, buffer, offset, length);
}
