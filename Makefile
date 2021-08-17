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

BUILD := DEFAULT
BUILD_TEXT := "$(shell echo $(BUILD) | tr '[:upper:]' '[:lower:]')"

PROJECT_NAME := "c-krit/ferox"
PROJECT_PATH := ferox
PROJECT_PREFIX := "$(shell tput setaf 2)$(PROJECT_NAME):$(shell tput sgr0)"

RAYLIB_PATH := ../raylib

INCLUDE_PATH := $(PROJECT_PATH)/include
LIBRARY_PATH := $(PROJECT_PATH)/lib
SOURCE_PATH := $(PROJECT_PATH)/src

INCLUDE_PATH += $(SOURCE_PATH)/external

SOURCES := \
	$(SOURCE_PATH)/collision.c \
	$(SOURCE_PATH)/dynamics.c \
	$(SOURCE_PATH)/geometry.c \
	$(SOURCE_PATH)/quadtree.c \
	$(SOURCE_PATH)/timer.c \
	$(SOURCE_PATH)/utils.c \
	$(SOURCE_PATH)/vector.c \
	$(SOURCE_PATH)/world.c
    
OBJECTS := $(SOURCES:.c=.o)
RESULT := $(LIBRARY_PATH)/lib$(PROJECT_PATH)-standalone.a

ifeq ($(BUILD),DEFAULT)
	SOURCES += $(SOURCE_PATH)/debug.c
    OBJECTS += $(SOURCE_PATH)/debug.o
    RESULT := $(LIBRARY_PATH)/lib$(PROJECT_PATH).a
endif

HOST_OS := LINUX
TARGET_OS := $(HOST_OS)

ifeq ($(OS),Windows_NT)
	PROJECT_PREFIX := "$(PROJECT_NAME):"
	HOST_OS := WINDOWS
else
	UNAME = $(shell uname)
	ifeq ($(UNAME),Linux)
		HOST_OS = LINUX
	endif
endif

CC := gcc
AR := ar
CFLAGS := -c -D_DEFAULT_SOURCE -g $(INCLUDE_PATH:%=-I%) -O2 -std=gnu99

ifeq ($(BUILD),DEFAULT)
	CFLAGS += -DHAVE_RAYLIB
endif

ifeq ($(TARGET_OS),WINDOWS)
	CC := x86_64-w64-mingw32-gcc
	AR := x86_64-w64-mingw32-ar
    CFLAGS += -I$(RAYLIB_PATH)/src
endif

all: pre-build build post-build

pre-build:
	@echo "$(PROJECT_PREFIX) Using: '$(CC)' and '$(AR)' to build a $(BUILD_TEXT)-mode static library."
    
build: $(RESULT)

$(SOURCE_PATH)/%.o: $(SOURCE_PATH)/%.c
	@echo "$(PROJECT_PREFIX) Compiling: $@ (from $<)"
	@$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LDLIBS)
    
$(RESULT): $(OBJECTS)
	@mkdir -p $(LIBRARY_PATH)
	@echo "$(PROJECT_PREFIX) Linking: $(RESULT)"
	@$(AR) rcs $(RESULT) $(OBJECTS)

post-build:
	@echo "$(PROJECT_PREFIX) Build complete."

clean:
	@echo "$(PROJECT_PREFIX) Cleaning up."
	@rm -rf $(LIBRARY_PATH)/*.a
	@rm -rf $(SOURCE_PATH)/*.o