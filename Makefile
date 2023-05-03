# Project Name
TARGET = big_knob

# Sources
CPP_SOURCES = big_knob.cpp

# Library Locations
LIBDAISY_DIR = /Users/ansel/Documents/DaisyExamples/libDaisy/
DAISYSP_DIR = /Users/ansel/Documents/DaisyExamples/DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# enable float printing
LDFLAGS += -u _printf_float