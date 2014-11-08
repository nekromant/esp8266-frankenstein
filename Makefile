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
