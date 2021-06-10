################################################################################
# Util Makefile.am (Static)
################################################################################

noinst_LIBRARIES = libUtilMT.a

libUtilMT_a_CPPFLAGS = \
-DWITH_OPENSSL -UWIN32 -U_WIN32 -UWINDOWS \
-I$(top_srcdir)/Include \
-I$(top_srcdir)/Include/ErrorCodes \
-I$(BOOST_CPPFLAGS) 

libUtilMT_a_SOURCES = \
$(top_srcdir)/Util/BlowFish.cpp \
$(top_srcdir)/Util/Blowfish.h \
$(top_srcdir)/Util/BuildVersionUtils.cpp \
$(top_srcdir)/Util/ClientLog.cpp \
$(top_srcdir)/Util/ClientLog.h \
$(top_srcdir)/Util/ClientLogUtils.cpp \
$(top_srcdir)/Util/ClientLogUtils.h \
$(top_srcdir)/Util/CriticalSection.h \
$(top_srcdir)/Util/CustomMutex.h \
$(top_srcdir)/Util/ErrorType.cpp \
$(top_srcdir)/Util/ErrorType.h \
$(top_srcdir)/Util/EventClass.cpp \
$(top_srcdir)/Util/EventClass.h \
$(top_srcdir)/Util/FileLogger.cpp \
$(top_srcdir)/Util/FileLogger.h \
$(top_srcdir)/Util/ILogObserver.h \
$(top_srcdir)/Util/MemLogger.cpp \
$(top_srcdir)/Util/MemLogger.h \
$(top_srcdir)/Util/MessageTimer.cpp \
$(top_srcdir)/Util/MessageTimer.h \
$(top_srcdir)/Util/MutexClass.cpp \
$(top_srcdir)/Util/MutexClass.h \
$(top_srcdir)/Util/ProfileManager.cpp \
$(top_srcdir)/Util/ProfileManager.h \
$(top_srcdir)/Util/Stdafx.h \
$(top_srcdir)/Util/StringUtil.cpp \
$(top_srcdir)/Util/StringUtil.h \
$(top_srcdir)/Util/Thread.cpp \
$(top_srcdir)/Util/Thread.h \
$(top_srcdir)/Util/UserProfileData.cpp \
$(top_srcdir)/Util/UserProfileData.h \
$(top_srcdir)/Util/Util.cpp \
$(top_srcdir)/Util/Util.h \
$(top_srcdir)/Util/XString.cpp \
$(top_srcdir)/Util/XString.h \
$(top_srcdir)/Util/configure.cpp \
$(top_srcdir)/Util/configure.h \
$(top_srcdir)/Util/dictionary.h


