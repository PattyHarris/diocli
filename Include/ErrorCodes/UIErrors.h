
#ifndef UIERRORS_H
#define UIERRORS_H

#include "../../Util/ErrorType.h"

ErrorCode const UI_COMP                     = UI_COMPONENT_ERROR;

ErrorCode const UI_UNKNOWN_ERROR			= UI_COMPONENT_ERROR;

ErrorCode const UI_FILE_GENERIC             = UI_COMPONENT_ERROR | 0x01;
ErrorCode const UI_FILE_NOT_FOUND			= UI_COMPONENT_ERROR | 0x02;
ErrorCode const UI_FILE_BAD_PATH		    = UI_COMPONENT_ERROR | 0x03;
ErrorCode const UI_FILE_TOO_MANY_OPEN_FILES	= UI_COMPONENT_ERROR | 0x04;
ErrorCode const UI_FILE_ACCESS_DENIED       = UI_COMPONENT_ERROR | 0x05;
ErrorCode const UI_INVALID_FILE             = UI_COMPONENT_ERROR | 0x06;
ErrorCode const UI_FILE_REMOVE_CURRENT_DIR  = UI_COMPONENT_ERROR | 0x07;
ErrorCode const UI_FILE_DIRECTORY_FULL      = UI_COMPONENT_ERROR | 0x08;
ErrorCode const UI_FILE_BAD_SEEK            = UI_COMPONENT_ERROR | 0x09;
ErrorCode const UI_FILE_HARD_IO             = UI_COMPONENT_ERROR | 0x0A;
ErrorCode const UI_FILE_SHARING_VIOLATION   = UI_COMPONENT_ERROR | 0x0B;
ErrorCode const UI_FILE_LOCK_VIOLATION      = UI_COMPONENT_ERROR | 0x0C;
ErrorCode const UI_DISK_FULL                = UI_COMPONENT_ERROR | 0x0D;
ErrorCode const UI_END_OF_FILE              = UI_COMPONENT_ERROR | 0x0E;

ErrorCode const UI_UNKNOWN_USER             = UI_COMPONENT_ERROR | 0x0F;
ErrorCode const UI_WINDOWS_GENERIC          = UI_COMPONENT_ERROR | 0x10;

ErrorCode const UI_COULD_NOT_LOAD_DCF       = UI_COMPONENT_ERROR | 0x11;
ErrorCode const UI_INVALID_USERNAME_OR_PWD  = UI_COMPONENT_ERROR | 0x12;
ErrorCode const UI_SERVER_NOT_AVAILABLE     = UI_COMPONENT_ERROR | 0x13;
ErrorCode const UI_SERVER_CONNECTION        = UI_COMPONENT_ERROR | 0x14;

#endif  /* UIERRORS_H */

