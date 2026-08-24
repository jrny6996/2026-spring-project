cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(game)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    # WebGL 2 has VAO in core; ES2 only enables VAO via OES extension string, which WebGL 2 does not
    # advertise the same way. Without VAO, rlgl binds batch attribs using loc -1 for normals →
    # INVALID_VALUE on glEnableVertexAttribArray. GLES3 forces VAO on and fixes 2D batch + models.
    if(${PLATFORM} STREQUAL "Web")
      set(GRAPHICS "GRAPHICS_API_OPENGL_ES3" CACHE STRING "raylib graphics API" FORCE)
    endif()
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

# Our Project

# add_subdirectory(vendor/jet-live)

# include(vendor/jet-live/cmake/jet_live_setup.cmake)

# set(JET_LIVE_SHARED ON)

# target_link_libraries(${PROJECT_NAME}  PRIVATE jet-live)

add_executable(${PROJECT_NAME} main.cpp)
#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE CLIENT_DEBUG_BUILD=1)
endif()

# Web Configurations
if (${PLATFORM} STREQUAL "Web")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html") # Tell Emscripten to build an game.html file.
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
    # miniaudio (raylib) Web Audio reads Module.HEAPF32 from JS; Emscripten no longer exports heap views by default.
    # Keep preloaded asset packs in IndexedDB across reloads to avoid re-fetching heavy model data.
    set(WEB_COMMON_LINK_FLAGS "-s USE_GLFW=3 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -sEXPORTED_RUNTIME_METHODS=['HEAPF32','HEAPU8','HEAP32','HEAPU32'] -lwebsocket.js --preload-file assets --use-preload-plugins --use-preload-cache")
    set(WEB_PROFILE_LINK_FLAGS "")
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
      # Release shell: full-width canvas and absolute fullscreen button.
      set(WEB_PROFILE_LINK_FLAGS "--shell-file ${CMAKE_SOURCE_DIR}/shell_release.html -s ASSERTIONS=0")
    elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
      # Debug shell includes fullscreen/audio controls and debug-friendly overlay behavior.
      set(WEB_PROFILE_LINK_FLAGS "--shell-file ${CMAKE_SOURCE_DIR}/shell_debug.html -s ASSERTIONS=2 -sSTACK_SIZE=2097152")
    else()
      # Debug profile keeps stronger diagnostics.
      set(WEB_PROFILE_LINK_FLAGS "-s ASSERTIONS=2")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${WEB_COMMON_LINK_FLAGS} ${WEB_PROFILE_LINK_FLAGS}")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-sMIN_WEBGL_VERSION=2 -s ALLOW_MEMORY_GROWTH=1 -sMAX_WEBGL_VERSION=2")

    endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
