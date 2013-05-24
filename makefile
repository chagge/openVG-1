APPNAME			= rodvg
DESTDIR			= .
ROOTFS=/home/rod/bsp/linux/MX6Q_110/ltib/rootfs
CROSS_COMPILE=/opt/ER110/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-none-linux-gnueabi-

# Make command to use for dependencies
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

TARGET_PATH_LIB = $(ROOTFS)/usr/lib
TARGET_PATH_INCLUDE = $(ROOTFS)/usr/include
COMMON_DIR=../common

BIN_TUT = $(DESTDIR)/$(APPNAME)

CFLAGS = -DDEBUG -D_DEBUG -D_GNU_SOURCE -mfloat-abi=softfp -mfpu=neon -fPIC -O3 -fno-strict-aliasing -fno-optimize-sibling-calls -Wall -g

CFLAGS_TUT = $(CFLAGS) $(CFLAGS_EGL)

CFLAGS_TUT += \
    -DLINUX \
	-DEGL_API_FB \

CFLAGS_TUT += \
	-I$(TARGET_PATH_INCLUDE) \
	-I$(COMMON_DIR)/inc \
	

OBJECTS_TUT += \
	rodvg.o test_tiger_paths.o \
	$(COMMON_DIR)/src/fsl_egl.o

DEPS_TUT = -lstdc++ -lOpenVG_3D -lEGL -lm -ldl -Wl,--library-path=$(TARGET_PATH_LIB),-rpath-link=$(TARGET_PATH_LIB)

$(BIN_TUT) : $(OBJECTS_TUT)
	@echo " LD " $@
	$(QUIET)$(CC) -o $(BIN_TUT) $(OBJECTS_TUT) $(DEPS_TUT)

%.o : %.c
	@echo " CC " $@
	$(QUIET)$(CC) $(CFLAGS_TUT) -MMD -c $< -o $@

%.o : %.cpp
	@echo " CXX " $@
	$(QUIET)$(CC) $(CFLAGS_TUT) -MMD -c $< -o $@

clean:
	rm -f $(OBJECTS_TUT) $(OBJECTS_TUT:.o=.d) $(BIN_TUT)
	
install:
	cp -f $(APPNAME) $(DESTDIR)/.
	
	
-include $(OBJECTS_TUT:.o=.d)
