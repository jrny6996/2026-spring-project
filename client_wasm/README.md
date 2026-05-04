# raylib CMake Project

This provides a base project template which builds with
[CMake](https://cmake.org).

## C++ API documentation (Doxygen)

Install the `doxygen` package, then from this directory:

```bash
# Debian / Ubuntu
sudo apt install doxygen

./doxygen.sh
# same as: doxygen Doxyfile
```

Other platforms: [doxygen.nl/download](https://www.doxygen.nl/download.html) or `brew install doxygen` (macOS).

Open **`doxygen-html/index.html`**. Output is listed in `.gitignore`; CI copies it to GitHub Pages under **`/cpp/`** on deploy.

## Usage

To compile the game, use one of the following dependending on your build
target...

### Desktop

Use the following to build for desktop:

```bash
cmake -B build
cmake --build build
```

### Web

Compiling for the web requires the
[Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
mkdir build
cd build
emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXECUTABLE_SUFFIX=".html"
emmake make
```
