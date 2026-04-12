# Project Name
TARGET = earth

CPP_STANDARD = -std=c++20

#APP_TYPE = BOOT_SRAM

USE_DAISYSP_LGPL = 1

# Compiler options
#OPT = -O1
OPT=-Ofast

# Sources
CPP_SOURCES = earth.cpp
CPP_SOURCES += Dattorro/dsp/filters/OnePoleFilters.cpp
CPP_SOURCES += Dattorro/dsp/delays/InterpDelay.cpp
CPP_SOURCES += Dattorro/Dattorro.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

WEB_GOALS := web-main web-octave web-all
REQUESTED_WEB_GOALS := $(filter $(WEB_GOALS),$(MAKECMDGOALS))

ifneq ($(strip $(REQUESTED_WEB_GOALS)),)
$(info [EarthPedal] Skipping firmware toolchain include for web-only target(s): $(REQUESTED_WEB_GOALS))
else
include $(SYSTEM_FILES_DIR)/Makefile
endif

C_INCLUDES += -I../../q/q_lib/include
C_INCLUDES += -I../../gcem/include
C_INCLUDES += -I../../infra/include

# Include funbox.h
C_INCLUDES += -I../../include


.PHONY: web-main web-octave web-all

web-main:
	bash src/makefile_wasm

web-octave:
	bash src/makefile_wasm_octave

web-all: web-main web-octave
