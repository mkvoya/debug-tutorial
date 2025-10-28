# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Automatically find all source files in current directory
SOURCES = $(wildcard *.cpp)

# Target executable names (one for each cpp file)
TARGETS = $(basename $(SOURCES))

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGETS)

# Link object files to create executables
%: %.o
	$(CXX) $< -o $@

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target for removing compiled files
clean:
	rm -f $(OBJECTS) $(TARGETS)

# Print source files (for debugging)
print:
	@echo "Source files found: $(SOURCES)"

valgrind:
	valgrind --tool=memcheck --leak-check=full ./$(TARGETS)

# Phony targets
.PHONY: all clean print valgrind
