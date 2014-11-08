SRCDIR=antares
GITURL=https://nekromant@github.com/nekromant/antares.git
OBJDIR=.
TMPDIR=tmp
TOPDIR=.
project_sources=src
ANTARES_DIR:=./antares

ifeq ($(ANTARES_INSTALL_DIR),)
antares:
	git clone $(GITURL) $(ANTARES_DIR)
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
	esptool.py --port /dev/ttyUSB1 dump_mem 0x40000000 65536 iram0.bin

reset: 
	$(reset)
flash:
	$(tobootloader)
	-esptool.py --port /dev/ttyUSB0 write_flash 0x00000 images/antares-0x00000.bin
	$(tobootloader)
	-esptool.py --port /dev/ttyUSB0 write_flash 0x40000 images/antares-0x40000.bin
	$(reset)
