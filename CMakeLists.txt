cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(PkgConfig REQUIRED)

if(NOT APPLE)
  find_package(glfw3 REQUIRED)

  pkg_check_modules(LIBUV REQUIRED IMPORTED_TARGET libuv)

  find_package(GLEW REQUIRED)
  include_directories(${GLEW_INCLUDE_DIRS})
  link_libraries(${GLEW_LIBRARIES})
  link_libraries(${glfw3_LIBRARIES})

  target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC -Wl,--no-allow-shlib-undefined -Wl,-z,defs GLEW::GLEW OpenGL glfw m PkgConfig::LIBUV)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)
endif()

if(APPLE)
  include_directories(/opt/homebrew/include)

  find_library(LIB_GLFW_ARM NAMES glfw PATHS /opt/homebrew/lib REQUIRED NO_DEFAULT_PATH)
  find_library(LIB_UV_ARM NAMES uv PATHS /opt/homebrew/lib REQUIRED NO_DEFAULT_PATH)

  find_library(LIB_GLFW_X86 NAMES glfw PATHS /usr/local/lib REQUIRED NO_DEFAULT_PATH)
  find_library(LIB_UV_X86 NAMES uv PATHS /usr/local/lib REQUIRED NO_DEFAULT_PATH)

  set(GLFW_LIBRARY "${CMAKE_BINARY_DIR}/libs-gen/glfw-multiarch.a")
  set(UV_LIBRARY "${CMAKE_BINARY_DIR}/libs-gen/uv-multiarch.a")

  add_custom_target(GLFW_MULTIARCH
		DEPENDS LIB_GLFW_ARM LIB_GLFW_X86
		BYPRODUCTS GLFW_LIBRARY
		COMMAND lipo ${LIB_GLFW_ARM} ${LIB_GLFW_X86} -create -output "${GLFW_LIBRARY}"
	)

  add_custom_target(UV_MULTIARCH
		DEPENDS LIB_UV_ARM LIB_UV_X86
		BYPRODUCTS UV_LIBRARY
		COMMAND lipo ${LIB_UV_ARM} ${LIB_UV_X86} -create -output "${UV_LIBRARY}"
	)

  set_target_properties(GLFW_MULTIARCH PROPERTIES IMPORTED_LOCATION "${GLFW_LIBRARY}")
  set_target_properties(UV_MULTIARCH PROPERTIES IMPORTED_LOCATION "${UV_LIBRARY}")

  add_dependencies(${CMAKE_PROJECT_NAME} GLFW_MULTIARCH)
  add_dependencies(${CMAKE_PROJECT_NAME} UV_MULTIARCH)

  link_directories("${CMAKE_BINARY_DIR}/libs-gen")
  link_libraries(glfw uv)
  
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs OpenGL::GL)
endif()

find_package(libobs REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${OpenGL_INCLUDE_DIRS})

link_libraries(${OpenGL_LIBRARIES})

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

add_compile_definitions(TRILIBRARY ENABLE_LOCALES ANSI_DECLARATORS)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${LIBUV_INCLUDE_DIRS} src/triangle src/cJSON src/tinycthread/source src/projector)

set(
  ShaderSources 
  "src/shaders/blend.fragment.shader"
  "src/shaders/blend.vertex.shader"
  "src/shaders/blur.fragment.shader"
  "src/shaders/blur.vertex.shader"
  "src/shaders/color-corrector.fragment.shader"
  "src/shaders/color-corrector.vertex.shader"
  "src/shaders/direct.fragment.shader"
  "src/shaders/direct.vertex.shader"
)

add_custom_command(
  OUTPUT src/projector/shaders.h
  COMMAND build-aux/compile-shaders.sh 
  DEPENDS build-aux/compile-shaders.sh ${ShaderSources}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

target_sources(
  ${CMAKE_PROJECT_NAME} 
  PRIVATE 
  src/plugin-main.c
  src/triangle/triangle.h
  src/triangle/triangle.c
  src/cJSON/cJSON_Utils.h
  src/cJSON/cJSON_Utils.c
  src/cJSON/cJSON.h
  src/cJSON/cJSON.c
  src/tinycthread/source/tinycthread.h
  src/tinycthread/source/tinycthread.c
  src/projector/clock.c
  src/projector/clock.h
  src/projector/config.c
  src/projector/config-debug.c
  src/projector/config-debug.h
  src/projector/config.h
  src/projector/config-parse.c
  src/projector/config-parse.h
  src/projector/config-serialize.c
  src/projector/config-serialize.h
  src/projector/config-structs.h
  src/projector/custom-math.h
  src/projector/debug.c
  src/projector/debug.h
  src/projector/loop.c
  src/projector/loop.h
  src/projector/monitor.c
  src/projector/monitor.h
  src/projector/ogl-loader.c
  src/projector/ogl-loader.h
  src/projector/render.c
  src/projector/render.h
  src/projector/render-obs.c
  src/projector/render-obs.h
  src/projector/render-pixel-unpack-buffer.c
  src/projector/render-pixel-unpack-buffer.h
  src/projector/shaders.h
  src/projector/virtual-screen.c
  src/projector/virtual-screen.h
  src/projector/vs-black-level-adjust.c
  src/projector/vs-black-level-adjust.h
  src/projector/vs-blend.c
  src/projector/vs-blend.h
  src/projector/vs-color-corrector.c
  src/projector/vs-color-corrector.h
  src/projector/vs-help-lines.c
  src/projector/vs-help-lines.h
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
