# fooyin VGMStream input plugin

An input plugin for [fooyin](https://www.fooyin.org/) that uses [VGMStream](https://github.com/vgmstream/vgmstream)
to play streamed video game music audio.

## Installation

Pre-built Linux binaries are available from the project's [GitHub releases](https://github.com/fooyin/fooyin-plugin-vgmstream/releases).
Download `fyplugin_vgmstream.so` and either install it from the `Plugins` settings page 
or place it in fooyin's plugin directory (`~/.local/lib/fooyin/plugins`), then restart fooyin.

## Building from source

You will need:

- A C++ compiler with C++23 support
- CMake 3.14 or later
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
