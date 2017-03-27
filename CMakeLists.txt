cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

find_package(IRODS 4.2.0 EXACT REQUIRED)
include(${IRODS_TARGETS_PATH})

set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)
add_definitions(-D_FILE_OFFSET_BITS=64)

project(pinstripe C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(FUSE REQUIRED)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH ${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib)

add_executable(
	pinstripe
	${CMAKE_SOURCE_DIR}/src/pinstripe.cc
)

target_link_libraries(
  pinstripe
  PRIVATE
  fuse
  irods_client
  irods_common
  irods_plugin_dependencies
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
  )
target_include_directories(
  pinstripe
  PRIVATE
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_SOURCE_DIR}/include
  ${IRODS_INCLUDE_DIRS}
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
  ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
  ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
  )
