#!/bin/bash

echo "==========================================="
echo "Building Tiny Compiler with For Loop Support"
echo "==========================================="

# Clean and rebuild
make clean
make

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "==========================================="
echo "Test 1: Basic For and Downto Loop"
echo "==========================================="
echo "Source code (test_for_loop.tiny):"
cat test-progs/test_for_loop.tiny
echo ""
echo "Expected output: 1 2 3 4 5 5 4 3 2 1"
echo ""
echo "Running test..."
./tc test-progs/test_for_loop.tiny
echo ""

echo "==========================================="
echo "Test 2: Nested For Loops"
echo "==========================================="
echo "Source code (test_nested_for.tiny):"
cat test-progs/test_nested_for.tiny
echo ""
echo "Expected output: 1 1  1 2  1 3  2 1  2 2  2 3  3 1  3 2  3 3"
echo ""
echo "Running test..."
./tc test-progs/test_nested_for.tiny
echo ""

echo "==========================================="
echo "Test 3: For Loop with Computation"
echo "==========================================="
echo "Source code (test_for_sum.tiny):"
cat test-progs/test_for_sum.tiny
echo ""
echo "Expected output: 55 (sum of 1 to 10)"
echo ""
echo "Running test..."
./tc test-progs/test_for_sum.tiny
echo ""

echo "==========================================="
echo "All tests completed!"
echo "==========================================="
