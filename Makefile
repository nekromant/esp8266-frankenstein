SRCDIR=antares
GITURL=https://nekromant@github.com/nekromant/antares.git
BRANCH=experimental
OBJDIR=.
TMPDIR=tmp
TOPDIR=.
project_sources=src
ANTARES_DIR:=./antares
ANTARES_INSTALL_DIR=$(ANTARES_DIR)

CFLAGS+=-I$(abspath ./include/lwip-esp8266/)
CFLAGS+=-D__ets__ \
	-DICACHE_FLASH \
	-DLWIP_OPEN_SRC \
	-DPBUF_RSV_FOR_WLAN \
	-DEBUF_LWIP


ifeq ($(ANTARES_INSTALL_DIR),)
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

ditch_that_icache_flash_attr:
	for f in `find src/ -type f `; do \
	sed -i 's/ICACHE_FLASH_ATTR//g' $$f;\
	done

defconfig:
	@cp configs/config_default .config
	@echo "Reverting to default config"

update:
	git fetch && git rebase -i
	cd antares && git fetch && git rebase -i

