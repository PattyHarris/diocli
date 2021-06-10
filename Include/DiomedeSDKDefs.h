/*********************************************************************
 * 
 *  file:  DiomedeSDKDefs.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Defines all constants and enums used by the Diomede
 *          service interfaces.
 * 
 *********************************************************************/
#pragma once

namespace DIOMEDE {

/////////////////////////////////////////////////////////////////////////////
// Constants

const int DEFAULT_OFFSET            = 0;
const int DEFAULT_PAGE_SIZE         = 1000;

const int SECURE_SERVICE_PARTIAL    = 0;
const int SECURE_SERVICE_ALL        = 1;
const int SECURE_SERVICE_NONE       = 2;

/////////////////////////////////////////////////////////////////////////////
// Enumerations

    typedef enum BoolStatus { boolNull   = 0x00000001,
                              boolTrue   = 0x00000002,
                              boolFalse  = 0x00000004
    } BoolStatusType;

    typedef enum UploadFileStatus { uploadCreateFile            = 0x00000001,
                                    uploadCreateFileComplete    = 0x00000002,
                                    uploadStarted               = 0x00000004,
                                    uploadReading               = 0x00000008,
                                    uploadSending               = 0x00000010,
                                    uploadSendComplete          = 0x00000011,
                                    uploadComplete              = 0x00000012,
                                    uploadError                 = 0x00000014,
                                    uploadCancelled             = 0x00000018
                                  } UploadFileStatusType;

    typedef enum DownloadFileStatus { downloadStarted                   = 0x00000001,
                                      downloadGetDownloadURL            = 0x00000002,
                                      downloadGetDownloadURLComplete    = 0x00000004,
                                      downloadReceiving                 = 0x00000008,
                                      downloadComplete                  = 0x00000010,
                                      downloadError                     = 0x00000011,
                                      downloadCancelled                 = 0x00000012
                                    } DownloadFileStatusType;

    typedef enum DisplayFileStatus { displayFileStarted                 = 0x00000001,
                                     displayFileGetDownloadURL          = 0x00000002,
                                     displayFileGetDownloadURLComplete  = 0x00000004,
                                     displayFileDisplaying              = 0x00000008,
                                     displayFileComplete                = 0x00000010,
                                     displayFileError                   = 0x00000011,
                                     displayFileCancelled               = 0x00000012
                                } DisplayFileStatusType;

    typedef enum InvoiceStatus { unPaid = 0x00000001,
                                 paid   = 0x00000002
                               } InvoiceStatusType;


    typedef enum PaymentMethods { paymentUndefined  = 0x00000000,
                                  creditCard        = 0x00000001,
                                  manual            = 0x00000002
                                } PaymentMethodsType;


    typedef enum UserAccount { accountUndefined = 0x00000000,
                               freeAccount      = 0x00000001,
                               paidAccount      = 0x00000002
                             } UserAccountType;


    typedef enum UserInfo { companyName       = 0x00000001,
                            websiteUrl        = 0x00000002,
                            lastName          = 0x00000004,
                            firstName         = 0x00000008,
                            phone             = 0x00000010, // 16,
                            cardName          = 0x00000020, // 32,
                            cardNumber        = 0x00000040, // 64,
                            cardExpiry        = 0x00000080, // 128,
                            cardCvv2          = 0x00000100, // 256,
                            cardAddress1      = 0x00000200, // 512,
                            cardAddress2      = 0x00000400, // 1024,
                            cardCity          = 0x00000800, // 2048,
                            cardState         = 0x00001000, // 4096,
                            cardZip           = 0x00002000, // 8192,
                            cardCountry       = 0x00004000, // 16384,
                            allUserInfo       = 0x00008000, // 32768,
                            allBillingInfo    = 0x00010000, // 65536,
                            cardExpiryMonth   = 0x00020000  // 131072
                          } UserInfoType;

} // End namespace
