#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Memory Manager Test Script${NC}"
echo "================================"

# Check which memory manager to test (default: buddy)
MEMORY_MANAGER=${1:-buddy}

if [ "$MEMORY_MANAGER" != "buddy" ] && [ "$MEMORY_MANAGER" != "mymalloc" ]; then
    echo -e "${RED}Error: Invalid memory manager. Use 'buddy' or 'mymalloc'${NC}"
    echo "Usage: ./test_memory.sh [buddy|mymalloc] [max_memory]"
    exit 1
fi

echo -e "Testing memory manager: ${GREEN}${MEMORY_MANAGER}${NC}\n"

# Get max_memory parameter (default: 50000)
MAX_MEMORY=${2:-50000}

# Compile the memory manager and test
echo "Compiling..."

if [ "$MEMORY_MANAGER" = "buddy" ]; then
    gcc -o test_mm test_mm.c test_util.c ../Kernel/memory/buddy.c \
        -I../Kernel/include -I. \
        -Wall -Wextra -std=c99
else
    gcc -o test_mm test_mm.c test_util.c ../Kernel/memory/myMalloc.c \
        -I../Kernel/include -I. \
        -Wall -Wextra -std=c99
fi

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Compilation successful!${NC}\n"

# Run the test
echo "Running test with max_memory=${MAX_MEMORY}..."
echo "================================"
./test_mm $MAX_MEMORY

TEST_RESULT=$?

echo "================================"

# Clean up
rm -f test_mm

# Report results
if [ $TEST_RESULT -eq 0 ]; then
    echo -e "\n${GREEN}✓ Test passed successfully!${NC}"
    exit 0
else
    echo -e "\n${RED}✗ Test failed!${NC}"
    exit 1
fi
