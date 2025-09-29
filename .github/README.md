# SAGE DB GitHub Actions CI/CD

This directory contains GitHub Actions workflows for automated testing of the SAGE DB component.

## Workflows

### 1. `ci-tests.yml` - Comprehensive CI Tests
- **Triggers**: Pull requests and pushes to `main` and `main-dev` branches
- **Platforms**: Ubuntu Latest
- **Build Types**: Release and Debug
- **Features**:
  - Full build with FAISS support
  - Comprehensive test suite including multimodal tests
  - Artifact upload for debugging failures
  - Conda-based dependency management

### 2. `quick-test.yml` - Fast CI Tests
- **Triggers**: Pull requests affecting source code files
- **Platforms**: Ubuntu Latest (Release only)
- **Features**:
  - Faster execution with cached dependencies
  - Essential tests only
  - Library dependency verification
  - Optimized for quick feedback

## Dependencies

The workflows automatically install:
- FAISS CPU version (via conda)
- OpenBLAS (via conda)
- CMake and build tools
- GCC/G++ compilers

## Test Coverage

Both workflows run:
- `test_sage_db` - Core functionality tests including:
  - Basic vector operations
  - Metadata operations
  - Batch operations
  - Filtered search
  - Persistence (save/load)
  - Performance benchmarks

- `test_multimodal` - Multimodal fusion tests (when enabled)

## Local Development

To run tests locally similar to CI:

```bash
# Install dependencies via conda
conda install -y faiss-cpu openblas cmake make gcc_linux-64 gxx_linux-64

# Configure and build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" \
  -DFAISS_ROOT="$CONDA_PREFIX"

cmake --build build --parallel

# Run tests
cd build
LD_LIBRARY_PATH=$PWD:$CONDA_PREFIX/lib ctest --output-on-failure
```

## Troubleshooting

If tests fail:
1. Check the uploaded artifacts for detailed logs
2. Verify library dependencies with `ldd`
3. Ensure FAISS is properly linked
4. Check for any runtime library path issues

## Contributing

When adding new tests:
1. Add them to the appropriate `tests/` directory
2. Update `CMakeLists.txt` to include the new test
3. Ensure tests can run in the CI environment
4. Test both Release and Debug builds locally before submitting