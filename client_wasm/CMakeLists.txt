cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(example)

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

add_executable(${PROJECT_NAME} main.cpp)
#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Web Configurations
if (${PLATFORM} STREQUAL "Web")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html") # Tell Emscripten to build an example.html file.
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -lwebsocket.js --preload-file assets ")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-sMIN_WEBGL_VERSION=2 -s ALLOW_MEMORY_GROWTH=1 -sMAX_WEBGL_VERSION=2 ")

    endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
