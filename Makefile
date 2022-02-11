#
# Copyright (c) 2021-2022 jdeokkim
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

_COLOR_BEGIN := $(shell tput setaf 2)
_COLOR_END := $(shell tput sgr0)

RAYLIB_PATH ?= ../raylib

PROJECT_NAME := ferox
PROJECT_FULL_NAME := c-krit/ferox

PROJECT_PATH := $(PROJECT_NAME)
PROJECT_PREFIX := $(_COLOR_BEGIN)$(PROJECT_FULL_NAME):$(_COLOR_END)

INCLUDE_PATH := \
	$(PROJECT_PATH)/include \
	$(RAYLIB_PATH)/src

LIBRARY_PATH := $(PROJECT_PATH)/lib
SOURCE_PATH := $(PROJECT_PATH)/src

INCLUDE_PATH += $(SOURCE_PATH)/external

SOURCES := \
	$(SOURCE_PATH)/broadphase.c \
	$(SOURCE_PATH)/collision.c  \
	$(SOURCE_PATH)/dynamics.c   \
	$(SOURCE_PATH)/geometry.c   \
	$(SOURCE_PATH)/timer.c      \
	$(SOURCE_PATH)/utils.c      \
	$(SOURCE_PATH)/world.c

OBJECTS := $(SOURCES:.c=.o)
TARGETS := $(LIBRARY_PATH)/lib$(PROJECT_NAME)-standalone.a

ifneq ($(BUILD),STANDALONE)
	SOURCES += $(SOURCE_PATH)/debug.c
	OBJECTS += $(SOURCE_PATH)/debug.o
	TARGETS := $(LIBRARY_PATH)/lib$(PROJECT_PATH).a
endif

HOST_PLATFORM := LINUX

ifeq ($(OS),Windows_NT)
	PROJECT_PREFIX := $(PROJECT_FULL_NAME):
	HOST_PLATFORM := WINDOWS
else
	UNAME = $(shell uname)

	ifeq ($(UNAME),Linux)
		HOST_PLATFORM = LINUX
	endif
endif

CC := gcc
AR := ar
CFLAGS := -D_DEFAULT_SOURCE -g $(INCLUDE_PATH:%=-I%) -O2 -std=gnu11

ifeq ($(BUILD),STANDALONE)
	CFLAGS += -DFEROX_STANDALONE
endif

PLATFORM := $(HOST_PLATFORM)

ifeq ($(PLATFORM),WINDOWS)
	ifneq ($(HOST_PLATFORM),WINDOWS)
		CC := x86_64-w64-mingw32-gcc
		AR := x86_64-w64-mingw32-ar
	endif
else ifeq ($(PLATFORM),WEB)
	CC := emcc
	AR := emar

	CFLAGS += -Wno-return-type
endif

all: pre-build build post-build

pre-build:
	@echo "$(PROJECT_PREFIX) Using: '$(CC)' and '$(AR)' to build a static library."
    
build: $(TARGETS)

$(SOURCE_PATH)/%.o: $(SOURCE_PATH)/%.c
	@echo "$(PROJECT_PREFIX) Compiling: $@ (from $<)"
	@$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS) $(LDLIBS)
    
$(TARGETS): $(OBJECTS)
	@mkdir -p $(LIBRARY_PATH)
	@echo "$(PROJECT_PREFIX) Linking: $(TARGETS)"
	@$(AR) rcs $(TARGETS) $(OBJECTS)

post-build:
	@echo "$(PROJECT_PREFIX) Build complete."

clean:
	@echo "$(PROJECT_PREFIX) Cleaning up."
	@rm -rf $(LIBRARY_PATH)/*.a
	@rm -rf $(SOURCE_PATH)/*.o
