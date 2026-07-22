# name of your application
APPLICATION = xipfs_coap_fileserver

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += shell_cmds_default

# enable the fileserver module
USEMODULE += gcoap_fileserver
USEMODULE += nanocoap_fileserver_callback
USEMODULE += nanocoap_fileserver_delete
USEMODULE += nanocoap_fileserver_put

# select network modules
USEMODULE += gnrc_ipv6_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += netdev_default
USEMODULE += gnrc_icmpv6_echo

PREFIX_CONF ?= uhcp

USEMODULE += stdio_ethos

# include UHCP client
USEMODULE += gnrc_uhcpc

USEMODULE += cortexm_svc

FEATURES_REQUIRED += cortexm_mpu

USEMODULE += saul_default

# Use xipfs file system
USEMODULE += xipfs

# XIPFS MPU support
CFLAGS += -DXIPFS_ENABLE_SAFE_EXEC_SUPPORT
# XIPFS MPU ISR stack requirement
CFLAGS += -DISR_STACKSIZE=1024

CFLAGS += -DXIPFS_COAP_CREATE_EXE_ENABLED

BLOBS += nvme0p0.flash

# XIPFS is currently not compatible with the llvm toolchain
TOOLCHAINS_BLACKLIST += llvm


# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Ethos/native TAP interface and UHCP prefix can be configured from make command
TAP ?= tap0
# UHCP advertises the prefix as is.
IPV6_PREFIX ?= 2001:db8::/64

# MODULE DEPENDENT CONFIGURATION IMPORT
# =====================================
ETHOS_BAUDRATE ?= 115200
include $(CURDIR)/Makefile.ethos.conf


# As there is an 'Kconfig' we want to explicitly disable Kconfig by setting
# the variable to empty
SHOULD_RUN_KCONFIG ?=

include $(RIOTBASE)/Makefile.include
