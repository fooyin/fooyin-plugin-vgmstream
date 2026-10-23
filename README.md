# fooyin VGMStream input plugin

An input plugin for [fooyin](https://www.fooyin.org/) that uses [VGMStream](https://github.com/vgmstream/vgmstream) to play streamed video game audio.

## Installation

Pre-built Linux and Windows plugin bundles are available from the project's
[GitHub releases](https://github.com/fooyin/fooyin-plugin-vgmstream/releases). 
Download `fooyin-vgmstream.fyplugin` for both Linux x64 and Windows x64, or choose the smaller Linux or Windows bundle. 
Install it from the `Plugins` settings page, then restart fooyin.

## Building from source

You will need:

- A C++ compiler with C++23 support
- CMake 3.19 or later
- fooyin, including its CMake development files
- VGMStream (optional)

The repository includes vgmstream as a submodule and uses it automatically when a system installation cannot be found. 
Clone recursively, configure, and build:

```
git clone --recurse-submodules https://github.com/fooyin/fooyin-plugin-vgmstream.git
cd fooyin-plugin-vgmstream
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The built plugin is `build/fyplugin_vgmstream.so`. To install it using CMake:

```
cmake --install build
```

## License

fooyin-plugin-vgmstream is licensed under the [GNU General Public License, version 3 or later](COPYING).
