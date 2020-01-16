PROGRAM = garage

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/dhcpserver \
	$(abspath components/wifi-config) \
	$(abspath components/cJSON) \
	$(abspath components/wolfssl) \
	$(abspath components/homekit) \
	$(abspath components/esp-button)

FLASH_MODE ?= dout

TOP_PIN ?= 13
BOTTOM_PIN ?= 12
RELAY_PIN ?= 5

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS -DTOP_PIN=$(TOP_PIN) -DBOTTOM_PIN=$(BOTTOM_PIN) -DRELAY_PIN=$(RELAY_PIN)


include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
