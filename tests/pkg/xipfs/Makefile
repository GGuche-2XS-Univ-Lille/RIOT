include ../Makefile.pkg_common

BOARD ?= dwm1001

SLOT_AUX_LEN := 0x8000

USEPKG += xipfs
USEMODULE += xipfs
USEMODULE += saul_default

TOOLCHAINS_BLACKLIST += llvm

# XIPFS MPU support
CFLAGS += -DXIPFS_ENABLE_SAFE_EXEC_SUPPORT
# XIPFS MPU ISR stack requirement
CFLAGS += -DISR_STACKSIZE=1024

BLOBS += minimal.fae
BLOBS += nvme0p1.flash

include $(RIOTBASE)/Makefile.include
