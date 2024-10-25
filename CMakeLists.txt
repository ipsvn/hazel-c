cmake_minimum_required(VERSION 3.13)
project(hazelnetworking C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
include(GNUInstallDirs)

find_package(Threads REQUIRED)

add_subdirectory(hazel)

add_executable(tmptest
    tmptest/main.c
)
target_include_directories(tmptest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tmptest
    INTERFACE
        ${PROJECT_SOURCE_DIR}/hazel
)
target_link_libraries(tmptest
    PRIVATE
        hazelnetworking
)


#find_package(mbedTLS REQUIRED) # todo, dTLS


# Install headers
install(DIRECTORY hazel/include/hazel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the library
include(CMakePackageConfigHelpers)

install(
    TARGETS hazelnetworking
    EXPORT hazelnetworkingTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT hazelnetworkingTargets
    NAMESPACE hazelnetworking::
    DESTINATION lib/cmake/hazelnetworking
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/hazelnetworkingConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hazelnetworkingConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hazelnetworking
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/hazelnetworkingConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hazelnetworking
)