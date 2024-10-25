add_library(hazelnetworking STATIC
    src/reader.c
    src/writer.c
    src/udp/client.c
    src/udp/connection.c
    src/udp/socket.c
)

target_include_directories(hazelnetworking
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(WIN32)
  target_link_libraries(hazelnetworking PUBLIC ws2_32)
endif()

if(MSVC)
  target_compile_options(hazelnetworking PRIVATE /W4)
else()
  target_compile_options(hazelnetworking PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(hazelnetworking PUBLIC
  HAZEL_LOGGING_ENABLE
  HAZEL_LOGGING_DEBUG
)