/*********************************************************************
 *
 *  file:  CommandDefs.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 *
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Command Constant Definitions
 *
 *********************************************************************/

#ifndef __COMMAND_DEFS_H__
#define __COMMAND_DEFS_H__

using namespace std;

#include <string>
#include <vector>
#include <list>
#include "../Util/XString.h"
#include <iostream>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

static const string NO_HELP	            = _T("");
const string CMD_EXIT                   = _T("exit");
const string CMD_MENU                   = _T("menu");
const string CMD_HELP                   = _T("help");
const string CMD_HELP_ALT1              = _T("?");
const string CMD_ABOUT                  = _T("about");
const string CMD_ABOUT_ALT1             = _T("ver");
const string CMD_STATUS                 = _T("status");

const string CMD_LAST                   = _T("last");
const string GROUP_LAST                 = _T("last");
const string ARG_LAST                   = _T("last");

// Common arguments and switches
const string ARG_FILENAME               = _T("filename");
const string ARG_FILEID                 = _T("fileid");
const string ARG_STORAGE_TYPE           = _T("storage");

const string ARG_HASHMD5                = _T("hashmd5");
const string ARG_HASHMD5_SWITCH         = _T("md5");
const string ARG_HASHSHA1               = _T("hashsha1");
const string ARG_HASHSHA1_SWITCH        = _T("sha1");

const string ARG_STARTDATE              = _T("startdate");
const string ARG_ENDDATE                = _T("enddate");
const string ARG_IP                     = _T("ip");
const string ARG_TOKEN                  = _T("token");

const string ARG_ON                     = _T("on");
const string ARG_OFF                    = _T("off");

const string ARG_YES                    = _T("yes");
const string ARG_NO                     = _T("no");
const string ARG_ALL                    = _T("all");

const string ARG_OUTPUT                 = _T("output");
const string ARG_TEST_SWITCH            = _T("test");

// Can be either filename, fileid, or hash value
const string ARG_FILEINFO               = _T("fileinfo");

// Comands that do not require authentication
const string CMD_CREATEUSER	            = _T("createuser");
const string CMD_CREATEUSER_ALT1        = _T("cu");

const string CMD_CHANGEPASSWORD	        = _T("changepassword");
const string CMD_CHANGEPASSWORD_ALT1    = _T("password");
const string CMD_CHANGEPASSWORD_ALT2    = _T("cp");
const string ARG_OLDPASSWORD            = _T("oldpassword");
const string ARG_NEWPASSWORD            = _T("newpassword");

const string CMD_RESETPASSWORD	        = _T("resetpassword");
const string CMD_RESETPASSWORD_ALT1     = _T("resetpass");
const string CMD_RESETPASSWORD_ALT2     = _T("rp");

const string ARG_USERNAME               = _T("username");
const string ARG_PASSWORD               = _T("password");
const string ARG_EMAIL                  = _T("email");

const string ARG_FIRSTNAME              = _T("firstname");
const string ARG_LASTNAME               = _T("lastname");
const string ARG_COMPANY                = _T("company");
const string ARG_WEBURL                 = _T("weburl");
const string ARG_PHONE                  = _T("phone");

const string CMD_LOGIN		            = _T("login");
const string CMD_LOGIN_ALT1             = _T("li");
const string ARG_LOGIN_USERNAME	        = _T("username");
const string ARG_LOGIN_PASSWORD         = _T("password");

const string CMD_LOGOUT                 = _T("logout");
const string CMD_LOGOUT_ALT1            = _T("lo");

const string CMD_SETCONFIG              = _T("setconf");
const string ARG_AUTOLOGIN              = _T("autologin");
const string ARG_AUTOLOGOUT             = _T("autologout");
const string ARG_CHECK_ACCOUNT          = _T("checkaccount");
const string ARG_RESULT_PAGE_SIZE       = _T("page");
const string ARG_RESULT_OFFSET          = _T("offset");
const string ARG_USE_SSL                = _T("ssl");
const string ARG_SERVICE_ENDPOINT       = _T("service");
const string ARG_SSL_SERVICE_ENDPOINT   = _T("sslservice");
const string ARG_TRANSFER_ENDPOINT      = _T("transfer");
const string ARG_SSL_TRANSFER_ENDPOINT  = _T("ssltransfer");
const string ARG_CLIPBOARD              = _T("clipboard");
const string ARG_VERBOSE                = _T("verbose");
const string ARG_VERBOSE_SWITCH         = _T("v");

const string CMD_SESSIONTOKEN           = _T("sessiontoken");
const string CMD_SESSIONTOKEN_ALT1      = _T("session");
const string CMD_SESSIONTOKEN_ALT2      = _T("token");

const string CMD_DELETEUSER             = _T("deleteuser");
const string CMD_DELETEUSER_ALT1        = _T("du");

const string CMD_UPLOAD                 = _T("upload");
const string CMD_UPLOAD_ALT1            = _T("up");
const string CMD_UPLOAD_ALT2            = _T("u");
const string ARG_RECURSE_SWITCH         = _T("s");
const string ARG_PATHMETADATA_SWITCH    = _T("m");

const string CMD_RESUME                 = _T("resume");
const string CMD_RESUME_ALT1            = _T("res");
const string CMD_RESUME_ALT2            = _T("r");
const string ARG_RESUME_LIST_SWITCH     = _T("list");
const string ARG_RESUME_CLEAR_SWITCH    = _T("clear");
const string ARG_RESUME_ALL             = _T("all");
const string ARG_RESUME_COMPLETE        = _T("complete");
const string ARG_RESUME_INCOMPLETE      = _T("incomplete");

const string CMD_DOWNLOAD               = _T("download");
const string CMD_DOWNLOAD_ALT1          = _T("down");
const string CMD_DOWNLOAD_ALT2          = _T("d");
const string ARG_DIRECTORY              = _T("directory");

const string CMD_GETUPLOADTOKEN         = _T("getuploadtoken");
const string CMD_GETUPLOADTOKEN_ALT1    = _T("gettoken");
const string CMD_GETUPLOADTOKEN_ALT2    = _T("getut");

const string ARG_TOKEN_BYTES            = _T("bytes");
const string ARG_TOKEN_CALLBACK         = _T("callback");

const string CMD_GETDOWNLOADURL         = _T("getdownloadurl");
const string CMD_GETDOWNLOADURL_ALT1    = _T("geturl");
const string CMD_GETDOWNLOADURL_ALT2    = _T("url");

const string ARG_MAXDOWNLOADS           = _T("maxdownloads");
const string ARG_LIFETIMEHOURS          = _T("lifetimehrs");
const string ARG_MAXUNIQUEIPS           = _T("maxuniqueips");
const string ARG_ERRORREDIRECT          = _T("errorredirect");

const string CMD_DELETEFILE             = _T("delete");
const string CMD_DELETEFILE_ALT1        = _T("del");
const string CMD_DELETEFILE_ALT2        = _T("rm");
const string CMD_DELETEFILE_ALT3        = _T("erase");

const string CMD_UNDELETEFILE           = _T("undelete");
const string CMD_UNDELETEFILE_ALT1      = _T("undel");

const string CMD_RENAMEFILE             = _T("rename");
const string CMD_RENAMEFILE_ALT1        = _T("ren");
const string CMD_RENAMEFILE_ALT2        = _T("mv");

const string CMD_DISPLAYFILE            = _T("type");
const string CMD_DISPLAYFILE_ALT1       = _T("cat");
const string ARG_STARTBYTE              = _T("startbyte");
const string ARG_ENDBYTE                = _T("endbyte");

const string CMD_CREATEMETADATA           = _T("createmetadata");
const string CMD_CREATEMETADATA_ALT1      = _T("createmeta");
const string CMD_CREATEMETADATA_ALT2      = _T("createm");

const string CMD_CREATEFILEMETADATA       = _T("createfilemetadata");
const string CMD_CREATEFILEMETADATA_ALT1  = _T("createfmeta");
const string CMD_CREATEFILEMETADATA_ALT2  = _T("createfm");

const string CMD_SETFILEMETADATA        = _T("setfilemetadata");
const string CMD_SETFILEMETADATA_ALT1   = _T("setfmeta");
const string CMD_SETFILEMETADATA_ALT2   = _T("setfm");

const string ARG_METANAME               = _T("name");
const string ARG_METAVALUE              = _T("value");
const string ARG_METADATAID             = _T("mid");

const string CMD_GETMETADATA            = _T("getmetadata");
const string CMD_GETMETADATA_ALT1       = _T("getm");

const string CMD_GETFILEMETADATA        = _T("getfilemetadata");
const string CMD_GETFILEMETADATA_ALT1   = _T("getfmeta");
const string CMD_GETFILEMETADATA_ALT2   = _T("getfm");

const string CMD_DELETEFILEMETADATA     = _T("deletefilemetadata");
const string CMD_DELETEFILEMETADATA_ALT1= _T("delfm");

const string CMD_DELETEMETADATA         = _T("deletemetadata");
const string CMD_DELETEMETADATA_ALT1    = _T("delm");
const string CMD_DELETEMETADATA_ALT2    = _T("deletefm");

const string CMD_EDITMETADATA           = _T("editmetadata");
const string CMD_EDITMETADATA_ALT1      = _T("editm");

const string CMD_REPLICATEFILE          = _T("replicate");
const string CMD_REPLICATEFILE_ALT1     = _T("rep");
const string ARG_EXPIRATION_DATE        = _T("expires");

const string CMD_UNREPLICATEFILE        = _T("unreplicate");
const string CMD_UNREPLICATEFILE_ALT1   = _T("unrep");
const string CMD_GETSTORAGETYPES        = _T("getstoragetypes");
const string CMD_GETSTORAGETYPES_ALT1   = _T("getsttypes");
const string CMD_GETPHYSICALFILES       = _T("getphysicalfiles");
const string CMD_GETPHYSICALFILES_ALT1  = _T("getpf");
const string ARG_PHYSICALFILE_ID        = _T("pf");

const string CMD_CREATEREPLICATIONPOLICY        = _T("createrp");
const string CMD_CREATEREPLICATIONPOLICY_ALT1   = _T("createpolicy");

const string ARG_REPLICATION_POLICY_ID          = _T("policyid");
const string ARG_DEF_ONLINE                     = _T("online");
const string ARG_DEF_NEARLINE                   = _T("nearline");
const string ARG_DEF_OFFLINE                    = _T("offline");
const string ARG_TRIGGER_HOURS                  = _T("trighrs");
const string ARG_TRIGGER_ONLINE                 = _T("trigonline");
const string ARG_TRIGGER_NEARLINE               = _T("trignearline");
const string ARG_TRIGGER_OFFLINE                = _T("trigoffline");
const string ARG_EXPIRE_HOURS                   = _T("expirehrs");

const string CMD_GETREPLICATIONPOLICIES         = _T("getrps");
const string CMD_GETREPLICATIONPOLICIES_ALT1    = _T("getpolicies");

const string CMD_EDITREPLICATIONPOLICY          = _T("editrp");
const string CMD_EDITREPLICATIONPOLICY_ALT1     = _T("editpolicy");

const string CMD_DELETEREPLICATIONPOLICY        = _T("deleterp");
const string CMD_DELETEREPLICATIONPOLICY_ALT1   = _T("deletepolicy");

const string CMD_SETREPLICATIONPOLICY           = _T("setrp");
const string CMD_SETREPLICATIONPOLICY_ALT1      = _T("setpolicy");

const string CMD_SETDEFREPLICATIONPOLICY        = _T("setdefaultrp");
const string CMD_SETDEFREPLICATIONPOLICY_ALT1   = _T("setdefrp");
const string CMD_SETDEFREPLICATIONPOLICY_ALT2   = _T("setdefaultpolicy");

const string CMD_GETDEFREPLICATIONPOLICY        = _T("getdefaultrp");
const string CMD_GETDEFREPLICATIONPOLICY_ALT1   = _T("getdefrp");
const string CMD_GETDEFREPLICATIONPOLICY_ALT2   = _T("getdefaultpolicy");

const string CMD_GETFILEINFO            = _T("getfileinfo");

const string CMD_SETUSERINFO            = _T("setuserinfo");
const string CMD_SETUSERINFO_ALT1       = _T("setui");

const string CMD_GETUSERINFO            = _T("getuserinfo");
const string CMD_GETUSERINFO_ALT1       = _T("getui");

const string CMD_DELETEUSERINFO         = _T("deleteuserinfo");
const string CMD_DELETEUSERINFO_ALT1    = _T("delui");
const string ARG_DELETEUSERINFO         = _T("key");

const string CMD_GETEMAILADDRESSES      = _T("getallemails");
const string CMD_GETEMAILADDRESSES_ALT1 = _T("getemail");

const string CMD_ADDEMAILADDRESS        = _T("addemail");

const string CMD_DELETEEMAILADDRESS             = _T("deleteemail");
const string CMD_DELETEEMAILADDRESS_ALT1        = _T("delemail");

const string CMD_SETPRIMARYEMAILADDRESS         = _T("setprimaryemail");
const string CMD_SETPRIMARYEMAILADDRESS_ALT1    = _T("setemail");

const string CMD_CHECKACCOUNT           = _T("checkaccount");
const string CMD_CHECKACCOUNT_ALT1      = _T("checkacct");
const string CMD_CHECKACCOUNT_ALT2      = _T("account");

const string CMD_SUBSCRIBE              = _T("subscribe");
const string CMD_SUBSCRIBE_ALT1         = _T("sub");

const string CMD_SETBILLINGINFO         = _T("setbillinginfo");
const string CMD_SETBILLINGINFO_ALT1    = _T("setbill");

const string CMD_GETBILLINGINFO         = _T("getbillinginfo");
const string CMD_GETBILLINGINFO_ALT1    = _T("getbill");

const string CMD_DELETEBILLINGINFO      = _T("deletebillinginfo");
const string CMD_DELETEBILLINGINFO_ALT1 = _T("delbill");
const string ARG_DELETEBILLINGINFO      = _T("key");

const string ARG_BILLING_NAME           = _T("cardname");
const string ARG_BILLING_NUMBER         = _T("cardnumber");
const string ARG_BILLING_EXPIRES        = _T("expires");
const string ARG_BILLING_CVV            = _T("cvv");
const string ARG_BILLING_ADDRESS1       = _T("address1");
const string ARG_BILLING_ADDRESS2       = _T("address2");
const string ARG_BILLING_CITY           = _T("city");
const string ARG_BILLING_STATE          = _T("state");
const string ARG_BILLING_ZIP            = _T("zip");
const string ARG_BILLING_COUNTRY        = _T("country");

const string CMD_SEARCHPAYMENTS         = _T("searchpayments");
const string CMD_SEARCHPAYMENTS_ALT1    = _T("spay");

const string CMD_SEARCHFILES            = _T("searchfiles");
const string CMD_SEARCHFILES_ALT1       = _T("sf");
const string CMD_SEARCHFILES_ALT2       = _T("s");
const string CMD_SEARCHFILES_ALT3       = _T("dir");
const string CMD_SEARCHFILES_ALT4       = _T("ls");

const string ARG_SEARCH_MINSIZE         = _T("minsize");
const string ARG_SEARCH_MAXSIZE         = _T("maxsize");
const string ARG_SEARCH_ISDELETED       = _T("isdeleted");
const string ARG_SEARCH_ISCOMPLETE      = _T("iscomplete");
const string ARG_SEARCH_PHYSICALFILES_SWITCH    = _T("pf");

const string ARG_SEARCH_METANAME        = _T("metaname");
const string ARG_SEARCH_METAVALUE       = _T("metavalue");

const string CMD_SEARCHFILESTOTAL       = _T("searchtotal");
const string CMD_SEARCHFILESTOTAL_ALT1  = _T("searchtot");
const string CMD_SEARCHFILESTOTAL_ALT2  = _T("sft");
const string CMD_SEARCHFILESTOTAL_ALT3  = _T("st");

const string CMD_SEARCHFILESTOTALLOG        = _T("searchtotallog");
const string CMD_SEARCHFILESTOTALLOG_ALT1   = _T("sftl");
const string CMD_SEARCHFILESTOTALLOG_ALT2   = _T("stl");

const string CMD_SEARCHUPLOADS          = _T("searchuploads");
const string CMD_SEARCHUPLOADS_ALT1     = _T("sup");

const string CMD_SEARCHDOWNLOADS        = _T("searchdownloads");
const string CMD_SEARCHDOWNLOADS_ALT1   = _T("sdown");

const string CMD_SEARCHLOGINS           = _T("searchlogins");
const string CMD_SEARCHLOGINS_ALT1      = _T("slogin");

//---------------------------------------------------------------------
// Purchasing and billing methods - the first group are those
// exposed to the user - the remaining are currently not
// exposed directly, but are left here since they're a pain to add...
//---------------------------------------------------------------------
const string CMD_GETALLPRODUCTS         = _T("getallproducts");
const string CMD_PURCHASEPRODUCT        = _T("purchaseproduct");
const string CMD_PURCHASEPRODUCT_ALT1   = _T("buyproduct");
const string CMD_PURCHASEPRODUCT_ALT2   = _T("buyprod");
const string ARG_PRODID                 = _T("prodid");

const string CMD_GETMYPRODUCTS          = _T("getmyproducts");

const string CMD_CANCELPRODUCT          = _T("cancelproduct");
const string CMD_CANCELPRODUCT_ALT1     = _T("cancelprod");

const string CMD_GETALLCONTRACTS        = _T("getallcontracts");
const string CMD_PURCHASECONTRACT       = _T("purchasecontract");
const string CMD_PURCHASECONTRACT_ALT1  = _T("buycontract");
const string CMD_PURCHASECONTRACT_ALT2  = _T("buycont");
const string ARG_CONTRACTID             = _T("contractid");

const string CMD_GETMYCONTRACTS         = _T("getmycontracts");
const string CMD_CANCELCONTRACT         = _T("cancelcontract");
const string CMD_CANCELCONTRACT_ALT1    = _T("cancelcont");

const string CMD_SEARCHINVOICES         = _T("searchinvoices");
const string CMD_SEARCHINVOICES_ALT1    = _T("sinvoice");
const string ARG_INVOICE_STATUS         = _T("status");
const string ARG_PAID_STATUS            = _T("paid");
const string ARG_UNPAID_STATUS          = _T("unpaid");

//---------------------------------------------------------------------
// Currently not explicity exposed to the user...
//---------------------------------------------------------------------
const string CMD_PURCHASESUPPORT        = _T("purchasesupport");
const string CMD_GETMYSUPPORTS          = _T("getmysupports");
const string CMD_CANCELSUPPORT          = _T("cancelsupport");

//---------------------------------------------------------------------
// Console system calls.
//---------------------------------------------------------------------
const string CMD_CLS                    = _T("cls");
const string CMD_REM                    = _T("rem");
const string CMD_REM_ALT1               = _T("#");
const string ARG_REM                    = _T("");
const string CMD_ECHO                   = _T("echo");
const string ARG_ECHO                   = _T("");

//---------------------------------------------------------------------
// Help: Group headers
//---------------------------------------------------------------------
const string GROUP_USER                 = _T("User management:");
const string GROUP_TRANSFER             = _T("Uploading & Downloading:");
const string GROUP_FILES                = _T("File operations:");
const string GROUP_METADATA             = _T("Metadata operations:");
const string GROUP_REPLICATE            = _T("Replication management:");
const string GROUP_POLICY               = _T("Replication policy management:");
const string GROUP_LOGS                 = _T("Log access:");
const string GROUP_PRODUCTS             = _T("Product information access: ");
const string GROUP_CLIENT               = _T("DioCLI client-only commands:");

//---------------------------------------------------------------------
//---------------------------------------------------------------------
namespace DioCLICommands {
	typedef enum {
		 CMD_NULL
		,CMD_EXIT
		,CMD_MENU
		,CMD_HELP
		,CMD_ABOUT
		,CMD_STATUS

		,CMD_CLS
		,CMD_REM
		,CMD_ECHO
		,CMD_SETCONFIG
		,CMD_SESSIONTOKEN
		,CMD_CHECKACCOUNT
        ,CMD_SUBSCRIBE

		,CMD_CREATEUSER
		,CMD_CHANGEPASSWORD
		,CMD_RESETPASSWORD
		,CMD_LOGIN
		,CMD_LOGOUT
		,CMD_SETUSERINFO
		,CMD_GETUSERINFO
		,CMD_DELETEUSERINFO
		,CMD_DELETEUSER

		,CMD_UPLOAD
		,CMD_RESUME
		,CMD_DOWNLOAD
		,CMD_GETUPLOADTOKEN
		,CMD_GETDOWNLOADURL

		,CMD_SEARCHFILES
		,CMD_SEARCHFILESTOTAL
		,CMD_SEARCHFILESTOTALLOG
		,CMD_DELETEFILE
		,CMD_UNDELETEFILE
		,CMD_RENAMEFILE
		,CMD_GETFILEINFO
		,CMD_DISPLAYFILE

		,CMD_CREATEMETADATA
		,CMD_CREATEFILEMETADATA
		,CMD_SETFILEMETADATA
		,CMD_GETMETADATA
		,CMD_GETFILEMETADATA
		,CMD_DELETEFILEMETADATA
		,CMD_DELETEMETADATA
		,CMD_EDITMETADATA

		,CMD_REPLICATEFILE
		,CMD_UNREPLICATEFILE
		,CMD_GETSTORAGETYPES
		,CMD_GETPHYSICALFILES

		,CMD_CREATEREPLICATIONPOLICY
		,CMD_GETREPLICATIONPOLICIES
		,CMD_EDITREPLICATIONPOLICY
		,CMD_DELETEREPLICATIONPOLICY
		,CMD_SETREPLICATIONPOLICY
		,CMD_SETDEFREPLICATIONPOLICY
		,CMD_GETDEFREPLICATIONPOLICY

		,CMD_GETEMAILADDRESSES
		,CMD_ADDEMAILADDRESS
		,CMD_DELETEEMAILADDRESS
		,CMD_SETPRIMARYEMAILADDRESS

		,CMD_SETBILLINGINFO
		,CMD_GETBILLINGINFO
		,CMD_DELETEBILLINGINFO
		,CMD_SEARCHPAYMENTS

		,CMD_SEARCHUPLOADS
		,CMD_SEARCHDOWNLOADS
		,CMD_SEARCHLOGINS

		,CMD_SEARCHINVOICES
		,CMD_GETALLPRODUCTS
		,CMD_PURCHASEPRODUCT
		,CMD_GETMYPRODUCTS
		,CMD_CANCELPRODUCT

		,CMD_GETALLCONTRACTS
		,CMD_PURCHASECONTRACT
		,CMD_GETMYCONTRACTS
		,CMD_CANCELCONTRACT

		,CMD_PURCHASESUPPORT
		,CMD_GETMYSUPPORTS
		,CMD_CANCELSUPPORT

		,CMD_LAST
		,LAST_MENU_ITEM
	} COMMAND_ID;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
namespace DioCLIGroups {
	typedef enum {
		 GROUP_NULL
		,GROUP_USER
		,GROUP_TRANSFER
		,GROUP_FILES
		,GROUP_METADATA
		,GROUP_REPLICATE
		,GROUP_POLICY
		,GROUP_LOGS
		,GROUP_PRODUCTS
		,GROUP_CLIENT
		,GROUP_LAST
		,LAST_GROUP_ITEM
	} GROUP_ID;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef struct {
	DioCLIGroups::GROUP_ID                  m_groupID;
	std::string			                    m_szGroupDesc;
	bool                                    m_bDisplayGroup;
} GROUP_SET;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef struct {
	DioCLICommands::COMMAND_ID              m_commandID;
	std::string			                    m_szCommand;
	DioCLIGroups::GROUP_ID                  m_groupID;
	std::vector<std::string>                m_listAlternate;
} COMMAND_SET;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef struct {
	std::string                             m_szCommand;
	std::string			                    m_szAlternateCommand;
} AlTERNATE_COMMAND_SET;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const AlTERNATE_COMMAND_SET DioAlternateCommands[] = {
     { CMD_CREATEUSER,                      CMD_CREATEUSER              }
    ,{ CMD_CREATEUSER,                      CMD_CREATEUSER_ALT1         }

    ,{ CMD_CHANGEPASSWORD,                  CMD_CHANGEPASSWORD          }
    ,{ CMD_CHANGEPASSWORD,                  CMD_CHANGEPASSWORD_ALT1     }
    ,{ CMD_CHANGEPASSWORD,                  CMD_CHANGEPASSWORD_ALT2     }

    ,{ CMD_RESETPASSWORD,                   CMD_RESETPASSWORD           }
    ,{ CMD_RESETPASSWORD,                   CMD_RESETPASSWORD_ALT1      }
    ,{ CMD_RESETPASSWORD,                   CMD_RESETPASSWORD_ALT2      }

    ,{ CMD_LOGIN,                           CMD_LOGIN                   }
    ,{ CMD_LOGIN,                           CMD_LOGIN_ALT1              }

    ,{ CMD_LOGOUT,                          CMD_LOGOUT                  }
    ,{ CMD_LOGOUT,                          CMD_LOGOUT_ALT1             }

    ,{ CMD_SESSIONTOKEN,                    CMD_SESSIONTOKEN            }
    ,{ CMD_SESSIONTOKEN,                    CMD_SESSIONTOKEN_ALT1       }

    ,{ CMD_DELETEUSER,                      CMD_DELETEUSER              }
    ,{ CMD_DELETEUSER,                      CMD_DELETEUSER_ALT1         }

    ,{ CMD_UPLOAD,                          CMD_UPLOAD                  }
    ,{ CMD_UPLOAD,                          CMD_UPLOAD_ALT1             }
    ,{ CMD_UPLOAD,                          CMD_UPLOAD_ALT2             }

    ,{ CMD_RESUME,                          CMD_RESUME                  }
    ,{ CMD_RESUME,                          CMD_RESUME_ALT1             }
    ,{ CMD_RESUME,                          CMD_RESUME_ALT2             }

    ,{ CMD_DOWNLOAD,                        CMD_DOWNLOAD                }
    ,{ CMD_DOWNLOAD,                        CMD_DOWNLOAD_ALT1           }
    ,{ CMD_DOWNLOAD,                        CMD_DOWNLOAD_ALT2           }

    ,{ CMD_GETUPLOADTOKEN,                  CMD_GETUPLOADTOKEN          }
    ,{ CMD_GETUPLOADTOKEN,                  CMD_GETUPLOADTOKEN_ALT1     }
    ,{ CMD_GETUPLOADTOKEN,                  CMD_GETUPLOADTOKEN_ALT2     }

    ,{ CMD_GETDOWNLOADURL,                  CMD_GETDOWNLOADURL          }
    ,{ CMD_GETDOWNLOADURL,                  CMD_GETDOWNLOADURL_ALT1     }
    ,{ CMD_GETDOWNLOADURL,                  CMD_GETDOWNLOADURL_ALT2     }

    ,{ CMD_DELETEFILE,                      CMD_DELETEFILE              }
    ,{ CMD_DELETEFILE,                      CMD_DELETEFILE_ALT1         }
    ,{ CMD_DELETEFILE,                      CMD_DELETEFILE_ALT2         }
    ,{ CMD_DELETEFILE,                      CMD_DELETEFILE_ALT3         }

    ,{ CMD_UNDELETEFILE,                    CMD_UNDELETEFILE            }
    ,{ CMD_UNDELETEFILE,                    CMD_UNDELETEFILE_ALT1       }

    ,{ CMD_RENAMEFILE,                      CMD_RENAMEFILE              }
    ,{ CMD_RENAMEFILE,                      CMD_RENAMEFILE_ALT1         }
    ,{ CMD_RENAMEFILE,                      CMD_RENAMEFILE_ALT2         }

    ,{ CMD_DISPLAYFILE,                     CMD_DISPLAYFILE             }
    ,{ CMD_DISPLAYFILE,                     CMD_DISPLAYFILE_ALT1        }

    ,{ CMD_CREATEMETADATA,                  CMD_CREATEMETADATA          }
    ,{ CMD_CREATEMETADATA,                  CMD_CREATEMETADATA_ALT1     }
    ,{ CMD_CREATEMETADATA,                  CMD_CREATEMETADATA_ALT2     }

    ,{ CMD_CREATEFILEMETADATA,              CMD_CREATEFILEMETADATA      }
    ,{ CMD_CREATEFILEMETADATA,              CMD_CREATEFILEMETADATA_ALT1 }
    ,{ CMD_CREATEFILEMETADATA,              CMD_CREATEFILEMETADATA_ALT2 }

    ,{ CMD_SETFILEMETADATA,                 CMD_SETFILEMETADATA         }
    ,{ CMD_SETFILEMETADATA,                 CMD_SETFILEMETADATA_ALT1    }
    ,{ CMD_SETFILEMETADATA,                 CMD_SETFILEMETADATA_ALT2    }

    ,{ CMD_GETMETADATA,                     CMD_GETMETADATA             }
    ,{ CMD_GETMETADATA,                     CMD_GETMETADATA_ALT1        }

    ,{ CMD_GETFILEMETADATA,                 CMD_GETFILEMETADATA         }
    ,{ CMD_GETFILEMETADATA,                 CMD_GETFILEMETADATA_ALT1    }
    ,{ CMD_GETFILEMETADATA,                 CMD_GETFILEMETADATA_ALT2    }

    ,{ CMD_DELETEMETADATA,                  CMD_DELETEMETADATA          }
    ,{ CMD_DELETEMETADATA,                  CMD_DELETEMETADATA_ALT1     }
    ,{ CMD_DELETEMETADATA,                  CMD_DELETEMETADATA_ALT2     }

    ,{ CMD_DELETEFILEMETADATA,              CMD_DELETEFILEMETADATA      }
    ,{ CMD_DELETEFILEMETADATA,              CMD_DELETEFILEMETADATA_ALT1 }

    ,{ CMD_EDITMETADATA,                    CMD_EDITMETADATA            }
    ,{ CMD_EDITMETADATA,                    CMD_EDITMETADATA_ALT1       }

    ,{ CMD_REPLICATEFILE,                   CMD_REPLICATEFILE           }
    ,{ CMD_REPLICATEFILE,                   CMD_REPLICATEFILE_ALT1      }

    ,{ CMD_UNREPLICATEFILE,                 CMD_UNREPLICATEFILE         }
    ,{ CMD_UNREPLICATEFILE,                 CMD_UNREPLICATEFILE_ALT1    }

    ,{ CMD_GETPHYSICALFILES,                CMD_GETPHYSICALFILES        }
    ,{ CMD_GETPHYSICALFILES,                CMD_GETPHYSICALFILES_ALT1   }

    ,{ CMD_CREATEREPLICATIONPOLICY,         CMD_CREATEREPLICATIONPOLICY     }
    ,{ CMD_CREATEREPLICATIONPOLICY,         CMD_CREATEREPLICATIONPOLICY_ALT1}

    ,{ CMD_GETREPLICATIONPOLICIES,          CMD_GETREPLICATIONPOLICIES      }
    ,{ CMD_GETREPLICATIONPOLICIES,          CMD_GETREPLICATIONPOLICIES_ALT1 }

    ,{ CMD_EDITREPLICATIONPOLICY,           CMD_EDITREPLICATIONPOLICY       }
    ,{ CMD_EDITREPLICATIONPOLICY,           CMD_EDITREPLICATIONPOLICY_ALT1  }

    ,{ CMD_DELETEREPLICATIONPOLICY,         CMD_DELETEREPLICATIONPOLICY     }
    ,{ CMD_DELETEREPLICATIONPOLICY,         CMD_DELETEREPLICATIONPOLICY_ALT1}

    ,{ CMD_SETREPLICATIONPOLICY,            CMD_SETREPLICATIONPOLICY        }
    ,{ CMD_SETREPLICATIONPOLICY,            CMD_SETREPLICATIONPOLICY_ALT1   }

    ,{ CMD_SETDEFREPLICATIONPOLICY,         CMD_SETDEFREPLICATIONPOLICY     }
    ,{ CMD_SETDEFREPLICATIONPOLICY,         CMD_SETDEFREPLICATIONPOLICY_ALT1}
    ,{ CMD_SETDEFREPLICATIONPOLICY,         CMD_SETDEFREPLICATIONPOLICY_ALT2}

    ,{ CMD_GETDEFREPLICATIONPOLICY,         CMD_GETDEFREPLICATIONPOLICY     }
    ,{ CMD_GETDEFREPLICATIONPOLICY,         CMD_GETDEFREPLICATIONPOLICY_ALT1}
    ,{ CMD_GETDEFREPLICATIONPOLICY,         CMD_GETDEFREPLICATIONPOLICY_ALT2}

    ,{ CMD_GETSTORAGETYPES,                 CMD_GETSTORAGETYPES         }
    ,{ CMD_GETSTORAGETYPES,                 CMD_GETSTORAGETYPES_ALT1    }

    ,{ CMD_SETUSERINFO,                     CMD_SETUSERINFO             }
    ,{ CMD_SETUSERINFO,                     CMD_SETUSERINFO_ALT1        }

    ,{ CMD_GETUSERINFO,                     CMD_GETUSERINFO             }
    ,{ CMD_GETUSERINFO,                     CMD_GETUSERINFO_ALT1        }

    ,{ CMD_DELETEUSERINFO,                  CMD_DELETEUSERINFO          }
    ,{ CMD_DELETEUSERINFO,                  CMD_DELETEUSERINFO_ALT1     }

    ,{ CMD_GETEMAILADDRESSES,               CMD_GETEMAILADDRESSES       }
    ,{ CMD_GETEMAILADDRESSES,               CMD_GETEMAILADDRESSES_ALT1  }

    ,{ CMD_DELETEEMAILADDRESS,               CMD_DELETEEMAILADDRESS        }
    ,{ CMD_DELETEEMAILADDRESS,               CMD_DELETEEMAILADDRESS_ALT1   }

    ,{ CMD_SETPRIMARYEMAILADDRESS,          CMD_SETPRIMARYEMAILADDRESS      }
    ,{ CMD_SETPRIMARYEMAILADDRESS,          CMD_SETPRIMARYEMAILADDRESS_ALT1 }

    ,{ CMD_CHECKACCOUNT,                    CMD_CHECKACCOUNT           }
    ,{ CMD_CHECKACCOUNT,                    CMD_CHECKACCOUNT_ALT1      }
    ,{ CMD_CHECKACCOUNT,                    CMD_CHECKACCOUNT_ALT2      }

    ,{ CMD_SUBSCRIBE,                       CMD_SUBSCRIBE               }
    ,{ CMD_SUBSCRIBE,                       CMD_SUBSCRIBE_ALT1          }

    ,{ CMD_SETBILLINGINFO,                  CMD_SETBILLINGINFO          }
    ,{ CMD_SETBILLINGINFO,                  CMD_SETBILLINGINFO_ALT1     }

    ,{ CMD_GETBILLINGINFO,                  CMD_GETBILLINGINFO          }
    ,{ CMD_GETBILLINGINFO,                  CMD_GETBILLINGINFO_ALT1     }

    ,{ CMD_DELETEBILLINGINFO,               CMD_DELETEBILLINGINFO       }
    ,{ CMD_DELETEBILLINGINFO,               CMD_DELETEBILLINGINFO_ALT1  }

    ,{ CMD_SEARCHPAYMENTS,                  CMD_SEARCHPAYMENTS          }
    ,{ CMD_SEARCHPAYMENTS,                  CMD_SEARCHPAYMENTS_ALT1     }

    ,{ CMD_SEARCHFILES,                     CMD_SEARCHFILES             }
    ,{ CMD_SEARCHFILES,                     CMD_SEARCHFILES_ALT1        }
    ,{ CMD_SEARCHFILES,                     CMD_SEARCHFILES_ALT2        }
    ,{ CMD_SEARCHFILES,                     CMD_SEARCHFILES_ALT3        }
    ,{ CMD_SEARCHFILES,                     CMD_SEARCHFILES_ALT4        }

    ,{ CMD_SEARCHFILESTOTAL,                CMD_SEARCHFILESTOTAL        }
    ,{ CMD_SEARCHFILESTOTAL,                CMD_SEARCHFILESTOTAL_ALT1   }
    ,{ CMD_SEARCHFILESTOTAL,                CMD_SEARCHFILESTOTAL_ALT2   }
    ,{ CMD_SEARCHFILESTOTAL,                CMD_SEARCHFILESTOTAL_ALT3   }

    ,{ CMD_SEARCHFILESTOTALLOG,             CMD_SEARCHFILESTOTALLOG         }
    ,{ CMD_SEARCHFILESTOTALLOG,             CMD_SEARCHFILESTOTALLOG_ALT1    }
    ,{ CMD_SEARCHFILESTOTALLOG,             CMD_SEARCHFILESTOTALLOG_ALT2    }

    ,{ CMD_SEARCHUPLOADS,                   CMD_SEARCHUPLOADS           }
    ,{ CMD_SEARCHUPLOADS,                   CMD_SEARCHUPLOADS_ALT1      }

    ,{ CMD_SEARCHDOWNLOADS,                 CMD_SEARCHDOWNLOADS         }
    ,{ CMD_SEARCHDOWNLOADS,                 CMD_SEARCHDOWNLOADS_ALT1    }

    ,{ CMD_SEARCHLOGINS,                    CMD_SEARCHLOGINS            }
    ,{ CMD_SEARCHLOGINS,                    CMD_SEARCHLOGINS_ALT1       }

    ,{ CMD_SEARCHINVOICES,                  CMD_SEARCHINVOICES          }
    ,{ CMD_SEARCHINVOICES,                  CMD_SEARCHINVOICES_ALT1     }

    ,{ CMD_PURCHASEPRODUCT,                 CMD_PURCHASEPRODUCT         }
    ,{ CMD_PURCHASEPRODUCT,                 CMD_PURCHASEPRODUCT_ALT1    }
    ,{ CMD_PURCHASEPRODUCT,                 CMD_PURCHASEPRODUCT_ALT2    }

    ,{ CMD_CANCELPRODUCT,                   CMD_CANCELPRODUCT           }
    ,{ CMD_CANCELPRODUCT,                   CMD_CANCELPRODUCT_ALT1      }

    ,{ CMD_CANCELCONTRACT,                  CMD_CANCELCONTRACT          }
    ,{ CMD_CANCELCONTRACT,                  CMD_CANCELCONTRACT_ALT1     }

    ,{ CMD_HELP,                            CMD_HELP                    }
    ,{ CMD_HELP,                            CMD_HELP_ALT1               }

    ,{ CMD_ABOUT,                           CMD_ABOUT                   }
    ,{ CMD_ABOUT,                           CMD_ABOUT_ALT1              }

    ,{ CMD_REM,                             CMD_REM                     }
    ,{ CMD_REM,                             CMD_REM_ALT1                }

    ,{ CMD_LAST,                            CMD_LAST                    }
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const GROUP_SET DioCLIGroupList[] = {
	 { DioCLIGroups::GROUP_USER,		    GROUP_USER,         true    }
	,{ DioCLIGroups::GROUP_TRANSFER,	   	GROUP_TRANSFER,     true    }
	,{ DioCLIGroups::GROUP_FILES,	    	GROUP_FILES,        true    }
	,{ DioCLIGroups::GROUP_METADATA,	   	GROUP_METADATA,     true    }
	,{ DioCLIGroups::GROUP_REPLICATE,	   	GROUP_REPLICATE,    true    }
	,{ DioCLIGroups::GROUP_POLICY,	   	    GROUP_POLICY,       true    }
	,{ DioCLIGroups::GROUP_LOGS,	    	GROUP_LOGS,         true    }
	,{ DioCLIGroups::GROUP_PRODUCTS,	    GROUP_PRODUCTS,     true    }
	,{ DioCLIGroups::GROUP_CLIENT,	    	GROUP_CLIENT,       true    }
	,{ DioCLIGroups::GROUP_LAST,		    GROUP_LAST,         false   }
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const COMMAND_SET DioCLICommandList[] = {
	 { DioCLICommands::CMD_CREATEUSER,  	CMD_CREATEUSER,         DioCLIGroups::GROUP_USER        }
	,{ DioCLICommands::CMD_CHANGEPASSWORD,  CMD_CHANGEPASSWORD,     DioCLIGroups::GROUP_USER        }
	,{ DioCLICommands::CMD_RESETPASSWORD,   CMD_RESETPASSWORD,      DioCLIGroups::GROUP_USER        }
	,{ DioCLICommands::CMD_LOGIN,		    CMD_LOGIN,              DioCLIGroups::GROUP_USER        }
	,{ DioCLICommands::CMD_LOGOUT,		    CMD_LOGOUT,             DioCLIGroups::GROUP_USER        }
	,{ DioCLICommands::CMD_SETUSERINFO,		CMD_SETUSERINFO,        DioCLIGroups::GROUP_USER		}
	,{ DioCLICommands::CMD_GETUSERINFO,		CMD_GETUSERINFO,        DioCLIGroups::GROUP_USER		}
	,{ DioCLICommands::CMD_DELETEUSERINFO,	CMD_DELETEUSERINFO,     DioCLIGroups::GROUP_USER		}
	,{ DioCLICommands::CMD_DELETEUSER,		CMD_DELETEUSER,         DioCLIGroups::GROUP_USER		}

	,{ DioCLICommands::CMD_GETEMAILADDRESSES,	    CMD_GETEMAILADDRESSES,      DioCLIGroups::GROUP_USER    }
	,{ DioCLICommands::CMD_ADDEMAILADDRESS,	        CMD_ADDEMAILADDRESS,        DioCLIGroups::GROUP_USER	}
	,{ DioCLICommands::CMD_DELETEEMAILADDRESS,      CMD_DELETEEMAILADDRESS,     DioCLIGroups::GROUP_USER	}
	,{ DioCLICommands::CMD_SETPRIMARYEMAILADDRESS,  CMD_SETPRIMARYEMAILADDRESS, DioCLIGroups::GROUP_USER	}

	,{ DioCLICommands::CMD_UPLOAD,		    CMD_UPLOAD,             DioCLIGroups::GROUP_TRANSFER    }
	,{ DioCLICommands::CMD_RESUME,		    CMD_RESUME,             DioCLIGroups::GROUP_TRANSFER    }
	,{ DioCLICommands::CMD_DOWNLOAD,		CMD_DOWNLOAD,           DioCLIGroups::GROUP_TRANSFER    }
	,{ DioCLICommands::CMD_GETUPLOADTOKEN,	CMD_GETUPLOADTOKEN,     DioCLIGroups::GROUP_TRANSFER    }
	,{ DioCLICommands::CMD_GETDOWNLOADURL,	CMD_GETDOWNLOADURL,     DioCLIGroups::GROUP_TRANSFER    }

	,{ DioCLICommands::CMD_SEARCHFILES,		CMD_SEARCHFILES,        DioCLIGroups::GROUP_FILES		}
	,{ DioCLICommands::CMD_SEARCHFILESTOTAL,CMD_SEARCHFILESTOTAL,   DioCLIGroups::GROUP_FILES	    }
	,{ DioCLICommands::CMD_SEARCHFILESTOTALLOG,     CMD_SEARCHFILESTOTALLOG,      DioCLIGroups::GROUP_FILES   }
	,{ DioCLICommands::CMD_DELETEFILE,	    CMD_DELETEFILE,         DioCLIGroups::GROUP_FILES		}
	,{ DioCLICommands::CMD_UNDELETEFILE,	CMD_UNDELETEFILE,       DioCLIGroups::GROUP_FILES	    }
	,{ DioCLICommands::CMD_RENAMEFILE,		CMD_RENAMEFILE,         DioCLIGroups::GROUP_FILES		}
	,{ DioCLICommands::CMD_GETFILEINFO,		CMD_GETFILEINFO,        DioCLIGroups::GROUP_FILES		}
	,{ DioCLICommands::CMD_DISPLAYFILE,		CMD_DISPLAYFILE,        DioCLIGroups::GROUP_FILES		}

	,{ DioCLICommands::CMD_CREATEMETADATA,	CMD_CREATEMETADATA,     DioCLIGroups::GROUP_METADATA	}
	,{ DioCLICommands::CMD_CREATEFILEMETADATA,  CMD_CREATEFILEMETADATA,         DioCLIGroups::GROUP_METADATA	}
	,{ DioCLICommands::CMD_SETFILEMETADATA,	CMD_SETFILEMETADATA,    DioCLIGroups::GROUP_METADATA	}
	,{ DioCLICommands::CMD_GETMETADATA,		CMD_GETMETADATA,        DioCLIGroups::GROUP_METADATA	}
	,{ DioCLICommands::CMD_GETFILEMETADATA,	CMD_GETFILEMETADATA,    DioCLIGroups::GROUP_METADATA	}
	,{ DioCLICommands::CMD_DELETEFILEMETADATA,  CMD_DELETEFILEMETADATA,     DioCLIGroups::GROUP_METADATA        }
	,{ DioCLICommands::CMD_DELETEMETADATA,	CMD_DELETEMETADATA,     DioCLIGroups::GROUP_METADATA	}
	,{ DioCLICommands::CMD_EDITMETADATA,	CMD_EDITMETADATA,       DioCLIGroups::GROUP_METADATA	}

	,{ DioCLICommands::CMD_REPLICATEFILE,	CMD_REPLICATEFILE,      DioCLIGroups::GROUP_REPLICATE	}
	,{ DioCLICommands::CMD_UNREPLICATEFILE,	CMD_UNREPLICATEFILE,    DioCLIGroups::GROUP_REPLICATE	}
	,{ DioCLICommands::CMD_GETPHYSICALFILES,     CMD_GETPHYSICALFILES,    DioCLIGroups::GROUP_REPLICATE	        }
	,{ DioCLICommands::CMD_GETSTORAGETYPES,	CMD_GETSTORAGETYPES,    DioCLIGroups::GROUP_REPLICATE	}

	,{ DioCLICommands::CMD_CREATEREPLICATIONPOLICY, CMD_CREATEREPLICATIONPOLICY,    DioCLIGroups::GROUP_POLICY  }
	,{ DioCLICommands::CMD_GETREPLICATIONPOLICIES,  CMD_GETREPLICATIONPOLICIES,     DioCLIGroups::GROUP_POLICY  }
	,{ DioCLICommands::CMD_EDITREPLICATIONPOLICY,   CMD_EDITREPLICATIONPOLICY,      DioCLIGroups::GROUP_POLICY  }
	,{ DioCLICommands::CMD_DELETEREPLICATIONPOLICY, CMD_DELETEREPLICATIONPOLICY,    DioCLIGroups::GROUP_POLICY  }
	,{ DioCLICommands::CMD_SETREPLICATIONPOLICY,    CMD_SETREPLICATIONPOLICY,       DioCLIGroups::GROUP_POLICY  }
	,{ DioCLICommands::CMD_SETDEFREPLICATIONPOLICY, CMD_SETDEFREPLICATIONPOLICY,    DioCLIGroups::GROUP_POLICY  }
	,{ DioCLICommands::CMD_GETDEFREPLICATIONPOLICY, CMD_GETDEFREPLICATIONPOLICY,    DioCLIGroups::GROUP_POLICY  }

	,{ DioCLICommands::CMD_SEARCHPAYMENTS,  CMD_SEARCHPAYMENTS,     DioCLIGroups::GROUP_LOGS        }
	,{ DioCLICommands::CMD_SEARCHLOGINS,	CMD_SEARCHLOGINS,       DioCLIGroups::GROUP_LOGS	    }
	,{ DioCLICommands::CMD_SEARCHUPLOADS,	CMD_SEARCHUPLOADS,      DioCLIGroups::GROUP_LOGS	    }
	,{ DioCLICommands::CMD_SEARCHDOWNLOADS,	CMD_SEARCHDOWNLOADS,    DioCLIGroups::GROUP_LOGS	    }

	,{ DioCLICommands::CMD_SETBILLINGINFO,	CMD_SETBILLINGINFO,     DioCLIGroups::GROUP_PRODUCTS                }
	,{ DioCLICommands::CMD_GETBILLINGINFO,	CMD_GETBILLINGINFO,     DioCLIGroups::GROUP_PRODUCTS                }
	,{ DioCLICommands::CMD_DELETEBILLINGINFO,	    CMD_DELETEBILLINGINFO,      DioCLIGroups::GROUP_PRODUCTS    }

	,{ DioCLICommands::CMD_GETALLPRODUCTS,	CMD_GETALLPRODUCTS,     DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_PURCHASEPRODUCT,	CMD_PURCHASEPRODUCT,    DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_GETMYPRODUCTS,	CMD_GETMYPRODUCTS,      DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_CANCELPRODUCT,	CMD_CANCELPRODUCT,      DioCLIGroups::GROUP_PRODUCTS    }

	,{ DioCLICommands::CMD_GETALLCONTRACTS,	CMD_GETALLCONTRACTS,    DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_PURCHASECONTRACT,CMD_PURCHASECONTRACT,   DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_GETMYCONTRACTS,	CMD_GETMYCONTRACTS,     DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_CANCELCONTRACT,	CMD_CANCELCONTRACT,     DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_SEARCHINVOICES,  CMD_SEARCHINVOICES,     DioCLIGroups::GROUP_PRODUCTS    }

	,{ DioCLICommands::CMD_PURCHASESUPPORT,	CMD_PURCHASESUPPORT,    DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_GETMYSUPPORTS,	CMD_GETMYSUPPORTS,      DioCLIGroups::GROUP_PRODUCTS    }
	,{ DioCLICommands::CMD_CANCELSUPPORT,	CMD_CANCELSUPPORT,      DioCLIGroups::GROUP_PRODUCTS    }

	,{ DioCLICommands::CMD_HELP,            CMD_HELP,               DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_ABOUT,           CMD_ABOUT,              DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_EXIT,            CMD_EXIT,               DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_MENU,            CMD_MENU,               DioCLIGroups::GROUP_CLIENT      }

	,{ DioCLICommands::CMD_STATUS,	    	CMD_STATUS,             DioCLIGroups::GROUP_CLIENT      }

	,{ DioCLICommands::CMD_CLS,	    	    CMD_CLS,                DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_REM,	    	    CMD_REM,                DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_ECHO,	    	CMD_ECHO,               DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_SETCONFIG,       CMD_SETCONFIG,          DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_SESSIONTOKEN,    CMD_SESSIONTOKEN,       DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_CHECKACCOUNT,	CMD_CHECKACCOUNT,       DioCLIGroups::GROUP_CLIENT      }
	,{ DioCLICommands::CMD_SUBSCRIBE,	    CMD_SUBSCRIBE,          DioCLIGroups::GROUP_CLIENT      }

	,{ DioCLICommands::CMD_LAST,		    CMD_LAST,               DioCLIGroups::GROUP_LAST		}
};

#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
inline DioCLICommands::COMMAND_ID CommandStrToCommandID( const string szCommandStr )
{
	static map<string, COMMAND_SET> mapCommands;
	if ( mapCommands.size() == 0) {

		// Create and initilize all the commands in a map.
		// This will be quicker and less prone to error that
		// all the if statments
		unsigned int nIndex = 0;

		// Original version allocated this on the heap - not sure why -
		// problem is that then you need to clean it up.
		// TBD: make this part of the ConsoleControl class.
		// mapCommands = new map<string,COMMAND_SET>;

		do {
			if (DioCLICommandList[nIndex].m_commandID != DioCLICommands::CMD_LAST) {
			    COMMAND_SET cmdSet = DioCLICommandList[nIndex];
				(mapCommands)[DioCLICommandList[nIndex].m_szCommand] = cmdSet;

				#if 0
				    // For debugging data
				    cout << _T("Setting ") << DioCLICommandList[nIndex].m_szCommand
				                           << _T(" to ") << cmdSet.m_commandID << endl;
				#endif
			}
			nIndex++;

		} while (DioCLICommandList[nIndex].m_commandID != DioCLICommands::CMD_LAST);


	}

    char szCommandBuffer[100];
    strcpy(szCommandBuffer, szCommandStr.c_str());

    _tcslwr(szCommandBuffer);
    std::string szTmpCommand(szCommandBuffer);

	// Pull and return the id from the map
	std::map<string, COMMAND_SET>::iterator iter = mapCommands.find( szTmpCommand );
	return iter == mapCommands.end() ? DioCLICommands::CMD_NULL : (*iter).second.m_commandID;

} // End CommandStrToCommandID

//---------------------------------------------------------------------
// Return the primary command for a given command.  Setup the
// map of alternate to primary commands on first access.
//---------------------------------------------------------------------
inline std::string AltCommandStrToCommandStr( const string szInputCmdStr )
{
	static map<string, string> mapAltCommands;
	if ( mapAltCommands.size() == 0) {

		// Create and initilize all the commands in a map.
		// This will be quicker and less prone to error that
		// all the if statments
		unsigned int nIndex = 0;

		// Original version allocated this on the heap - not sure why -
		// problem is that then you need to clean it up.
		// TBD: make this part of the ConsoleControl class.
		// mapAltCommands = new map<string,COMMAND_SET>;
		do {
		    if (DioAlternateCommands[nIndex].m_szCommand != CMD_LAST) {

		        std::string szCommand = DioAlternateCommands[nIndex].m_szCommand;

		        while (szCommand == DioAlternateCommands[nIndex].m_szCommand) {
		            std::string szAltCmd = DioAlternateCommands[nIndex].m_szAlternateCommand;
		            mapAltCommands.insert(make_pair(szAltCmd,szCommand));
		            nIndex ++;
		        }
		    }
	    } while (DioAlternateCommands[nIndex].m_szCommand != CMD_LAST);
    }

    char szCommandBuffer[100];
    strcpy(szCommandBuffer, szInputCmdStr.c_str());

    _tcslwr(szCommandBuffer);
    std::string szOutCmdStr(szCommandBuffer);

    map<string,string>::iterator iter = mapAltCommands.find(szOutCmdStr);
    if( iter != mapAltCommands.end() ) {
        szOutCmdStr = iter->second;
    }
    else {
        szOutCmdStr = szInputCmdStr;
    }

	return szOutCmdStr;

} // End AltCommandStrToCommandStr

//---------------------------------------------------------------------
// For a given primary command, return it's list of alternates.
//---------------------------------------------------------------------
inline void GetAltCommandStrs( const string szCommandStr, list<string>* pListAltCommannds )
{
	// Create and initilize all the commands in a map.
	// This will be quicker and less prone to error that
	// all the if statments
	unsigned int nIndex = 0;

	// Original version allocated this on the heap - not sure why -
	// problem is that then you need to clean it up.
	// TBD: make this part of the ConsoleControl class.
	// mapAltCommands = new map<string,COMMAND_SET>;
	do {
	    if (DioAlternateCommands[nIndex].m_szCommand != CMD_LAST) {

	        if (szCommandStr == DioAlternateCommands[nIndex].m_szCommand) {
	            std::string szAltCmd = DioAlternateCommands[nIndex].m_szAlternateCommand;
	            pListAltCommannds->push_back(szAltCmd);
	        }
	    }

        nIndex ++;

    } while (DioAlternateCommands[nIndex].m_szCommand != CMD_LAST);

} // End GetAltCommandStrs

#endif // __COMMAND_DEFS_H__