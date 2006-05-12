# Build, install and distribute the following subdirectories:
SUBDIRS = demos examples tests

# We keep our local autoconf macros in a separate 'm4'subdirectory.
ACLOCAL_AMFLAGS = -I admin/autotools/m4

# The following files should be in our source distribution in addition to the
# standard ones included by automake itself.
EXTRA_DIST =					\
	admin/autotools/.indent.pro		\
	autogen.sh				\
	tests/altest/gendocs.py			\
	tests/altest/sounds/README		\
	tests/altest/sounds/boom.ogg		\
	tests/altest/sounds/ding.wav		\
	tests/altest/sounds/footadpcm.wav	\
	tests/altest/sounds/footsteps.wav	\
	tests/altest/sounds/stereo.wav		\
	tests/altest/sounds/wave1.wav		\
	tests/altest/sounds/wave2.wav		\
	tests/altest/sounds/wave3.wav		\
	tests/altest/sounds/wave4.wav

# Automatically rebuild libtool if it becomes out-of-date.
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

# The Creative repository has some broken permissions, let's fix this.
permissions:
	find . -type f \! -perm +222 \! -name "*.sh" -exec chmod +w {} \;

# Automatically reformat all C headers and sources with indent according to
# admin/autotools/.indent.pro, but be make-friendly and take some precautions to
# leave those files untouched which wouldn't change.
indent:
	@for i in `find . \( -name "*.c" -o -name "*.h" \) -print` ; do \
	    cat $$i | ( cd admin/autotools ; indent ) > indent.tmp ; \
	    if diff $$i indent.tmp > /dev/null 2>&1 ; then \
		: ; \
	    else \
		echo Reformatted $$i ; \
		$(RM) $$i ; \
		mv indent.tmp $$i ; \
	    fi ; \
	done
	@$(RM) indent.tmp

# Targets which are no files
.PHONY: permissions indent
