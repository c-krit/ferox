#
# Copyright (c) 2021 jdeokkim
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

.PHONY: all clean

PROJECT_NAME := c-krit/ferox
PROJECT_PATH := ../ferox
PROJECT_PREFIX := $(shell tput setaf 2)$(PROJECT_NAME):$(shell tput sgr0)

RAYLIB_PATH ?= ../../raylib

BINARY_PATH := bin
INCLUDE_PATH := $(PROJECT_PATH)/include
LIBRARY_PATH := $(PROJECT_PATH)/lib
SOURCE_PATH := src

EXAMPLES := \
	example_broadphase \
	example_collision \
	example_dynamics \
	example_geometry
    
SOURCES := $(EXAMPLES:%=$(SOURCE_PATH)/%.c)
RESULTS := $(EXAMPLES:%=$(BINARY_PATH)/%)

HOST_OS := LINUX
TARGET_OS := $(HOST_OS)

ifeq ($(OS),Windows_NT)
	PROJECT_PREFIX := $(PROJECT_NAME):
	HOST_OS := WINDOWS
else
	UNAME = $(shell uname)
	ifeq ($(UNAME),Linux)
		HOST_OS = LINUX
	endif
endif

CC := gcc
CFLAGS := -D_DEFAULT_SOURCE -DHAVE_RAYLIB -g $(INCLUDE_PATH:%=-I%) -std=gnu99 -O2
LDFLAGS := -L$(LIBRARY_PATH)
LDLIBS := -lferox -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

ifeq ($(TARGET_OS),WINDOWS)
	RESULTS := $(EXAMPLES:%=$(BINARY_PATH)/%.exe)
	CC := x86_64-w64-mingw32-gcc
	CFLAGS += -I$(RAYLIB_PATH)/src
	LDFLAGS += -L$(RAYLIB_PATH)/src 
	LDLIBS := -lferox -lraylib -lopengl32 -lgdi32 -lwinmm -lpthread
endif

all: pre-build build post-build

pre-build:
	@echo "$(PROJECT_PREFIX) Using: '$(CC)' to build examples."
    
build: $(RESULTS)

$(BINARY_PATH)/%: $(SOURCE_PATH)/%.c
	@mkdir -p $(BINARY_PATH)
	@echo "$(PROJECT_PREFIX) Compiling: $@ (from $<)"
	@$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LDLIBS)
    
$(BINARY_PATH)/%.exe: $(SOURCE_PATH)/%.c
	@mkdir -p $(BINARY_PATH)
	@echo "$(PROJECT_PREFIX) Compiling: $@ (from $<)"
	@$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LDLIBS)
    
post-build:
	@echo "$(PROJECT_PREFIX) Build complete."

clean:
	@echo "$(PROJECT_PREFIX) Cleaning up."
	@find . -type d -name bin -exec rm -rf {} +