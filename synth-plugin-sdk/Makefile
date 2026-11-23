# Polyend Community Edition Synth Plugin SDK (.psyn) Makefile
# Builds standalone relocatable SD-card synth plugins (/Synths/*.psyn)

SDK_DIR       := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
REPO_ROOT     := $(abspath $(SDK_DIR)/..)

ifneq ($(wildcard $(REPO_ROOT)/agt/10.3/bin/arm-none-eabi-g++),)
  TOOLCHAIN   ?= $(REPO_ROOT)/agt/10.3/bin/arm-none-eabi-
else ifneq ($(wildcard $(REPO_ROOT)/../tracker-ce/agt/10.3/bin/arm-none-eabi-g++),)
  TOOLCHAIN   ?= $(REPO_ROOT)/../tracker-ce/agt/10.3/bin/arm-none-eabi-
else
  TOOLCHAIN   ?= arm-none-eabi-
endif

CXX           := $(TOOLCHAIN)g++
CC            := $(TOOLCHAIN)gcc
PYTHON        ?= python3
BUILD_DIR     ?= $(SDK_DIR)/build
LXR_DSP_DIR   := $(SDK_DIR)/engines/lxr_dsp
LXR_DSP_SRCS  := $(wildcard $(LXR_DSP_DIR)/src/Soundengine/DSPAudio/*.c)
LXR_DSP_OBJS  := $(patsubst $(LXR_DSP_DIR)/src/%.c,$(BUILD_DIR)/lxr_dsp/%.o,$(LXR_DSP_SRCS))

LXR_CFLAGS    := -std=gnu11 -O3 -DNDEBUG -Wall -Wextra -fno-common \
                 -ffunction-sections -fdata-sections -fno-builtin \
                 -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 \
                 -I$(LXR_DSP_DIR)/include

CXXFLAGS      := -std=c++20 -includecstdint -felide-constructors -Wno-psabi \
                 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics \
                 -fcheck-new -Wall -Wextra -fno-builtin -fno-common \
                 -ffunction-sections -fdata-sections -ffreestanding \
                 -fsingle-precision-constant -ffast-math -O3 -DNDEBUG \
                 -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 \
                 -mfp16-format=ieee -mword-relocations \
                 -DPRODUCT=WAVE -DPRODUCT_WAVE \
                 -I$(SDK_DIR)/include \
                 -I$(LXR_DSP_DIR)/include \
                 -I$(REPO_ROOT)/synth/include \
                 -I$(REPO_ROOT)/synth/config \
                 -I$(REPO_ROOT)/util/include \
                 -I$(REPO_ROOT)/util/3rdparty \
                 -I$(REPO_ROOT)/thirdparty/boost/include \
                 -I$(REPO_ROOT)/thirdparty/enum_util/include

LDFLAGS       := -nostartfiles --specs=nano.specs \
                 -T$(SDK_DIR)/tools/psyn.ld \
                 -Wl,--gc-sections -Wl,--emit-relocs \
                 -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=calloc \
                 -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_calloc_r

all: $(BUILD_DIR)/CHIP.psyn $(BUILD_DIR)/LXRD.psyn

$(BUILD_DIR)/CHIP.elf: $(SDK_DIR)/engines/chip.cpp $(SDK_DIR)/include/psyn/psyn_plugin.h $(SDK_DIR)/include/psyn/psyn_cpp_bridge.h $(SDK_DIR)/tools/psyn.ld
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

HAS_SAMPLE_SLOTS ?= 0
SUPPORTS_IDLE_SLEEP ?= 1
HAS_VOICE_TICK_IDLE ?= 1

$(BUILD_DIR)/CHIP.psyn: $(BUILD_DIR)/CHIP.elf $(SDK_DIR)/tools/psyn_builder.py
	$(PYTHON) $(SDK_DIR)/tools/psyn_builder.py \
		--elf $< \
		--out $@ \
		--code-name "CHIP" \
		--display-name "Chiptune" \
		--patch-folder "CHIP" \
		--type-id-wave 12 \
		--type-id-play-tracker 8 \
		--default-voices 4 \
		--max-voices 8 \
		--output-channels 1 \
		--renders-aux 0 \
		--has-sample-slots $(HAS_SAMPLE_SLOTS) \
		--supports-idle-sleep $(SUPPORTS_IDLE_SLEEP) \
		--has-voice-tick-idle $(HAS_VOICE_TICK_IDLE)

HOST_CC       ?= gcc
HOST_CFLAGS   := -std=gnu11 -O2 -Wall -Wextra -I$(LXR_DSP_DIR)/include
HOST_TEST_BIN := $(BUILD_DIR)/host/lxr_drumvoice_smoke

host-test: $(HOST_TEST_BIN)
	$(HOST_TEST_BIN)

$(HOST_TEST_BIN): $(SDK_DIR)/tests/lxr_drumvoice_smoke.c $(LXR_DSP_SRCS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $^ -lm -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean host-test

$(BUILD_DIR)/lxr_dsp/%.o: $(LXR_DSP_DIR)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(LXR_CFLAGS) -c $< -o $@

$(BUILD_DIR)/LXRD.elf: $(SDK_DIR)/engines/lxr_drum_poc.cpp $(LXR_DSP_OBJS) $(SDK_DIR)/include/psyn/psyn_plugin.h $(SDK_DIR)/include/psyn/psyn_cpp_bridge.h $(SDK_DIR)/tools/psyn.ld
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SDK_DIR)/engines/lxr_drum_poc.cpp $(LXR_DSP_OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/LXRD.psyn: $(BUILD_DIR)/LXRD.elf $(SDK_DIR)/tools/psyn_builder.py
	$(PYTHON) $(SDK_DIR)/tools/psyn_builder.py \
		--elf $< \
		--out $@ \
		--code-name "LXRD" \
		--display-name "LXR Drum POC" \
		--patch-folder "LXRD" \
		--type-id-wave 0 \
		--type-id-play-tracker 0 \
		--default-voices 1 \
		--max-voices 1 \
		--output-channels 1 \
		--renders-aux 0 \
		--has-sample-slots 0 \
		--supports-idle-sleep 0 \
		--has-voice-tick-idle 0
