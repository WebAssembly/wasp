MAKEFILE_NAME := $(lastword $(MAKEFILE_LIST))
ROOT_DIR := $(dir $(abspath $(MAKEFILE_NAME)))

USE_NINJA ?= 0
CMAKE_CMD ?= cmake

DEFAULT_SUFFIX = clang-debug

COMPILERS := GCC GCC_I686 CLANG EMCC
BUILD_TYPES := DEBUG RELEASE
SANITIZERS := ASAN MSAN LSAN UBSAN
CONFIGS := NORMAL $(SANITIZERS)
#
# directory names
GCC_DIR := gcc/
GCC_I686_DIR := gcc-i686/
CLANG_DIR := clang/
DEBUG_DIR := Debug/
RELEASE_DIR := Release/
NORMAL_DIR :=
ASAN_DIR := asan/
MSAN_DIR := msan/
LSAN_DIR := lsan/
UBSAN_DIR := ubsan/

# CMake flags
GCC_FLAG := -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
GCC_I686_FLAG := -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
	-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32
CLANG_FLAG := -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
DEBUG_FLAG := -DCMAKE_BUILD_TYPE=Debug
RELEASE_FLAG := -DCMAKE_BUILD_TYPE=Release
NORMAL_FLAG :=
ASAN_FLAG := -DUSE_ASAN=ON
MSAN_FLAG := -DUSE_MSAN=ON
LSAN_FLAG := -DUSE_LSAN=ON
UBSAN_FLAG := -DUSE_UBSAN=ON

# make target prefixes
GCC_PREFIX := gcc
GCC_I686_PREFIX := gcc-i686
CLANG_PREFIX := clang
DEBUG_PREFIX := -debug
RELEASE_PREFIX := -release
NORMAL_PREFIX :=
ASAN_PREFIX := -asan
MSAN_PREFIX := -msan
LSAN_PREFIX := -lsan
UBSAN_PREFIX := -ubsan

ifeq ($(USE_NINJA),1)
BUILD_CMD := ninja
BUILD_FILE := build.ninja
GENERATOR := Ninja
else
BUILD_CMD := +$(MAKE) --no-print-directory
BUILD_FILE := Makefile
GENERATOR := "Unix Makefiles"
endif

CMAKE_DIR = out/$($(1)_DIR)$($(2)_DIR)$($(3)_DIR)
BUILD_TARGET = $($(1)_PREFIX)$($(2)_PREFIX)$($(3)_PREFIX)
INSTALL_TARGET = install-$($(1)_PREFIX)$($(2)_PREFIX)$($(3)_PREFIX)
TEST_TARGET = test-$($(1)_PREFIX)$($(2)_PREFIX)$($(3)_PREFIX)

define CMAKE
$(call CMAKE_DIR,$(1),$(2),$(3)):
	mkdir -p $(call CMAKE_DIR,$(1),$(2),$(3))

$(call CMAKE_DIR,$(1),$(2),$(3))$$(BUILD_FILE): | $(call CMAKE_DIR,$(1),$(2),$(3))
	cd $(call CMAKE_DIR,$(1),$(2),$(3)) && \
	$$(CMAKE_CMD) -G $$(GENERATOR) $$(ROOT_DIR) $$($(1)_FLAG) $$($(2)_FLAG) $$($(3)_FLAG)
endef

define BUILD
.PHONY: $(call BUILD_TARGET,$(1),$(2),$(3))
$(call BUILD_TARGET,$(1),$(2),$(3)): $(call CMAKE_DIR,$(1),$(2),$(3))$$(BUILD_FILE)
	$$(BUILD_CMD) -C $(call CMAKE_DIR,$(1),$(2),$(3)) all
endef

define INSTALL
.PHONY: $(call INSTALL_TARGET,$(1),$(2),$(3))
$(call INSTALL_TARGET,$(1),$(2),$(3)): $(call CMAKE_DIR,$(1),$(2),$(3))$$(BUILD_FILE)
	$$(BUILD_CMD) -C $(call CMAKE_DIR,$(1),$(2),$(3)) install
endef

define TEST
.PHONY: $(call TEST_TARGET,$(1),$(2),$(3))
$(call TEST_TARGET,$(1),$(2),$(3)): $(call CMAKE_DIR,$(1),$(2),$(3))$$(BUILD_FILE)
	$$(BUILD_CMD) -C $(call CMAKE_DIR,$(1),$(2),$(3)) run-tests
test-everything: $(CALL TEST_TARGET,$(1),$(2),$(3))
endef

.PHONY: all install test
all: $(DEFAULT_SUFFIX)
install: install-$(DEFAULT_SUFFIX)
test: test-$(DEFAULT_SUFFIX)

.PHONY: clean
clean:
	rm -rf out

.PHONY: test-everything
test-everything:

# running CMake
$(foreach CONFIG,$(CONFIGS), \
	$(foreach COMPILER,$(COMPILERS), \
		$(foreach BUILD_TYPE,$(BUILD_TYPES), \
			$(eval $(call CMAKE,$(COMPILER),$(BUILD_TYPE),$(CONFIG))))))

# building
$(foreach CONFIG,$(CONFIGS), \
	$(foreach COMPILER,$(COMPILERS), \
		$(foreach BUILD_TYPE,$(BUILD_TYPES), \
			$(eval $(call BUILD,$(COMPILER),$(BUILD_TYPE),$(CONFIG))))))

# installing
$(foreach CONFIG,$(CONFIGS), \
	$(foreach COMPILER,$(COMPILERS), \
		$(foreach BUILD_TYPE,$(BUILD_TYPES), \
			$(eval $(call INSTALL,$(COMPILER),$(BUILD_TYPE),$(CONFIG))))))

# test running
$(foreach CONFIG,$(CONFIGS), \
	$(foreach COMPILER,$(COMPILERS), \
		$(foreach BUILD_TYPE,$(BUILD_TYPES), \
			$(eval $(call TEST,$(COMPILER),$(BUILD_TYPE),$(CONFIG))))))
