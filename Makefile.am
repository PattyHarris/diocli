################################################################################
# diocli (top level Makefile)
################################################################################

# For libtool
ACLOCAL_AMFLAGS = -I m4

# adding doxygen support
include $(top_srcdir)/aminclude.am

# lib path
DIOCLI_LIB = $(top_srcdir)/Lib/Linux

# The subdirectories of the project to go into
SUBDIRS = \
	Include Include/ErrorCodes Include/curl Include/tclap \
	Util/Static Util/DLL \
	gSoap/Static gSoap/DLL \
	CPPSDK.Lib/Static CPPSDK.Lib/DLL \
	DioCLI CPPSDK.DLL

EXTRA_DIST = LICENSE

