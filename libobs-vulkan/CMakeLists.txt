project(libobs-vulkan)

add_library(libobs-vulkan SHARED)
add_library(OBS::libobs-vulkan ALIAS libobs-vulkan)

target_sources(
  libobs-vulkan
  PRIVATE vk-includes.hpp
          vk-subsystem.hpp
          vk-subsystem.cpp
          vk-instance.cpp
          vk-device.cpp
          vk-surface.cpp
          vk-shader.cpp
          vk-buffer.cpp
          vk-helpers.hpp
          vk-sampler.cpp
          vk-texture.cpp
          vk-stagesurf.cpp
          vk-shaderhandler.hpp
          vk-shaderhandler.cpp)

find_package(Vulkan REQUIRED)

target_include_directories(libobs-vulkan PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_compile_features(libobs-vulkan PRIVATE cxx_std_17)

target_link_directories(libobs-vulkan PRIVATE ${CMAKE_PREFIX_PATH}/lib)

target_link_libraries(libobs-vulkan PRIVATE OBS::libobs Vulkan::Vulkan)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
  target_link_libraries(libobs-vulkan PRIVATE shaderc_combinedd)
else()
  target_link_libraries(libobs-vulkan PRIVATE shaderc_combined)
endif()

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS Library Vulkan wrapper")

elseif(OS_MACOS)
  set_target_properties(libobs-vulkan PROPERTIES PREFIX "")

elseif(OS_POSIX)
  set_target_properties(libobs-vulkan PROPERTIES PREFIX "")

  if(ENABLE_WAYLAND)
    find_package(Wayland REQUIRED)
  endif()
endif()

set_target_properties(
  libobs-vulkan
  PROPERTIES FOLDER "core"
  VERSION "${OBS_VERSION_MAJOR}"
  SOVERSION "1")

setup_binary_target(libobs-vulkan)
