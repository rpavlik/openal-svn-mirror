# Build, install and distribute the following subdirectories:
SUBDIRS = admin src common include

# We keep our local autoconf macros in a separate 'm4'subdirectory.
ACLOCAL_AMFLAGS = -I admin/autotools/m4

# The following files should be in our source distribution in addition to the
# standard ones included by automake itself. A few remarks:
#
#  * As a temporary measure until the build system is fixed we mention a few
#    files which don't really belong here.
#
#  * We do not mention the doc and test subdirectories, they are really too much
#    out-of-date.
EXTRA_DIST =							\
	CMakeLists.txt						\
	NOTES							\
	admin/CMakeModules/CMakeASMCompiler.cmake.in		\
	admin/CMakeModules/CMakeASMInformation.cmake		\
	admin/CMakeModules/CMakeDetermineASMCompiler.cmake	\
	admin/CMakeModules/CMakeTestASMCompiler.cmake		\
	admin/CMakeModules/FindCompilerAttribute.cmake		\
	admin/CMakeModules/FindCompilerFlagsSet.cmake		\
	admin/CMakeModules/FindCompilerVisibility.cmake		\
	admin/CMakeModules/FindConfigHelper.cmake		\
        admin/CMakeModules/FindPkgConfig.cmake			\
        admin/CMakeModules/FindSelectArgtypes.cmake		\
	admin/CMakeModules/FindSleepFunction.cmake		\
	admin/RPM/openal.spec					\
	admin/RPM/openal.xpm					\
	admin/autotools/.indent.pro				\
	autogen.sh						\
	common/specification/OpenAL1-1Spec.pdf			\
	common/specification/OpenAL1-1Spec.sxw			\
	config-osx.h

# Automatically rebuild libtool if it becomes out-of-date.
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

# The Creative repository has some broken permissions, let's fix this.
permissions:
	find . -type f \! -perm +222 \! -name "*.sh" -exec chmod +w {} \;

# Automatically reformat all C headers and sources with indent according to
# admin/autotools/.indent.pro, but be make-friendly and take some precautions to
# leave those files untouched which wouldn't change. Do not reformat the public
# AL extension header, it won't look the way we want it due to the 'extern "C" {
# ... }' wrapper in it.
indent:
	@for i in `find . \( -name "*.c" -o -name "*.h" \) \! -name alut.h -print` ; do \
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
