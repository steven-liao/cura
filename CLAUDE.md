# Cura - Photo Deduplication Tool

## Project Overview
Cura is a high-performance, cross-platform photo deduplication tool built with C++ and Slint UI.

## Implementation Plan
See [PLAN.md](PLAN.md) for the detailed implementation plan including:
- Architecture overview
- Core component designs
- UI screen mockups
- Performance optimization strategies
- File structure and naming conventions

## Build Commands

### Build the project
```bash
cmake -B build
cmake --build build
```

### Run tests
```bash
cmake -B build -DCURA_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

### Run the application
```bash
./build/src/cura
```

## Project Structure
- `src/core/` - Core engine (scanner, hasher, clusterer, fileops, image)
- `src/threading/` - Thread pool implementation
- `src/ui/` - Slint UI bindings
- `ui/` - Slint UI files (.slint)
- `tests/` - Unit tests (Catch2)
- `third_party/` - Third-party headers

## Naming Conventions
- Classes: `Cura*` prefix (e.g., CuraScanner, CuraHasher)
- Namespace: `cura::`
- Files: `cura_*.cpp`, `cura_*.hpp`
- Slint files: `cura_*.slint`

## Key Features
- Hash-based detection (mandatory, default)
- Visual similarity detection (optional, user toggle)
- Duplicate handling: Delete (trash) or Move to folder
- Undo support for operations

## Dependencies
- Slint (UI framework)
- stb_image (image loading)
- xxhash (fast hashing)
- Catch2 (testing)

## Testing
Unit tests use Catch2. Test files are in `tests/` directory.
Run `ctest` from build directory.