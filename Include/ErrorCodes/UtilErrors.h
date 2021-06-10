#ifndef UtilErrors_H
#define UtilErrors_H

#include "../../Util/ErrorType.h"

#define UTIL_COMP UTIL_COMPONENT_ERROR

ErrorCode const UTIL_UNKNOWN_ERROR          	= UTIL_COMPONENT_ERROR;
ErrorCode const ERR_PROFILE_ALREADY_EXIST   	= UTIL_COMPONENT_ERROR | 0x01;
ErrorCode const ERR_PROFILE_NO_NAME         	= UTIL_COMPONENT_ERROR | 0x02;
ErrorCode const ERR_PROFILE_CANT_MAP_NAME   	= UTIL_COMPONENT_ERROR | 0x03;
ErrorCode const ERR_RESULT_POINTER_NULL     	= UTIL_COMPONENT_ERROR | 0x04;
ErrorCode const ERR_USERID_POINTER_NULL     	= UTIL_COMPONENT_ERROR | 0x05;
ErrorCode const ERR_NO_USER_DATA            	= UTIL_COMPONENT_ERROR | 0x06;
ErrorCode const ERR_NULL_USERID             	= UTIL_COMPONENT_ERROR | 0x07;
ErrorCode const ERR_INVALID_OBJECT_STATE		= UTIL_COMPONENT_ERROR | 0x08;
ErrorCode const ERR_CONFIG_FILE_NOT_SUPPORTED	= UTIL_COMPONENT_ERROR | 0x09;

#endif  /* UtilErrors_H */

