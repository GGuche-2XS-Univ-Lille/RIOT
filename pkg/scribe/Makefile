PKG_NAME=scribe
PKG_URL=https://github.com/GGuche-2XS-Univ-Lille/scribe.git
PKG_VERSION=fe282b322756d6c34af84360569f8bbe8ef23196
PKG_LICENSE=MIT

include $(RIOTBASE)/pkg/pkg.mk

export RIOT_INCLUDES=$(INCLUDES)
export RIOT_CFLAGS=$(CFLAGS)

export DEBUG:=$(DEBUG)
export PREFIX:=$(PREFIX)
export CC:=$(CC)
export QUIET:=$(QUIET)
export LOG:=$(LOG)
export COAP:=$(COAP)

all: $(BINDIR)/scribe.a

$(BINDIR)/scribe.a:
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR) build-library
	$(QQ)cp $(PKG_SOURCE_DIR)/$(BOARD)/scribe.a $@

.PHONY: $(BINDIR)/scribe.a
