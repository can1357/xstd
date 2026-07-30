# xstd

[![ci](https://github.com/can1357/xstd/actions/workflows/ci.yml/badge.svg)](https://github.com/can1357/xstd/actions/workflows/ci.yml)

*xstd* is an extremely portable (x64, ARM64, WASM32) and header-only library extending the C++20 STL with useful features. It is mainly extracted from VTIL-Common in an attempt to hopefully completely move it at some point.

## Usage

Add the `includes/` directory to your include path, or consume the CMake target:

```cmake
add_subdirectory(xstd)               # or FetchContent
target_link_libraries(app PRIVATE xstd::xstd)
```

Requires a C++20 compiler. `xstd/gzip.hpp` and `xstd/zstd.hpp` additionally require zlib and libzstd.

## Development

- Formatting is enforced by `.clang-format` (clang-format >= 19); vendored files are excluded via `.clang-format-ignore`.
- `cmake -B build -DXSTD_BUILD_CHECKS=ON && cmake --build build` compiles every public header in a single TU — the same check CI runs on Linux (GCC, Clang) and macOS.

## License

BSD-3-Clause, see [LICENSE](LICENSE).
