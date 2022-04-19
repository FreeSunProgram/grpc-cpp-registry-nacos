#
# Created by sx on 2022/4/5.
#

if(gRPC_NACOS_PROVIDER STREQUAL "module")
  if(NOT NACOS_ROOT_DIR)
    set(NACOS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nacos-sdk-cpp)
  endif()
  if(EXISTS "${NACOS_ROOT_DIR}/CMakeLists.txt")
    include_directories("${NACOS_ROOT_DIR}/include")
    add_subdirectory(${NACOS_ROOT_DIR} third_party/nacos-sdk-cpp)
    set(_gRPC_NACOS_LIBRARIES nacos-cli)
    if(gRPC_INSTALL AND _gRPC_INSTALL_SUPPORTED_FROM_MODULE)
      install(TARGETS nacos-cli
              EXPORT gRPCTargets
              LIBRARY DESTINATION lib)
      install(DIRECTORY ${NACOS_ROOT_DIR}/include/
              DESTINATION include/nacos
              FILES_MATCHING PATTERN "*.h*")
    endif()
  else()
    message(WARNING "gRPC_NACOS_PROVIDER is \"module\" but NACOS_ROOT_DIR is wrong")
  endif()
elseif(gRPC_NACOS_PROVIDER STREQUAL "package")
  include_directories(${CMAKE_PREFIX_PATH}/include/nacos)
  link_directories(${CMAKE_PREFIX_PATH}/lib)
endif()
