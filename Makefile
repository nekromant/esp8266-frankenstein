SRCDIR=antares
GITURL=https://nekromant@github.com/nekromant/antares.git
BRANCH=experimental
OBJDIR=.
TMPDIR=tmp
TOPDIR=.
project_sources=src
ANTARES_DIR:=./antares
ANTARES_INSTALL_DIR:=$(abspath ./antares)

CFLAGS+=-I$(abspath ./include/lwip-esp8266/)
CFLAGS+=-D__ets__ \
	-DICACHE_FLASH \
	-DLWIP_OPEN_SRC \
	-DPBUF_RSV_FOR_WLAN \
	-DEBUF_LWIP


ifeq ($(ANTARES_INSTALL_DIR2),)
antares:
	git clone $(GITURL) $(ANTARES_DIR) -b$(BRANCH)
	@echo "I have fetched the antares sources for you to $(ANTARES_DIR)"
	@echo "Please, re-run make"
else
antares:
	ln -sf $(ANTARES_INSTALL_DIR) $(ANTARES_DIR)
	@echo "Using global antares installation: $(ANTARES_INSTALL_DIR)"
	@echo "Symlinking done, please re-run make"
endif

-include antares/Makefile

# You can define any custom make rules right here!
# They will can be later hooked as default make target
# in menuconfig 


#GP0 - Loader GPIO
#GP1 - reset
#Press both.
#release reset.
#sleep 300 ms
#release boot
define tobootloader
	sudo pl2303gpio --gpio=0 --out=0 --gpio=1 --out=0 \
	--gpio=1 --in --sleep 50 \
	--gpio=0 --in
endef 

define reset
	sudo pl2303gpio --gpio=1 --out=0 --gpio=1 --in
endef

dumpiram:
	$(tobootloader)
	esptool.py --port /dev/ttyUSB0 dump_mem 0x40000000 65536 iram0.bin

reset: 
	$(reset)

PORT=/dev/ttyUSB0

flash:
	$(tobootloader)
	-esptool.py --port $(PORT) write_flash 0x00000 images/antares.rom
	$(reset)
	minicom -o -D $(PORT) -b 115200

flashidata:
	$(tobootloader)
	-esptool.py --port $(PORT) write_flash 0x7c000 esp_iot_sdk_v0.9.2/bin/esp_init_data_default.bin
	$(reset)
	minicom -o -D $(PORT) -b 115200
