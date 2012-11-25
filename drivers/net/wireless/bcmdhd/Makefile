# bcmdhd
SRCROOT = drivers/net/wireless/bcmdhd/src
DHDCFLAGS = -Wall -Wstrict-prototypes -Werror -Dlinux -DBCMDRIVER             \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DWLBTAMP -DBCMFILEIMAGE  \
	-DDHDTHREAD -DDHD_GPL -DDHD_SCHED -DDHD_DEBUG -DSDTEST -DBDC -DTOE    \
	-DDHD_BCMEVENTS -DSHOW_EVENTS -DDONGLEOVERLAYS -DBCMDBG               \
	-DMMC_SDIO_ABORT -DBCMSDIO -DBCMLXSDMMC -DBCMPLATFORM_BUS -DWLP2P     \
	-DNEW_COMPAT_WIRELESS -DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT          \
	-DKEEP_ALIVE -DCSCAN -DPKT_FILTER_SUPPORT -DGET_CUSTOM_MAC_ENABLE     \
	-DEMBEDDED_PLATFORM -DENABLE_INSMOD_NO_FW_LOAD -DPNO_SUPPORT          \
	-DHW_OOB -DOOB_INTR_ONLY                                              \
	-I$(SRCROOT) \
	-I$(SRCROOT)/include \
	-I$(SRCROOT)/shared \
	-I$(SRCROOT)/dhd/sys \
	-I$(SRCROOT)/dongle \
	-I$(SRCROOT)/wl/sys \
	-I$(SRCROOT)/wl/bcmwifi/src \
	-I$(SRCROOT)/wl/bcmwifi/include

DHDCFLAGS += -DCONFIG_COMMON_PATCH -DDHD_USE_IDLECOUNT
DHDCFLAGS += -DBCM4330_CHIP

ifeq ($(CONFIG_WIFI_CONTROL_FUNC),y)
DHDCFLAGS += -DCONFIG_WIFI_CONTROL_FUNC -DCUSTOMER_HW2
endif

# For CCX
ifeq ($(CONFIG_BRCM_CCX),y)
DHDCFLAGS += -DBCMCCX
endif

# Pantech patch
DHDCFLAGS += -DCUSTOMER_HW_PANTECH

DHDOFILES = \
	src/bcmsdio/sys/bcmsdh.o \
	src/bcmsdio/sys/bcmsdh_linux.o \
	src/bcmsdio/sys/bcmsdh_sdmmc.o \
	src/bcmsdio/sys/bcmsdh_sdmmc_linux.o \
	src/dhd/sys/dhd_bta.o \
	src/dhd/sys/dhd_cdc.o \
	src/dhd/sys/dhd_common.o \
	src/dhd/sys/dhd_custom_gpio.o \
	src/dhd/sys/dhd_linux.o \
	src/dhd/sys/dhd_linux_sched.o \
	src/dhd/sys/dhd_sdio.o \
	src/shared/aiutils.o \
	src/shared/bcmutils.o   \
	src/shared/bcmevent.o \
	src/shared/hndpmu.o  \
	src/shared/linux_osl.o	\
	src/shared/sbutils.o \
	src/shared/siutils.o \
	src/wl/sys/wl_android.o	\
	src/wl/sys/wldev_common.o \
	src/wl/sys/wl_linux_mon.o \
	src/wl/bcmwifi/src/bcmwifi_channels.o 

obj-$(CONFIG_BCMDHD) += bcmdhd.o
bcmdhd-objs += $(DHDOFILES)
ifneq ($(CONFIG_WIRELESS_EXT),)
bcmdhd-objs += src/wl/sys/wl_iw.o
DHDCFLAGS += -DSOFTAP
endif
ifneq ($(CONFIG_CFG80211),)
bcmdhd-objs += \
	src/wl/sys/wl_cfg80211.o \
	src/wl/sys/wl_cfgp2p.o \
	src/dhd/sys/dhd_cfg80211.o
DHDCFLAGS += -DWL_CFG80211
endif
EXTRA_CFLAGS = $(DHDCFLAGS)
ifeq ($(CONFIG_BCMDHD),m)
EXTRA_LDFLAGS += --strip-debug
endif

