PKG_NAME=xipfs
PKG_URL=https://github.com/2xs/xipfs.git
PKG_VERSION=c81659c6059f0e94b0ee3e9f77b80f19a8e857f5
PKG_LICENSE=CeCILL-2.1

include $(RIOTBASE)/pkg/pkg.mk

CFLAGS += -DXIPFS_ENABLE_SAFE_EXEC_SUPPORT

export RIOT_INCLUDES=$(INCLUDES)
export RIOT_CFLAGS=$(CFLAGS)

all: $(BINDIR)/xipfs.a

$(BINDIR)/xipfs.a:
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR)
	cp $(PKG_SOURCE_DIR)/$(BOARD)/xipfs.a $@

.PHONY: $(BINDIR)/xipfs.a
