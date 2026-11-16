# Deltarune Chapter 1 - Nintendo DS Port
# Built with BlocksDS Core SDK

BLOCKSDS ?= /opt/blocksds/core
ARM_NONE_EABI_PATH ?= /opt/arm-none-eabi/bin/

NAME := Deltarune
GAME_TITLE := Deltarune Chapter 1
GAME_SUBTITLE := Nintendo DS Port
GAME_AUTHOR := Toby Fox / DS Port
GAME_ICON := icon.gif

SOURCEDIRS := source
LIBS := -lnds9 -lm

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
