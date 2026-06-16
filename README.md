# Software Rasterizer Playground

[![License](https://img.shields.io/github/license/flubbe/swr-playground)](https://github.com/flubbe/swr-playground/blob/main/LICENSE.txt)
[![Build Status](https://github.com/flubbe/swr-playground/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/flubbe/swr-playground/actions)

An [ImGui](https://github.com/ocornut/imgui) application with a software-rendered
viewport and some controls.

## Build

The project uses [CMake](https://www.cmake.org) for building and dependency fetching.

Configure and build the project:

```bash
cmake -B build
cmake --build build
```

Run the executable:

```bash
./build/swr_playground
```

## License

The software in this repository is licensed under the MIT License. See [LICENSE.txt](./LICENSE.txt).

Bundled third-party assets are licensed separately:

- `assets/fonts/inter/Inter-Regular.ttf` from the [Inter project](https://github.com/rsms/inter/) is licensed under the [SIL Open Font License 1.1](https://openfontlicense.org/open-font-license-official-text/). See `assets/fonts/inter/LICENSE.txt`.
- `assets/textures/tiles/*` from [TextureCan](https://www.texturecan.com/details/490/) is licensed under the [Creative Commons CC0 1.0 Universal License](https://creativecommons.org/publicdomain/zero/1.0/). See `assets/textures/tiles/NOTICE`.


## References

- [swr](https://github.com/flubbe/swr): The software rasterizer used.
