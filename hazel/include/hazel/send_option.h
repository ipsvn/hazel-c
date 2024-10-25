#pragma once

#include "hazel/common.h"

enum hazel_send_option
{
    HAZEL_SEND_OPTION_UNRELIABLE = 0,
    HAZEL_SEND_OPTION_RELIABLE = 1,

    // UDP options
    HAZEL_SEND_OPTION_HELLO = 8,
    HAZEL_SEND_OPTION_PING = 12,
    HAZEL_SEND_OPTION_DISCONNECT = 9,
    HAZEL_SEND_OPTION_ACK = 10
};
