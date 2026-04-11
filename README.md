# Software Rasterizer Playground

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

The project is licensed under the MIT License.

- The [Inter](https://github.com/rsms/inter/) font is licensed under the [SIL Open Font License 1.1](https://openfontlicense.org/open-font-license-official-text/)

## References

- [swr](https://github.com/flubbe/swr): The software rasterizer used.
