/*********************************************************************
 *
 *  file:  DiomedeStorage.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Classes derived from DiomedeStorage and  DiomedeStorageTransfer
 *          to facilitate changes to the the endpoint set by the gSoap 
 *          compiler.
 *
 *********************************************************************/

#ifndef __DIOMEDE_STORAGE_H__
#define __DIOMEDE_STORAGE_H__

#include "../CPPSDK.Lib/stdafx.h"

#include "types.h"

#include "../gSoap/soapDiomedeStorageProxy.h"
#include "../gSoap/soapDiomedeStorageTransferProxy.h"
#include "../CPPSDK.Lib/SecureThread.h"

#include "../Util/UserProfileData.h"
#include "../Util/ProfileManager.h"
#include "../Util/Util.h"

#ifdef _DEBUG
// #define LOCAL_SERVICE
#endif

static const std::string m_szCertPem = _T("Diomede.pem");

///////////////////////////////////////////////////////////////////////
class DiomedeServiceConfig
{
protected:
    int             m_nSendTimeout;
    int             m_nRecvTimeout;
    int             m_nConnectTimeout;
    
    std::string     m_szProxyHost;
    int             m_nProxyPort;
    std::string     m_szProxyUserID;
    std::string     m_szProxyPassword;
    struct soap*    m_pSoap;

public:
	DiomedeServiceConfig() : m_nSendTimeout(0), m_nRecvTimeout(0),
                             m_nConnectTimeout(0), m_szProxyHost(_T("")), m_nProxyPort(0), 
                             m_szProxyUserID(_T("")), m_szProxyPassword(_T("")),
                             m_pSoap(NULL) {};
	virtual ~DiomedeServiceConfig() 
	{
	    if (m_pSoap) {
	        if (m_pSoap->proxy_host) {
	            delete [] m_pSoap->proxy_host;
	        }
	        if (m_pSoap->proxy_userid) {
	            delete [] m_pSoap->proxy_userid;
	        }
	        if (m_pSoap->proxy_passwd) {
	            delete [] m_pSoap->proxy_passwd;
	        }
	    }
	};

    //-----------------------------------------------------------------
    // Configure soap using the data from  the configuration file.
    //-----------------------------------------------------------------
    void Configure(struct soap* soap)
    {
        if (soap == NULL) {
            return;
        }

        m_pSoap = soap;
        
        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), false );
        if (pProfileData) {
            m_nSendTimeout = pProfileData->GetUserProfileInt(GEN_SEND_TIMEOUT,
                                                           GEN_SEND_TIMEOUT_DF);
            m_nRecvTimeout = pProfileData->GetUserProfileInt(GEN_RECV_TIMEOUT,
                                                           GEN_RECV_TIMEOUT_DF);
            m_nConnectTimeout = pProfileData->GetUserProfileInt(GEN_CONNECT_TIMEOUT,
                                                              GEN_CONNECT_TIMEOUT_DF);
                                                              
            m_szProxyHost = pProfileData->GetUserProfileStr(GEN_PROXY_HOST, GEN_PROXY_HOST_DF);
            m_nProxyPort = pProfileData->GetUserProfileInt(GEN_PROXY_PORT, GEN_PROXY_PORT_DF);
            m_szProxyUserID = pProfileData->GetUserProfileStr(GEN_PROXY_USERID, GEN_PROXY_USERID_DF);
            m_szProxyPassword = pProfileData->GetUserProfileStr(GEN_PROXY_PASSWORD, GEN_PROXY_PASSWORD_DF);
        }

	    if (m_pSoap) {
	        m_pSoap->imode = SOAP_IO_KEEPALIVE;
	        m_pSoap->omode = SOAP_IO_KEEPALIVE;
	        m_pSoap->send_timeout = m_nSendTimeout;
	        m_pSoap->recv_timeout = m_nRecvTimeout;
	        m_pSoap->connect_timeout = m_nConnectTimeout;
	        m_pSoap->header = NULL;
	        
	        if (m_szProxyHost.length() > 0) {

                int sourceLength = m_szProxyHost.length();
                m_pSoap->proxy_host = new char[sourceLength + 1];
	            memset(const_cast<char*>(m_pSoap->proxy_host), 0, (sourceLength + 1) * sizeof(char));
	            strncpy(const_cast<char*>(m_pSoap->proxy_host), m_szProxyHost.c_str(), sourceLength);
                const_cast<char*>(m_pSoap->proxy_host)[sourceLength] = 0;

	            m_pSoap->proxy_port = m_nProxyPort;

                sourceLength = m_szProxyUserID.length();
                m_pSoap->proxy_userid = new char[sourceLength + 1];
	            memset(const_cast<char*>(m_pSoap->proxy_userid), 0, (sourceLength + 1) * sizeof(char));
	            strncpy(const_cast<char*>(m_pSoap->proxy_userid), m_szProxyUserID.c_str(), sourceLength);
                const_cast<char*>(m_pSoap->proxy_userid)[sourceLength] = 0;

                sourceLength = m_szProxyPassword.length();
                m_pSoap->proxy_passwd = new char[sourceLength + 1];
	            memset(const_cast<char*>(m_pSoap->proxy_passwd), 0, (sourceLength + 1) * sizeof(char));
	            strncpy(const_cast<char*>(m_pSoap->proxy_passwd), m_szProxyPassword.c_str(), sourceLength);	            
                const_cast<char*>(m_pSoap->proxy_passwd)[sourceLength] = 0;
	        }
	    }
    }
    
};

///////////////////////////////////////////////////////////////////////
class DiomedeStorageService : public DiomedeStorage
{
protected:
    char*                   m_szSettingBuffer;
    bool                    m_bUseOpenSSL;
    
    DiomedeServiceConfig    m_serviceConfig;

public:
    //-----------------------------------------------------------------
    // Default constructor
    //-----------------------------------------------------------------
    DiomedeStorageService() : DiomedeStorage()
    {
        m_szSettingBuffer = NULL;
        m_bUseOpenSSL = false;

        SetupStorageService();
        
        m_serviceConfig.Configure(soap);
        
    } // End constructor

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    DiomedeStorageService(bool bUseOpenSSL) : DiomedeStorage()
    {
        m_szSettingBuffer = NULL;
        m_bUseOpenSSL = bUseOpenSSL;

        if (m_bUseOpenSSL) {
            SetupSecureStorageService();
        }
        else {
            SetupStorageService();
        }

        m_serviceConfig.Configure(soap);

    } // End constructor

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    void SetupStorageService()
    {
        std::string szSetting = _T("");
        bool bUseDefaultEndpoint = true;

        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), false );
        if (pProfileData) {
            bUseDefaultEndpoint = (pProfileData->GetUserProfileInt(GEN_SERVICE_USE_DEFAULT_ENDPOINT,
                                                                   GEN_SERVICE_USE_DEFAULT_ENDPOINT_DF) == 1);

            // With 1.1, it became necessary to allow testing against the old service but at
            // the same time ensure user's are using the new endpoints.
            if (bUseDefaultEndpoint && (0 != stricmp(szSetting.c_str(), GEN_SERVICE_ENDPOINT_DF))) {
                szSetting = GEN_SERVICE_ENDPOINT_DF;
        	    pProfileData->SetUserProfileStr(GEN_SERVICE_ENDPOINT, GEN_SERVICE_ENDPOINT_DF );
        	    pProfileData->SaveUserProfile();
            }
            else {
                szSetting = pProfileData->GetUserProfileStr(GEN_SERVICE_ENDPOINT,
                    GEN_SERVICE_ENDPOINT_DF);
            }
        }

        if (szSetting.length() > 0) {
            m_szSettingBuffer = new char[szSetting.length() + 1];
	        strcpy(m_szSettingBuffer, szSetting.c_str());
            endpoint = m_szSettingBuffer;
        }
        else {
            #ifdef LOCAL_SERVICE
    	        endpoint = _T("http://reveal-deploy/DiomedeServices/DiomedeStorageService.svc");
	        #else
	            endpoint = _T("http://service.diomedestorage.com/1.1/DiomedeStorageService.svc");
	        #endif
	    }

    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    void SetupSecureStorageService()
    {
        std::string szSetting = _T("");
        bool bUseDefaultEndpoint = true;

        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), false );
        if (pProfileData) {
            bUseDefaultEndpoint = (pProfileData->GetUserProfileInt(GEN_SERVICE_USE_DEFAULT_ENDPOINT,
                                                                   GEN_SERVICE_USE_DEFAULT_ENDPOINT_DF) == 1);
            // With 1.1, it became necessary to allow testing against the old service but at
            // the same time ensure user's are using the new endpoints.
            if (bUseDefaultEndpoint && (0 != stricmp(szSetting.c_str(), GEN_SECURE_SERVICE_ENDPOINT_DF))) {
                szSetting = GEN_SECURE_SERVICE_ENDPOINT_DF;
        	    pProfileData->SetUserProfileStr(GEN_SECURE_SERVICE_ENDPOINT, GEN_SECURE_SERVICE_ENDPOINT_DF );
        	    pProfileData->SaveUserProfile();
            }
            else {
                szSetting = pProfileData->GetUserProfileStr(GEN_SECURE_SERVICE_ENDPOINT,
                    GEN_SECURE_SERVICE_ENDPOINT_DF);
            }
        }

        if (szSetting.length() > 0) {
            m_szSettingBuffer = new char[szSetting.length() + 1];
	        strcpy(m_szSettingBuffer, szSetting.c_str());
            endpoint = m_szSettingBuffer;
        }
        else {
            #ifdef LOCAL_SERVICE
    	        endpoint = _T("http://reveal-deploy/DiomedeServices/DiomedeStorageService.svc");
	        #else
	            endpoint = _T("https://service.diomedestorage.com/1.1/DiomedeStorageService.svc");
	        #endif
	    }
    }

    virtual ~DiomedeStorageService()
    {
        if (m_szSettingBuffer != NULL) {
            delete m_szSettingBuffer;
            m_szSettingBuffer = NULL;
        }

        if (m_bUseOpenSSL) {
            CryptoThreadCleanup();
        }
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    int SetupSecureContext()
    {

        if (m_bUseOpenSSL == false) {
            return SOAP_OK;
        }

        soap_ssl_init();

        int nResult = CryptoThreadSetup();
        if (nResult != SOAP_OK) {
            return nResult;
        }

        // On Vista, Release mode, SSL can't find the PEM
        // file unless we set the current working directory.
        std::string szCurrentDir = _T("");
        Util::GetWorkingDirectory(szCurrentDir);

        std::string szDataPath = _T("");

        // First look for the PEM file locally, otherwise, we'll look for the
        // file in the Diomede data folder.  The local look is first to
        // handle those applications where the PEM file resides in the same
        // folder as the application.

        if ( true == Util::DoesFileExist(m_szCertPem.c_str())) {
            Util::SetWorkingDirectory(szCurrentDir.c_str());
        }
        else if ( true == Util::GetDataDirectory(szDataPath)) {
            Util::SetWorkingDirectory(szDataPath.c_str());
        }

        // If using SSL, set up the context at this point.
        nResult = soap_ssl_client_context(soap,
            SOAP_SSL_DEFAULT,
                        // use SOAP_SSL_DEFAULT in production code
                        // (SOAP_SSL_NO_AUTHENTICATION | SOAP_SSL_SKIP_HOST_CHECK otherwise)
            NULL,       // keyfile: required only when client must authenticate to
                        // server (see SSL docs on how to obtain this file)
            NULL,       // password to read the keyfile
            m_szCertPem.c_str(),
                        // optional cacert file to store trusted certificates
            NULL,       // optional capath to directory with trusted certificates
            NULL        // if randfile!=NULL: use a file with random data to seed randomness
            );

        // Reset the current working directory
        if ( szCurrentDir.length() > 0) {
            Util::SetWorkingDirectory(szCurrentDir.c_str());
        }

	    return nResult;
    }

};

///////////////////////////////////////////////////////////////////////
class DiomedeTransferService  : public DiomedeStorageTransfer
{
protected:
    char*                       m_szSettingBuffer;
    struct SOAP_ENV__Header*    m_pHeader;              // Private copy of the soap header
                                                        // allocated durin upload.  We need
                                                        // our own copy of the allocated
                                                        // data - soap NULLs the pointer
                                                        // following transmission of the
                                                        // message.
    bool                        m_bUseOpenSSL;
    DiomedeServiceConfig        m_serviceConfig;

public:
    //-----------------------------------------------------------------
    // Default constructor
    //-----------------------------------------------------------------
    DiomedeTransferService() : DiomedeStorageTransfer()
    {
        m_pHeader = NULL;
        m_szSettingBuffer = NULL;
        m_bUseOpenSSL = false;

        SetupTransferService();

        m_serviceConfig.Configure(soap);

    } // End constructor

    //-----------------------------------------------------------------
    // Default constructor
    //-----------------------------------------------------------------
    DiomedeTransferService(bool bUseOpenSSL) : DiomedeStorageTransfer()
    {
        m_pHeader = NULL;
        m_szSettingBuffer = NULL;
        m_bUseOpenSSL = bUseOpenSSL;

        if (m_bUseOpenSSL) {
            SetupSecureTransferService();
        }
        else {
            SetupTransferService();
        }

        m_serviceConfig.Configure(soap);

    } // End constructor

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    void SetupTransferService()
    {
        std::string szSetting = _T("");
        bool bUseDefaultEndpoint = true;

        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), false );
        if (pProfileData) {
            bUseDefaultEndpoint = (pProfileData->GetUserProfileInt(GEN_SERVICE_USE_DEFAULT_ENDPOINT,
                                                                   GEN_SERVICE_USE_DEFAULT_ENDPOINT_DF) == 1);

            // With 1.1, it became necessary to allow testing against the old service but at
            // the same time ensure user's are using the new endpoints.
            if (bUseDefaultEndpoint && (0 != stricmp(szSetting.c_str(), GEN_TRANSFER_ENDPOINT_DF))) {
                szSetting = GEN_TRANSFER_ENDPOINT_DF;
        	    pProfileData->SetUserProfileStr(GEN_TRANSFER_ENDPOINT, GEN_TRANSFER_ENDPOINT_DF );
        	    pProfileData->SaveUserProfile();
            }
            else {
                szSetting = pProfileData->GetUserProfileStr(GEN_TRANSFER_ENDPOINT,
                    GEN_TRANSFER_ENDPOINT_DF);
            }
        }

        if (szSetting.length() > 0) {
            m_szSettingBuffer = new char[szSetting.length() + 1];
	        strcpy(m_szSettingBuffer, szSetting.c_str());
            endpoint = m_szSettingBuffer;
        }
        else {
            #ifdef LOCAL_SERVICE
    	        endpoint = _T("http://reveal-deploy/DiomedeServices/DiomedeStorageTransfer.svc");
	        #else
	            endpoint = _T("http://transfer.diomedestorage.com/1.1/DiomedeStorageTransfer.svc");
	        #endif
	    }

    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    void SetupSecureTransferService()
    {
        std::string szSetting = _T("");
        bool bUseDefaultEndpoint = true;

        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), false );
        if (pProfileData) {
            bUseDefaultEndpoint = (pProfileData->GetUserProfileInt(GEN_SERVICE_USE_DEFAULT_ENDPOINT,
                                                                   GEN_SERVICE_USE_DEFAULT_ENDPOINT_DF) == 1);

            // With 1.1, it became necessary to allow testing against the old service but at
            // the same time ensure user's are using the new endpoints.
            if (bUseDefaultEndpoint && (0 != stricmp(szSetting.c_str(), GEN_SECURE_TRANSFER_ENDPOINT_DF))) {
                szSetting = GEN_SECURE_TRANSFER_ENDPOINT_DF;
        	    pProfileData->SetUserProfileStr(GEN_SECURE_TRANSFER_ENDPOINT, GEN_SECURE_TRANSFER_ENDPOINT_DF );
        	    pProfileData->SaveUserProfile();
            }
            else {
                szSetting = pProfileData->GetUserProfileStr(GEN_SECURE_TRANSFER_ENDPOINT,
                    GEN_SECURE_TRANSFER_ENDPOINT_DF);
            }
        }

        if (szSetting.length() > 0) {
            m_szSettingBuffer = new char[szSetting.length() + 1];
	        strcpy(m_szSettingBuffer, szSetting.c_str());
            endpoint = m_szSettingBuffer;
        }
        else {
            #ifdef LOCAL_SERVICE
    	        endpoint = _T("http://reveal-deploy/DiomedeServices/DiomedeStorageTransfer.svc");
	        #else
	            endpoint = _T("https://transfer.diomedestorage.com/1.1/DiomedeStorageTransfer.svc");
	        #endif
	    }
    }

    //-----------------------------------------------------------------
    // Purpose: Allocate the soap header.  Caller is responsible for
    //          allocating and setting up the attributes of this
    //          structure.
    // Requires: nothing
    // Returns: reference to the allocated header structure
    //-----------------------------------------------------------------
    struct SOAP_ENV__Header* CreateHeader()
    {
        m_pHeader = (SOAP_ENV__Header *)soap_malloc(this->soap,
                                      sizeof(SOAP_ENV__Header));
        return m_pHeader;
    }

    //-----------------------------------------------------------------
    // Purpose: Deallocates the allocated the soap header.  Any attributes
    //          allocated will be deallocated.
    // Requires: nothing
    // Returns: nothing
    //-----------------------------------------------------------------
	void DeleteHeaderData()
	{
        if (m_pHeader == NULL) {
           return;
        }

        if (m_pHeader->tds__sessionToken != NULL) {
            delete m_pHeader->tds__sessionToken;
            m_pHeader->tds__sessionToken = NULL;
        }

        if (m_pHeader->tds__uploadInfo != NULL) {
            delete m_pHeader->tds__uploadInfo;
            m_pHeader->tds__uploadInfo = NULL;
        }

        dds__UploadInfo* uploadFileInfo = m_pHeader->tds__uploadInfo;

        if (uploadFileInfo != NULL) {

            if (uploadFileInfo->fileID != NULL) {
                delete uploadFileInfo->fileID;
                uploadFileInfo->fileID = NULL;
            }

            if (uploadFileInfo->bufferLength != NULL) {
                delete uploadFileInfo->bufferLength;
                uploadFileInfo->bufferLength = NULL;
            }

            if (uploadFileInfo->offset != NULL) {
                delete uploadFileInfo->offset;
                uploadFileInfo->offset = NULL;
            }

            if (uploadFileInfo->isComplete != NULL) {
                delete uploadFileInfo->isComplete;
                uploadFileInfo->isComplete = NULL;
            }

            delete uploadFileInfo;
            uploadFileInfo = NULL;
        }

        dds__UploadWithCreateFileInfo* pUploadWithCreateFileInfo = m_pHeader->tds__uploadWithCreateFileInfo;

        if (pUploadWithCreateFileInfo != NULL) {

            if (pUploadWithCreateFileInfo->totalLength != NULL) {
                delete pUploadWithCreateFileInfo->totalLength;
                pUploadWithCreateFileInfo->totalLength = NULL;
            }

            if (pUploadWithCreateFileInfo->fileName != NULL) {
                delete pUploadWithCreateFileInfo->fileName;
                pUploadWithCreateFileInfo->fileName = NULL;
            }

            if (pUploadWithCreateFileInfo->callbackAddress != NULL) {
                delete pUploadWithCreateFileInfo->callbackAddress;
                pUploadWithCreateFileInfo->callbackAddress = NULL;
            }

            if (pUploadWithCreateFileInfo->hash != NULL) {
                delete pUploadWithCreateFileInfo->hash;
                pUploadWithCreateFileInfo->hash = NULL;
            }

            delete pUploadWithCreateFileInfo;
            pUploadWithCreateFileInfo = NULL;

        }


        soap_dealloc(this->soap, m_pHeader);
        m_pHeader = NULL;
	}

    //-----------------------------------------------------------------
    // Destructor
    //-----------------------------------------------------------------
    virtual ~DiomedeTransferService()
    {
        DeleteHeaderData();

        if (m_szSettingBuffer != NULL) {
            delete m_szSettingBuffer;
            m_szSettingBuffer = NULL;
        }

        if (m_bUseOpenSSL) {
            CryptoThreadCleanup();
        }
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    int SetupSecureContext()
    {
        if (m_bUseOpenSSL == false) {
            return SOAP_OK;
        }

        soap_ssl_init();

        int nResult = CryptoThreadSetup();
        if (nResult != SOAP_OK) {
            return nResult;
        }

        // On Vista, Release mode, SSL can't find the PEM
        // file unless we set the current working directory.
        std::string szCurrentDir = _T("");
        Util::GetWorkingDirectory(szCurrentDir);

        std::string szDataPath = _T("");
        if ( true == Util::GetDataDirectory(szDataPath)) {
            Util::SetWorkingDirectory(szDataPath.c_str());
        }

        // If using SSL, set up the context at this point.
        nResult = soap_ssl_client_context(soap,
            SOAP_SSL_DEFAULT,
                        // use SOAP_SSL_DEFAULT in production code
                        // (SOAP_SSL_NO_AUTHENTICATION | SOAP_SSL_SKIP_HOST_CHECK otherwise)
            NULL,       // keyfile: required only when client must authenticate to
                        // server (see SSL docs on how to obtain this file)
            NULL,       // password to read the keyfile
            m_szCertPem.c_str(),
                        // optional cacert file to store trusted certificates
            NULL,       // optional capath to directory with trusted certificates
            NULL        // if randfile!=NULL: use a file with random data to seed randomness
            );

        // Reset the current working directory
        if ( szCurrentDir.length() > 0) {
            Util::SetWorkingDirectory(szCurrentDir.c_str());
        }

	    return nResult;
    }

};
#endif // __DIOMEDE_STORAGE_H__
