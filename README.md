# Cura

A high-performance, cross-platform photo deduplication tool built with C++ and Slint UI.

## Features

- **Hash-based detection** (exact duplicates) - mandatory, always enabled
- **Visual similarity detection** (similar images) - optional, user can enable
- **Duplicate handling**: Delete (move to trash) or Move to another folder
- **Undo support**: Restore last deleted/moved files
- **High performance**: Parallel processing, handles thousands of images
- **Cross-platform**: Windows, macOS, Linux
- **Modern UI**: GPU-accelerated Slint UI with dark theme

## Build

### Prerequisites

- CMake 3.20+
- C++20 compiler
- Slint (UI framework)

### Install Slint

```bash
# Using vcpkg
vcpkg install slint

# Or download from https://slint.dev
```

### Build Commands

```bash
cmake -B build
cmake --build build
```

### Run Tests

```bash
cmake -B build -DCURA_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Usage

```bash
./build/src/cura
```

1. Select folders to scan
2. Choose detection mode (hash only or hash + visual similarity)
3. Review duplicate groups
4. Handle duplicates: Delete or Move to folder

## Project Structure

```
cura/
├── src/core/       # Core engine
├── src/threading/  # Thread pool
├── src/ui/         # Slint bindings
├── ui/             # Slint UI files
├── tests/          # Unit tests
└── third_party/    # External headers
```

## License

[Choose license: GPLv3 (open source) or commercial for Slint]

## Documentation

See [PLAN.md](PLAN.md) for detailed implementation plan.
See [CLAUDE.md](CLAUDE.md) for project instructions.