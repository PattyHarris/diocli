################################################################################
# DioCLI Makefile.am
################################################################################

EXTRA_DIST = Diomede.pem

bin_PROGRAMS = diocli
DIOCLI_LIB_DIR = $(top_srcdir)/Lib/Linux

diocli_SOURCES = \
$(top_srcdir)/DioCLI/stdafx.cpp \
$(top_srcdir)/DioCLI/stdafx.h \
$(top_srcdir)/DioCLI/resource.h \
$(top_srcdir)/DioCLI/ApplicationDefs.h \
$(top_srcdir)/DioCLI/BillingDefs.h \
$(top_srcdir)/DioCLI/CommandDefs.h \
$(top_srcdir)/DioCLI/ConsoleControl.cpp \
$(top_srcdir)/DioCLI/ConsoleControl.h \
$(top_srcdir)/DioCLI/DioCLI.cpp \
$(top_srcdir)/DioCLI/DiomedeCmdLine.h \
$(top_srcdir)/DioCLI/DiomedePEM.h \
$(top_srcdir)/DioCLI/DiomedeMultiArg.h \
$(top_srcdir)/DioCLI/DiomedeStdOut.h \
$(top_srcdir)/DioCLI/DiomedeSwitchArg.h \
$(top_srcdir)/DioCLI/DiomedeTask.cpp \
$(top_srcdir)/DioCLI/DiomedeTask.h \
$(top_srcdir)/DioCLI/DiomedeUnlabeledMultiArg.h \
$(top_srcdir)/DioCLI/DiomedeUnlabeledValueArg.h \
$(top_srcdir)/DioCLI/DiomedeValueArg.h \
$(top_srcdir)/DioCLI/Enum.h \
$(top_srcdir)/DioCLI/ResumeInfoData.cpp \
$(top_srcdir)/DioCLI/ResumeInfoData.h \
$(top_srcdir)/DioCLI/ResumeManager.cpp \
$(top_srcdir)/DioCLI/ResumeManager.h \
$(top_srcdir)/DioCLI/ResumeNamedMutex.h \
$(top_srcdir)/DioCLI/SimpleRedirect.cpp \
$(top_srcdir)/DioCLI/SimpleRedirect.h

diocli_CPPFLAGS = \
$(SSL_CXXFLAGS) -DCURL_STATICLIB -UWIN32 -U_WIN32 -UWINDOWS \
-I$(BOOST_CPPFLAGS)  \
-I$(top_srcdir)/Include/tclap \
-I$(top_srcdir)/Include/ErrorCodes \
-I$(top_srcdir)/Include \
-I$(top_srcdir)/CPPSDK.Lib \
-I$(top_srcdir)/Util \
-I$(top_srcdir)/gSoap \
-I$(top_srcdir)/DioCLI

diocli_LDFLAGS = @LDFLAGS@ -lpthread -lcurl -lssl -lcrypto

#====================================================================================
# Uncomment to link in the PEM file as an object.  Works in Linux, unproven on the 
# Mac.
# xDiomedePem = $(top_srcdir)/DioCLI/DiomedePem.o

# if USING_HOST_OSX
# xTargetFlag = macho-o-i386
# else
# xTargetFlag = elf32-i386
# endif

diocli_LDADD = \
	$(xDiomedePem) \
	-L/usr/lib \
	$(top_srcdir)/CPPSDK.Lib/Static/libCPPSDKMT.Lib.a \
	$(top_srcdir)/gSoap/Static/libgSoapMT.a \
	$(top_srcdir)/Util/Static/libUtilMT.a


#====================================================================================
# Uncomment to link in the PEM file as an object.  Works in Linux, unproven on the 
# Mac.
# $(xDiomedePem):
# 	@echo "Creating PEM object file with target...$(xTargetFlag)"
# 	objcopy --input-target binary \
# 		--output-target $(xTargetFlag) \
# 		--binary-architecture i386 Diomede.pem $(xDiomedePem)


