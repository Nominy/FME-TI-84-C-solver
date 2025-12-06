# ----------------------------
# Makefile Options
# ----------------------------

NAME = FME
DESCRIPTION = "FME solver CE"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

CEDEV_TOOLCHAIN := C:/Users/User/Desktop/dev/FME-ti84/CSetup
COMMENT :=

include $(CEDEV_TOOLCHAIN)/meta/makefile.mk

