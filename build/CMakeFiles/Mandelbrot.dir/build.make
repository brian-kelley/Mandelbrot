# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.0

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/Brian/Mandelbrot

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/Brian/Mandelbrot/build

# Include any dependencies generated for this target.
include CMakeFiles/Mandelbrot.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Mandelbrot.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Mandelbrot.dir/flags.make

CMakeFiles/Mandelbrot.dir/mandelbrot.c.o: CMakeFiles/Mandelbrot.dir/flags.make
CMakeFiles/Mandelbrot.dir/mandelbrot.c.o: ../mandelbrot.c
	$(CMAKE_COMMAND) -E cmake_progress_report /Users/Brian/Mandelbrot/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object CMakeFiles/Mandelbrot.dir/mandelbrot.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/Mandelbrot.dir/mandelbrot.c.o   -c /Users/Brian/Mandelbrot/mandelbrot.c

CMakeFiles/Mandelbrot.dir/mandelbrot.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Mandelbrot.dir/mandelbrot.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -E /Users/Brian/Mandelbrot/mandelbrot.c > CMakeFiles/Mandelbrot.dir/mandelbrot.c.i

CMakeFiles/Mandelbrot.dir/mandelbrot.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Mandelbrot.dir/mandelbrot.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -S /Users/Brian/Mandelbrot/mandelbrot.c -o CMakeFiles/Mandelbrot.dir/mandelbrot.c.s

CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.requires:
.PHONY : CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.requires

CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.provides: CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.requires
	$(MAKE) -f CMakeFiles/Mandelbrot.dir/build.make CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.provides.build
.PHONY : CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.provides

CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.provides.build: CMakeFiles/Mandelbrot.dir/mandelbrot.c.o

CMakeFiles/Mandelbrot.dir/lodepng.c.o: CMakeFiles/Mandelbrot.dir/flags.make
CMakeFiles/Mandelbrot.dir/lodepng.c.o: ../lodepng.c
	$(CMAKE_COMMAND) -E cmake_progress_report /Users/Brian/Mandelbrot/build/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object CMakeFiles/Mandelbrot.dir/lodepng.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/Mandelbrot.dir/lodepng.c.o   -c /Users/Brian/Mandelbrot/lodepng.c

CMakeFiles/Mandelbrot.dir/lodepng.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Mandelbrot.dir/lodepng.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -E /Users/Brian/Mandelbrot/lodepng.c > CMakeFiles/Mandelbrot.dir/lodepng.c.i

CMakeFiles/Mandelbrot.dir/lodepng.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Mandelbrot.dir/lodepng.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -S /Users/Brian/Mandelbrot/lodepng.c -o CMakeFiles/Mandelbrot.dir/lodepng.c.s

CMakeFiles/Mandelbrot.dir/lodepng.c.o.requires:
.PHONY : CMakeFiles/Mandelbrot.dir/lodepng.c.o.requires

CMakeFiles/Mandelbrot.dir/lodepng.c.o.provides: CMakeFiles/Mandelbrot.dir/lodepng.c.o.requires
	$(MAKE) -f CMakeFiles/Mandelbrot.dir/build.make CMakeFiles/Mandelbrot.dir/lodepng.c.o.provides.build
.PHONY : CMakeFiles/Mandelbrot.dir/lodepng.c.o.provides

CMakeFiles/Mandelbrot.dir/lodepng.c.o.provides.build: CMakeFiles/Mandelbrot.dir/lodepng.c.o

# Object files for target Mandelbrot
Mandelbrot_OBJECTS = \
"CMakeFiles/Mandelbrot.dir/mandelbrot.c.o" \
"CMakeFiles/Mandelbrot.dir/lodepng.c.o"

# External object files for target Mandelbrot
Mandelbrot_EXTERNAL_OBJECTS =

Mandelbrot: CMakeFiles/Mandelbrot.dir/mandelbrot.c.o
Mandelbrot: CMakeFiles/Mandelbrot.dir/lodepng.c.o
Mandelbrot: CMakeFiles/Mandelbrot.dir/build.make
Mandelbrot: CMakeFiles/Mandelbrot.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C executable Mandelbrot"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Mandelbrot.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Mandelbrot.dir/build: Mandelbrot
.PHONY : CMakeFiles/Mandelbrot.dir/build

CMakeFiles/Mandelbrot.dir/requires: CMakeFiles/Mandelbrot.dir/mandelbrot.c.o.requires
CMakeFiles/Mandelbrot.dir/requires: CMakeFiles/Mandelbrot.dir/lodepng.c.o.requires
.PHONY : CMakeFiles/Mandelbrot.dir/requires

CMakeFiles/Mandelbrot.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Mandelbrot.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Mandelbrot.dir/clean

CMakeFiles/Mandelbrot.dir/depend:
	cd /Users/Brian/Mandelbrot/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/Brian/Mandelbrot /Users/Brian/Mandelbrot /Users/Brian/Mandelbrot/build /Users/Brian/Mandelbrot/build /Users/Brian/Mandelbrot/build/CMakeFiles/Mandelbrot.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Mandelbrot.dir/depend

