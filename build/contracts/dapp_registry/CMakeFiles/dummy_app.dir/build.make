# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ubuntu/contracts/contracts

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ubuntu/contracts/build/contracts

# Include any dependencies generated for this target.
include dapp_registry/CMakeFiles/dummy_app.dir/depend.make

# Include the progress variables for this target.
include dapp_registry/CMakeFiles/dummy_app.dir/progress.make

# Include the compile flags for this target's objects.
include dapp_registry/CMakeFiles/dummy_app.dir/flags.make

dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj: dapp_registry/CMakeFiles/dummy_app.dir/flags.make
dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj: /home/ubuntu/contracts/contracts/dapp_registry/src/dummy_app.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ubuntu/contracts/build/contracts/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj"
	cd /home/ubuntu/contracts/build/contracts/dapp_registry && /usr/local/alaio.cdt/bin/alaio-cpp  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj -c /home/ubuntu/contracts/contracts/dapp_registry/src/dummy_app.cpp

dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/dummy_app.dir/src/dummy_app.cpp.i"
	cd /home/ubuntu/contracts/build/contracts/dapp_registry && /usr/local/alaio.cdt/bin/alaio-cpp $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ubuntu/contracts/contracts/dapp_registry/src/dummy_app.cpp > CMakeFiles/dummy_app.dir/src/dummy_app.cpp.i

dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/dummy_app.dir/src/dummy_app.cpp.s"
	cd /home/ubuntu/contracts/build/contracts/dapp_registry && /usr/local/alaio.cdt/bin/alaio-cpp $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ubuntu/contracts/contracts/dapp_registry/src/dummy_app.cpp -o CMakeFiles/dummy_app.dir/src/dummy_app.cpp.s

dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.requires:

.PHONY : dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.requires

dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.provides: dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.requires
	$(MAKE) -f dapp_registry/CMakeFiles/dummy_app.dir/build.make dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.provides.build
.PHONY : dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.provides

dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.provides.build: dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj


# Object files for target dummy_app
dummy_app_OBJECTS = \
"CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj"

# External object files for target dummy_app
dummy_app_EXTERNAL_OBJECTS =

dapp_registry/dummy_app.wasm: dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj
dapp_registry/dummy_app.wasm: dapp_registry/CMakeFiles/dummy_app.dir/build.make
dapp_registry/dummy_app.wasm: dapp_registry/CMakeFiles/dummy_app.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ubuntu/contracts/build/contracts/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable dummy_app.wasm"
	cd /home/ubuntu/contracts/build/contracts/dapp_registry && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dummy_app.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
dapp_registry/CMakeFiles/dummy_app.dir/build: dapp_registry/dummy_app.wasm

.PHONY : dapp_registry/CMakeFiles/dummy_app.dir/build

dapp_registry/CMakeFiles/dummy_app.dir/requires: dapp_registry/CMakeFiles/dummy_app.dir/src/dummy_app.cpp.obj.requires

.PHONY : dapp_registry/CMakeFiles/dummy_app.dir/requires

dapp_registry/CMakeFiles/dummy_app.dir/clean:
	cd /home/ubuntu/contracts/build/contracts/dapp_registry && $(CMAKE_COMMAND) -P CMakeFiles/dummy_app.dir/cmake_clean.cmake
.PHONY : dapp_registry/CMakeFiles/dummy_app.dir/clean

dapp_registry/CMakeFiles/dummy_app.dir/depend:
	cd /home/ubuntu/contracts/build/contracts && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ubuntu/contracts/contracts /home/ubuntu/contracts/contracts/dapp_registry /home/ubuntu/contracts/build/contracts /home/ubuntu/contracts/build/contracts/dapp_registry /home/ubuntu/contracts/build/contracts/dapp_registry/CMakeFiles/dummy_app.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : dapp_registry/CMakeFiles/dummy_app.dir/depend
