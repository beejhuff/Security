/*
 * Copyright (c) 2002-2016 Apple Inc. All Rights Reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include <Security/SecCertificate.h>
#include <Security/SecCertificatePriv.h>
#include <security_keychain/Certificate.h>
#include <security_keychain/Item.h>
#include <security_keychain/KCCursor.h>
#include <Security/cssmapi.h>
#include <Security/cssmapple.h>
#include <security_cdsa_client/cspclient.h>
#include <security_cdsa_client/clclient.h>
#include <security_cdsa_client/tpclient.h>
#include <Security/cssmtype.h>

#include "SecBridge.h"

// %%% used by SecCertificate{Copy,Set}Preference
#include <Security/SecKeychainItemPriv.h>
#include <Security/SecIdentityPriv.h>
#include <Security/SecItemPriv.h>
#include <security_keychain/KCCursor.h>
#include <security_cdsa_utilities/Schema.h>
#include <security_cdsa_utils/cuCdsaUtils.h>
#include <sys/param.h>
#include <syslog.h>
#include "CertificateValues.h"
#include "SecCertificateP.h"
#include "SecCertificatePrivP.h"

#include "AppleBaselineEscrowCertificates.h"


OSStatus SecCertificateGetCLHandle_legacy(SecCertificateRef certificate, CSSM_CL_HANDLE *clHandle);
extern CSSM_KEYUSE ConvertArrayToKeyUsage(CFArrayRef usage);

#define SEC_CONST_DECL(k,v) const CFStringRef k = CFSTR(v);

SEC_CONST_DECL (kSecCertificateProductionEscrowKey, "ProductionEscrowKey");
SEC_CONST_DECL (kSecCertificateProductionPCSEscrowKey, "ProductionPCSEscrowKey");
SEC_CONST_DECL (kSecCertificateEscrowFileName, "AppleESCertificates");


using namespace CssmClient;

CFTypeID
#if SECTRUST_OSX
static SecCertificateGetTypeID_osx(void)
#else
SecCertificateGetTypeID(void)
#endif
{
	BEGIN_SECAPI

	return gTypes().Certificate.typeID;

	END_SECAPI1(_kCFRuntimeNotATypeID)
}

Boolean
SecCertificateIsItemImplInstance(SecCertificateRef certificate)
{
    if (certificate == NULL) {
        return false;
    }
#if !SECTRUST_OSX
    return true;
#else
    CFTypeID typeID = CFGetTypeID(certificate);

#if 0 /* debug code to verify type IDs */
	syslog(LOG_ERR, "SecCertificate typeID=%d [STU=%d, OSX=%d, SKI=%d]",
			(int)typeID,
			(int)SecCertificateGetTypeID(),
			(int)SecCertificateGetTypeID_osx(),
			(int)SecKeychainItemGetTypeID());
#endif
	if (typeID == _kCFRuntimeNotATypeID) {
		return false;
	}

    return (typeID == SecCertificateGetTypeID_osx() ||
            typeID == SecKeychainItemGetTypeID()) ? true : false;
#endif
}

/* convert a new-world SecCertificateRef to an old-world ItemImpl instance */
SecCertificateRef
SecCertificateCreateItemImplInstance(SecCertificateRef certificate)
{
#if !SECTRUST_OSX
    return (SecCertificateRef)(certificate ? CFRetain(certificate) : NULL);
#else
	if (!certificate) {
		return NULL;
	}
	SecCertificateRef implCertRef = (SecCertificateRef) SecCertificateCopyKeychainItem(certificate);
	if (implCertRef) {
		return implCertRef;
	}
	CFDataRef data = SecCertificateCopyData(certificate);
	if (!data) {
		return NULL;
	}
	try {
		CSSM_DATA cssmCertData;
		cssmCertData.Length = (data) ? (CSSM_SIZE)CFDataGetLength(data) : 0;
		cssmCertData.Data = (data) ? (uint8 *)CFDataGetBytePtr(data) : NULL;

		SecPointer<Certificate> certificatePtr(new Certificate(cssmCertData, CSSM_CERT_X_509v3, CSSM_CERT_ENCODING_DER));
		implCertRef = certificatePtr->handle();
	}
	catch (...) {}
	CFRelease(data);
	return implCertRef;
#endif
}

/* convert an old-world ItemImpl instance to a new-world SecCertificateRef */
SecCertificateRef
SecCertificateCreateFromItemImplInstance(SecCertificateRef certificate)
{
#if !SECTRUST_OSX
    return (SecCertificateRef)(certificate ? CFRetain(certificate) : NULL);
#else
	if (!certificate) {
		return NULL;
	}
	SecCertificateRef result = NULL;
	CFDataRef data = NULL;
	try {
		CssmData certData = Certificate::required(certificate)->data();
		if (certData.Data && certData.Length) {
			data = CFDataCreate(NULL, certData.Data, certData.Length);
		}
		if (!data) {
			if (certData.Data && !certData.Length) {
				/* zero-length certs can exist, so don't bother logging this */
			}
			else {
				syslog(LOG_ERR, "WARNING: SecKeychainSearchCopyNext failed to retrieve certificate data (length=%ld, data=0x%lX)",
						(long)certData.Length, (uintptr_t)certData.Data);
			}
			return NULL;
		}
	}
	catch (...) {}

	result = SecCertificateCreateWithKeychainItem(NULL, data, certificate);
	if (data)
		CFRelease(data);
	return result;
#endif
}

#if !SECTRUST_OSX
OSStatus
SecCertificateSetKeychainItem(SecCertificateRef certificate, CFTypeRef keychain_item)
{
	// pre-STU, this function is a no-op since it's the same item reference
	return errSecSuccess;
}
#endif

#if !SECTRUST_OSX
CFTypeRef
SecCertificateCopyKeychainItem(SecCertificateRef certificate)
{
	if (certificate) {
		CFRetain(certificate);
	}
	return certificate;
}
#endif

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateCreateFromData(const CSSM_DATA *data, CSSM_CERT_TYPE type, CSSM_CERT_ENCODING encoding, SecCertificateRef *certificate)
{
#if !SECTRUST_OSX
	BEGIN_SECAPI

	SecPointer<Certificate> certificatePtr(new Certificate(Required(data), type, encoding));
	Required(certificate) = certificatePtr->handle();

	END_SECAPI
#else
	/* bridge to support old functionality */
	if (!data || !data->Data || !data->Length || !certificate) {
		return errSecParam;
	}
	SecCertificateRef certRef = NULL;

    // <rdar://problem/24403998> REG: Adobe {Photoshop, InDesign} CC(2015) crashes on launch
    // If you take the length that SecKeychainItemCopyContent gives you (a Uint32) and assign it incorrectly
    // to a CSSM_DATA Length field (a CSSM_SIZE, i.e., a size_t), the upper 32 bits aren't set. If those bits
    // are non-zero, the length is incredibly wrong.
    //
    // Assume that there will not exist a certificate > 4GiB, and fake this length field.
    CSSM_SIZE length = data->Length & 0xfffffffful;

	CFDataRef dataRef = CFDataCreate(NULL, data->Data, length);
	if (dataRef) {
		certRef = SecCertificateCreateWithData(NULL, dataRef);
		CFRelease(dataRef);
	}
	*certificate = certRef;
	return (certRef) ? errSecSuccess : errSecUnknownFormat;
#endif
}

#if !SECTRUST_OSX
/* new in 10.6 */
SecCertificateRef
SecCertificateCreateWithData(CFAllocatorRef allocator, CFDataRef data)
{
	SecCertificateRef certificate = NULL;
    OSStatus __secapiresult;
	try {
		CSSM_DATA cssmCertData;
		cssmCertData.Length = (data) ? (CSSM_SIZE)CFDataGetLength(data) : 0;
		cssmCertData.Data = (data) ? (uint8 *)CFDataGetBytePtr(data) : NULL;

		//NOTE: there isn't yet a Certificate constructor which accepts a CFAllocatorRef
		SecPointer<Certificate> certificatePtr(new Certificate(cssmCertData, CSSM_CERT_X_509v3, CSSM_CERT_ENCODING_DER));
		certificate = certificatePtr->handle();

		__secapiresult=errSecSuccess;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
    return certificate;
}
#endif

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_3, __IPHONE_NA) */
OSStatus
SecCertificateAddToKeychain(SecCertificateRef certificate, SecKeychainRef keychain)
{
	// This macro creates an ItemImpl certificate if it does not exist
	BEGIN_SECCERTAPI

	Item item(Certificate::required(__itemImplRef));
	Keychain::optional(keychain)->add(item);

	END_SECCERTAPI
}

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateGetData(SecCertificateRef certificate, CSSM_DATA_PTR data)
{
#if !SECTRUST_OSX
	BEGIN_SECAPI

	Required(data) = Certificate::required(certificate)->data();

	END_SECAPI
#else
	BEGIN_SECCERTAPI

	if (!certificate || !data) {
		__secapiresult=errSecParam;
	}
	else if (SecCertificateIsItemImplInstance(certificate)) {
		Required(data) = Certificate::required(certificate)->data();
	}
	else {
		data->Length = (CSSM_SIZE)SecCertificateGetLength(certificate);
		data->Data = (uint8*)SecCertificateGetBytePtr(certificate);
	}

	END_SECCERTAPI
#endif
}

#if !SECTRUST_OSX
/* new in 10.6 */
CFDataRef
SecCertificateCopyData(SecCertificateRef certificate)
{
	CFDataRef data = NULL;
	OSStatus __secapiresult = errSecSuccess;
	try {
		CssmData output = Certificate::required(certificate)->data();
		CFIndex length = (CFIndex)output.length();
		const UInt8 *bytes = (const UInt8 *)output.data();
		if (length && bytes) {
			data = CFDataCreate(NULL, bytes, length);
		}
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return data;
}
#endif

#if !SECTRUST_OSX
/* new in 10.12 */
CFDataRef
SecCertificateCopySHA256Digest(SecCertificateRef certificate)
{
    CFDataRef data = NULL;
    OSStatus __secapiresult = errSecSuccess;
    try {
        data = Certificate::required(certificate)->sha256Hash();
        if (data) {
            CFRetain(data);
        }
    }
    catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
    catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
    catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
    catch (...) { __secapiresult=errSecInternalComponent; }
    return data;
}
#endif

#if !SECTRUST_OSX
CFDataRef
SecCertificateGetSHA1Digest(SecCertificateRef certificate)
{
	CFDataRef data = NULL;
    OSStatus __secapiresult = errSecSuccess;
	try {
		data = Certificate::required(certificate)->sha1Hash();
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
    return data;
}
#endif

#if !SECTRUST_OSX
CFDataRef
SecCertificateCopyPublicKeySHA1Digest(SecCertificateRef certificate)
{
    CFDataRef data = NULL;
    OSStatus __secapiresult = errSecSuccess;
    try {
        CssmData output = Certificate::required(certificate)->publicKeyHash();
        CFIndex length = (CFIndex)output.length();
        const UInt8 *bytes = (const UInt8 *)output.data();
        if (length && bytes) {
            data = CFDataCreate(NULL, bytes, length);
        }
    }
    catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
    catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
    catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
    catch (...) { __secapiresult=errSecInternalComponent; }
    return data;
}
#endif

#if !SECTRUST_OSX
CFArrayRef
SecCertificateCopyDNSNames(SecCertificateRef certificate)
{
	CFArrayRef names = NULL;
	OSStatus __secapiresult = errSecSuccess;
	try {
		names = Certificate::required(certificate)->copyDNSNames();
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
    return names;
}
#endif

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateGetType(SecCertificateRef certificate, CSSM_CERT_TYPE *certificateType)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(certificateType) = Certificate::required(__itemImplRef)->type();

    END_SECCERTAPI
}

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateGetSubject(SecCertificateRef certificate, const CSSM_X509_NAME **subject)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(subject) = Certificate::required(__itemImplRef)->subjectName();

    END_SECCERTAPI
}

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateGetIssuer(SecCertificateRef certificate, const CSSM_X509_NAME **issuer)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(issuer) = Certificate::required(__itemImplRef)->issuerName();

    END_SECCERTAPI
}

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateGetCLHandle(SecCertificateRef certificate, CSSM_CL_HANDLE *clHandle)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(clHandle) = Certificate::required(__itemImplRef)->clHandle();

    END_SECCERTAPI
}

/* private function; assumes input is old-style ItemImpl certificate reference,
   and does not release that certificate reference!
 */
OSStatus
SecCertificateGetCLHandle_legacy(SecCertificateRef certificate, CSSM_CL_HANDLE *clHandle)
{
    BEGIN_SECAPI

    Required(clHandle) = Certificate::required(certificate)->clHandle();

    END_SECAPI
}

/*
 * Private API to infer a display name for a SecCertificateRef which
 * may or may not be in a keychain.
 *
 * OS X only
 */
OSStatus
SecCertificateInferLabel(SecCertificateRef certificate, CFStringRef *label)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Certificate::required(__itemImplRef)->inferLabel(false, &Required(label));

    END_SECCERTAPI
}

/* OS X only (note: iOS version has different arguments and return value) */
OSStatus
SecCertificateCopyPublicKey(SecCertificateRef certificate, SecKeyRef *key)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(key) = Certificate::required(__itemImplRef)->publicKey()->handle();

    END_SECCERTAPI
}

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateGetAlgorithmID(SecCertificateRef certificate, const CSSM_X509_ALGORITHM_IDENTIFIER **algid)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(algid) = Certificate::required(__itemImplRef)->algorithmID();

    END_SECCERTAPI
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_NA) */
OSStatus
SecCertificateCopyCommonName(SecCertificateRef certificate, CFStringRef *commonName)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(commonName) = Certificate::required(__itemImplRef)->commonName();

    END_SECCERTAPI
}

#if !SECTRUST_OSX
/* new in 10.6 */
CFStringRef
SecCertificateCopySubjectSummary(SecCertificateRef certificate)
{
	CFStringRef summary = NULL;
	OSStatus __secapiresult;
	try {
		Certificate::required(certificate)->inferLabel(false, &summary);

		__secapiresult=errSecSuccess;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return summary;
}
#endif

#if !SECTRUST_OSX
CFStringRef
SecCertificateCopyIssuerSummary(SecCertificateRef certificate)
{
	CFStringRef issuerStr = NULL;
	SecCertificateRefP certP = NULL;
	CFDataRef certData = SecCertificateCopyData(certificate);
	if (certData) {
		certP = SecCertificateCreateWithDataP(NULL, certData);
		CFRelease(certData);
	}
	if (certP) {
		issuerStr = SecCertificateCopyIssuerSummaryP(certP);
		CFRelease(certP);
	}
	return issuerStr;
}
#endif

/* OS X only */
OSStatus
SecCertificateCopySubjectComponent(SecCertificateRef certificate, const CSSM_OID *component, CFStringRef *result)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(result) = Certificate::required(__itemImplRef)->distinguishedName(&CSSMOID_X509V1SubjectNameCStruct, component);

    END_SECCERTAPI
}

/* OS X only; deprecated SPI */
OSStatus
SecCertificateGetCommonName(SecCertificateRef certificate, CFStringRef *commonName)
{
    // deprecated SPI signature; replaced by SecCertificateCopyCommonName
    return SecCertificateCopyCommonName(certificate, commonName);
}

/* OS X only; deprecated SPI */
OSStatus
SecCertificateGetEmailAddress(SecCertificateRef certificate, CFStringRef *emailAddress)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(emailAddress) = Certificate::required(__itemImplRef)->copyFirstEmailAddress();

    END_SECCERTAPI
}

/* OS X only */
OSStatus
SecCertificateCopyEmailAddresses(SecCertificateRef certificate, CFArrayRef *emailAddresses)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(emailAddresses) = Certificate::required(__itemImplRef)->copyEmailAddresses();

    END_SECCERTAPI
}

/* Return a zero terminated list of CSSM_DATA_PTR's with the values of the field specified by field.
 * Caller must call releaseFieldValues to free the storage allocated by this call.
 *
 * OS X only
 */
OSStatus
SecCertificateCopyFieldValues(SecCertificateRef certificate, const CSSM_OID *field, CSSM_DATA_PTR **fieldValues)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(fieldValues) = Certificate::required(__itemImplRef)->copyFieldValues(Required(field));

    END_SECCERTAPI
}

/* OS X only */
OSStatus
SecCertificateReleaseFieldValues(SecCertificateRef certificate, const CSSM_OID *field, CSSM_DATA_PTR *fieldValues)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Certificate::required(__itemImplRef)->releaseFieldValues(Required(field), fieldValues);

    END_SECCERTAPI
}

/* OS X only */
OSStatus
SecCertificateCopyFirstFieldValue(SecCertificateRef certificate, const CSSM_OID *field, CSSM_DATA_PTR *fieldValue)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Required(fieldValue) = Certificate::required(__itemImplRef)->copyFirstFieldValue(Required(field));

    END_SECCERTAPI
}

/* OS X only */
OSStatus
SecCertificateReleaseFirstFieldValue(SecCertificateRef certificate, const CSSM_OID *field, CSSM_DATA_PTR fieldValue)
{
    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

    Certificate::required(__itemImplRef)->releaseFieldValue(Required(field), fieldValue);

    END_SECCERTAPI
}

/* OS X only */
OSStatus
SecCertificateFindByIssuerAndSN(CFTypeRef keychainOrArray,const CSSM_DATA *issuer,
	const CSSM_DATA *serialNumber, SecCertificateRef *certificate)
{
    if (issuer && serialNumber) {
        CFRef<CFMutableDictionaryRef> query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(query, kSecClass, kSecClassCertificate);
        CFDictionarySetValue(query, kSecReturnRef, kCFBooleanTrue);
        CFDictionarySetValue(query, kSecAttrNoLegacy, kCFBooleanTrue);

        CFRef<CFDataRef> issuerData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)issuer->Data, issuer->Length, kCFAllocatorNull);
        CFDictionarySetValue(query, kSecAttrIssuer, issuerData);

        CFRef<CFDataRef> serialNumberData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)serialNumber->Data, serialNumber->Length, kCFAllocatorNull);
        CFDictionarySetValue(query, kSecAttrSerialNumber, serialNumberData);

        OSStatus status = SecItemCopyMatching(query, (CFTypeRef*)certificate);
        if (status == errSecSuccess) {
            return status;
        }
    }

	BEGIN_SECAPI

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	Required(certificate) = Certificate::findByIssuerAndSN(keychains, CssmData::required(issuer), CssmData::required(serialNumber))->handle();

#if SECTRUST_OSX
    // convert ItemImpl-based SecCertificateRef to new-world version before returning
	CssmData certData = Certificate::required(*certificate)->data();
	CFRef<CFDataRef> cfData(CFDataCreate(NULL, certData.Data, certData.Length));
	SecCertificateRef tmpRef = *certificate;
	*certificate = SecCertificateCreateWithData(NULL, cfData);
	CFRelease(tmpRef);
#endif

	END_SECAPI
}

/* OS X only */
OSStatus
SecCertificateFindBySubjectKeyID(CFTypeRef keychainOrArray, const CSSM_DATA *subjectKeyID,
	SecCertificateRef *certificate)
{
    if (subjectKeyID) {
        CFRef<CFMutableDictionaryRef> query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(query, kSecClass, kSecClassCertificate);
        CFDictionarySetValue(query, kSecReturnRef, kCFBooleanTrue);
        CFDictionarySetValue(query, kSecAttrNoLegacy, kCFBooleanTrue);

        CFRef<CFDataRef> subjectKeyIDData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)subjectKeyID->Data, subjectKeyID->Length, kCFAllocatorNull);
        CFDictionarySetValue(query, kSecAttrSubjectKeyID, subjectKeyIDData);

        OSStatus status = SecItemCopyMatching(query, (CFTypeRef*)certificate);
        if (status == errSecSuccess) {
            return status;
        }
    }

    BEGIN_SECAPI

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	Required(certificate) = Certificate::findBySubjectKeyID(keychains, CssmData::required(subjectKeyID))->handle();

#if SECTRUST_OSX
    // convert ItemImpl-based SecCertificateRef to new-world version before returning
	CssmData certData = Certificate::required(*certificate)->data();
	CFRef<CFDataRef> cfData(CFDataCreate(NULL, certData.Data, certData.Length));
	SecCertificateRef tmpRef = *certificate;
	*certificate = SecCertificateCreateWithData(NULL, cfData);
	CFRelease(tmpRef);
#endif

	END_SECAPI
}

/* OS X only */
OSStatus
SecCertificateFindByEmail(CFTypeRef keychainOrArray, const char *emailAddress, SecCertificateRef *certificate)
{
    if (emailAddress) {
        CFRef<CFMutableDictionaryRef> query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(query, kSecClass, kSecClassCertificate);
        CFDictionarySetValue(query, kSecReturnRef, kCFBooleanTrue);
        CFDictionarySetValue(query, kSecAttrNoLegacy, kCFBooleanTrue);

        CFRef<CFStringRef> emailAddressString = CFStringCreateWithCString(kCFAllocatorDefault, emailAddress, kCFStringEncodingUTF8);
        CFTypeRef keys[] = { kSecPolicyName };
        CFTypeRef values[] = { emailAddressString };
        CFRef<CFDictionaryRef> properties = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFRef<SecPolicyRef> policy = SecPolicyCreateWithProperties(kSecPolicyAppleSMIME, properties);
        CFDictionarySetValue(query, kSecMatchPolicy, policy);

        OSStatus status = SecItemCopyMatching(query, (CFTypeRef*)certificate);
        if (status == errSecSuccess) {
            return status;
        }
    }

    BEGIN_SECAPI

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	Required(certificate) = Certificate::findByEmail(keychains, emailAddress)->handle();

#if SECTRUST_OSX
    // convert ItemImpl-based SecCertificateRef to new-world version before returning
	CssmData certData = Certificate::required(*certificate)->data();
	CFRef<CFDataRef> cfData(CFDataCreate(NULL, certData.Data, certData.Length));
	SecCertificateRef tmpRef = *certificate;
	*certificate = SecCertificateCreateWithData(NULL, cfData);
	CFRelease(tmpRef);
#endif

	END_SECAPI
}

/* OS X only */
OSStatus
SecKeychainSearchCreateForCertificateByIssuerAndSN(CFTypeRef keychainOrArray, const CSSM_DATA *issuer,
	const CSSM_DATA *serialNumber, SecKeychainSearchRef *searchRef)
{
    BEGIN_SECAPI

	Required(searchRef);

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	KCCursor cursor(Certificate::cursorForIssuerAndSN(keychains, CssmData::required(issuer), CssmData::required(serialNumber)));
	*searchRef = cursor->handle();

	END_SECAPI
}

/* OS X only */
OSStatus
SecKeychainSearchCreateForCertificateByIssuerAndSN_CF(CFTypeRef keychainOrArray, CFDataRef issuer,
	CFDataRef serialNumber, SecKeychainSearchRef *searchRef)
{
    BEGIN_SECAPI

	Required(searchRef);

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	Required(issuer);
	Required(serialNumber);
	KCCursor cursor(Certificate::cursorForIssuerAndSN_CF(keychains, issuer, serialNumber));
	*searchRef = cursor->handle();

	END_SECAPI
}

/* OS X only */
OSStatus
SecKeychainSearchCreateForCertificateBySubjectKeyID(CFTypeRef keychainOrArray, const CSSM_DATA *subjectKeyID,
	SecKeychainSearchRef *searchRef)
{
    BEGIN_SECAPI

	Required(searchRef);

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	KCCursor cursor(Certificate::cursorForSubjectKeyID(keychains, CssmData::required(subjectKeyID)));
	*searchRef = cursor->handle();

	END_SECAPI
}

/* OS X only */
OSStatus
SecKeychainSearchCreateForCertificateByEmail(CFTypeRef keychainOrArray, const char *emailAddress,
	SecKeychainSearchRef *searchRef)
{
    BEGIN_SECAPI

	Required(searchRef);

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	KCCursor cursor(Certificate::cursorForEmail(keychains, emailAddress));
	*searchRef = cursor->handle();

	END_SECAPI
}

/* OS X only */
CSSM_RETURN
SecDigestGetData (CSSM_ALGORITHMS alg, CSSM_DATA* digest, const CSSM_DATA* data)
{
	BEGIN_SECAPI
	// sanity checking
	if (!digest || !digest->Data || !digest->Length || !data || !data->Data || !data->Length)
		return errSecParam;

	CSP csp(gGuidAppleCSP);
	Digest context(csp, alg);
	CssmData input(data->Data, data->Length);
	CssmData output(digest->Data, digest->Length);

	context.digest(input, output);
	digest->Length = output.length();

	return CSSM_OK;
	END_SECAPI1(1);
}

#if !SECTRUST_OSX
/* determine whether a cert is self-signed */
OSStatus SecCertificateIsSelfSigned(
	SecCertificateRef certificate,
	Boolean *isSelfSigned)		/* RETURNED */
{
    BEGIN_SECAPI

	*isSelfSigned = Certificate::required(certificate)->isSelfSigned();

	END_SECAPI
}
#endif

/* OS X only: DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER */
OSStatus
SecCertificateCopyPreference(
    CFStringRef name,
    CSSM_KEYUSE keyUsage,
    SecCertificateRef *certificate)
{
    BEGIN_SECAPI

	Required(name);
	Required(certificate);
	StorageManager::KeychainList keychains;
	globals().storageManager.getSearchList(keychains);
	KCCursor cursor(keychains, kSecGenericPasswordItemClass, NULL);

	char idUTF8[MAXPATHLEN];
    if (!CFStringGetCString(name, idUTF8, sizeof(idUTF8)-1, kCFStringEncodingUTF8))
        idUTF8[0] = (char)'\0';
    CssmData service(const_cast<char *>(idUTF8), strlen(idUTF8));
    FourCharCode itemType = 'cprf';
    cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecServiceItemAttr), service);
	cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecTypeItemAttr), itemType);
    if (keyUsage)
        cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecScriptCodeItemAttr), (sint32)keyUsage);

	Item prefItem;
	if (!cursor->next(prefItem))
		MacOSError::throwMe(errSecItemNotFound);

	// get persistent certificate reference
	SecKeychainAttribute itemAttrs[] = { { kSecGenericItemAttr, 0, NULL } };
	SecKeychainAttributeList itemAttrList = { sizeof(itemAttrs) / sizeof(itemAttrs[0]), itemAttrs };
	prefItem->getContent(NULL, &itemAttrList, NULL, NULL);

	// find certificate, given persistent reference data
	CFDataRef pItemRef = CFDataCreateWithBytesNoCopy(NULL, (const UInt8 *)itemAttrs[0].data, itemAttrs[0].length, kCFAllocatorNull);
	SecKeychainItemRef certItemRef = nil;
	OSStatus status = SecKeychainItemCopyFromPersistentReference(pItemRef, &certItemRef); //%%% need to make this a method of ItemImpl
	prefItem->freeContent(&itemAttrList, NULL);
	if (pItemRef)
		CFRelease(pItemRef);
	if (status)
		return status;

	*certificate = (SecCertificateRef)certItemRef;

#if SECTRUST_OSX
	if (certItemRef && (CFGetTypeID(certItemRef) == SecIdentityGetTypeID())) {
		// SecKeychainItemCopyFromPersistentReference handed out an identity reference
		*certificate = NULL;
		status = SecIdentityCopyCertificate((SecIdentityRef)certItemRef, certificate);
		CFRelease(certItemRef);
		return status;
	}
#if 0  /* SecKeychainItemCopyFromPersistentReference now does this work for us */
	// convert ItemImpl-based SecCertificateRef to new-world version before returning
	CssmData certData = Certificate::required(*certificate)->data();
	CFRef<CFDataRef> cfData(CFDataCreate(NULL, certData.Data, certData.Length));
	SecCertificateRef tmpRef = *certificate;
	*certificate = SecCertificateCreateWithData(NULL, cfData);
	CFRelease(tmpRef);
#endif
#endif

    END_SECAPI
}

/* OS X only */
SecCertificateRef
SecCertificateCopyPreferred(
	CFStringRef name,
	CFArrayRef keyUsage)
{
	// This function will look for a matching preference in the following order:
	// - matches the name and the supplied key use
	// - matches the name and the special 'ANY' key use
	// - matches the name with no key usage constraint

	SecCertificateRef certRef = NULL;
	CSSM_KEYUSE keyUse = ConvertArrayToKeyUsage(keyUsage);
	OSStatus status = SecCertificateCopyPreference(name, keyUse, &certRef);
	if (status != errSecSuccess && keyUse != CSSM_KEYUSE_ANY)
		status = SecCertificateCopyPreference(name, CSSM_KEYUSE_ANY, &certRef);
	if (status != errSecSuccess && keyUse != 0)
		status = SecCertificateCopyPreference(name, 0, &certRef);

	return certRef;
}

/* OS X only; not exported */
static OSStatus
SecCertificateFindPreferenceItemWithNameAndKeyUsage(
	CFTypeRef keychainOrArray,
	CFStringRef name,
	int32_t keyUsage,
	SecKeychainItemRef *itemRef)
{
    BEGIN_SECAPI

	StorageManager::KeychainList keychains;
	globals().storageManager.optionalSearchList(keychainOrArray, keychains);
	KCCursor cursor(keychains, kSecGenericPasswordItemClass, NULL);

	char idUTF8[MAXPATHLEN];
    idUTF8[0] = (char)'\0';
	if (name)
	{
		if (!CFStringGetCString(name, idUTF8, sizeof(idUTF8)-1, kCFStringEncodingUTF8))
			idUTF8[0] = (char)'\0';
	}
    size_t idUTF8Len = strlen(idUTF8);
    if (!idUTF8Len)
        MacOSError::throwMe(errSecParam);

    CssmData service(const_cast<char *>(idUTF8), idUTF8Len);
    cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecServiceItemAttr), service);
	cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecTypeItemAttr), (FourCharCode)'cprf');
    if (keyUsage)
        cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecScriptCodeItemAttr), (sint32)keyUsage);

	Item item;
	if (!cursor->next(item))
		MacOSError::throwMe(errSecItemNotFound);

	if (itemRef)
		*itemRef=item->handle();

    END_SECAPI
}

/* OS X only; not exported */
static
OSStatus SecCertificateDeletePreferenceItemWithNameAndKeyUsage(
	CFTypeRef keychainOrArray,
	CFStringRef name,
	int32_t keyUsage)
{
	// when a specific key usage is passed, we'll only match & delete that pref;
	// when a key usage of 0 is passed, all matching prefs should be deleted.
	// maxUsages represents the most matches there could theoretically be, so
	// cut things off at that point if we're still finding items (if they can't
	// be deleted for some reason, we'd never break out of the loop.)

	OSStatus status;
	SecKeychainItemRef item = NULL;
	int count = 0, maxUsages = 12;
	while (++count <= maxUsages &&
			(status = SecCertificateFindPreferenceItemWithNameAndKeyUsage(keychainOrArray, name, keyUsage, &item)) == errSecSuccess) {
		status = SecKeychainItemDelete(item);
		CFRelease(item);
		item = NULL;
	}

	// it's not an error if the item isn't found
	return (status == errSecItemNotFound) ? errSecSuccess : status;
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_NA) */
OSStatus SecCertificateSetPreference(
    SecCertificateRef certificate,
    CFStringRef name,
    CSSM_KEYUSE keyUsage,
    CFDateRef date)
{
	if (!name) {
		return errSecParam;
	}
	if (!certificate) {
		// treat NULL certificate as a request to clear the preference
		// (note: if keyUsage is 0, this clears all key usage prefs for name)
		return SecCertificateDeletePreferenceItemWithNameAndKeyUsage(NULL, name, keyUsage);
	}

    // This macro creates an ItemImpl certificate if it does not exist
    BEGIN_SECCERTAPI

	// determine the account attribute
	//
	// This attribute must be synthesized from certificate label + pref item type + key usage,
	// as only the account and service attributes can make a generic keychain item unique.
	// For 'iprf' type items (but not 'cprf'), we append a trailing space. This insures that
	// we can save a certificate preference if an identity preference already exists for the
	// given service name, and vice-versa.
	// If the key usage is 0 (i.e. the normal case), we omit the appended key usage string.
	//
    CFStringRef labelStr = nil;
	Certificate::required(__itemImplRef)->inferLabel(false, &labelStr);
	if (!labelStr) {
        MacOSError::throwMe(errSecDataTooLarge); // data is "in a format which cannot be displayed"
	}
	CFIndex accountUTF8Len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(labelStr), kCFStringEncodingUTF8) + 1;
	const char *templateStr = "%s [key usage 0x%X]";
	const int keyUsageMaxStrLen = 8;
	accountUTF8Len += strlen(templateStr) + keyUsageMaxStrLen;
	char accountUTF8[accountUTF8Len];
    if (!CFStringGetCString(labelStr, accountUTF8, accountUTF8Len-1, kCFStringEncodingUTF8))
		accountUTF8[0] = (char)'\0';
	if (keyUsage)
		snprintf(accountUTF8, accountUTF8Len-1, templateStr, accountUTF8, keyUsage);
    CssmData account(const_cast<char *>(accountUTF8), strlen(accountUTF8));
    CFRelease(labelStr);

	// service attribute (name provided by the caller)
	CFIndex serviceUTF8Len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(name), kCFStringEncodingUTF8) + 1;;
	char serviceUTF8[serviceUTF8Len];
    if (!CFStringGetCString(name, serviceUTF8, serviceUTF8Len-1, kCFStringEncodingUTF8))
        serviceUTF8[0] = (char)'\0';
    CssmData service(const_cast<char *>(serviceUTF8), strlen(serviceUTF8));

    // look for existing preference item, in case this is an update
	StorageManager::KeychainList keychains;
	globals().storageManager.getSearchList(keychains);
	KCCursor cursor(keychains, kSecGenericPasswordItemClass, NULL);
    FourCharCode itemType = 'cprf';
    cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecServiceItemAttr), service);
	cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecTypeItemAttr), itemType);
    if (keyUsage)
        cursor->add(CSSM_DB_EQUAL, Schema::attributeInfo(kSecScriptCodeItemAttr), (sint32)keyUsage);
    if (date)
        ; // %%%TBI

	Item item(kSecGenericPasswordItemClass, 'aapl', 0, NULL, false);
    bool add = (!cursor->next(item));
	// at this point, we either have a new item to add or an existing item to update

    // set item attribute values
    item->setAttribute(Schema::attributeInfo(kSecServiceItemAttr), service);
    item->setAttribute(Schema::attributeInfo(kSecTypeItemAttr), itemType);
    item->setAttribute(Schema::attributeInfo(kSecAccountItemAttr), account);
    item->setAttribute(Schema::attributeInfo(kSecScriptCodeItemAttr), (sint32)keyUsage);
    item->setAttribute(Schema::attributeInfo(kSecLabelItemAttr), service);

    // date
    if (date)
        ; // %%%TBI

	// generic attribute (store persistent certificate reference)
	CFDataRef pItemRef = nil;
	Certificate::required(__itemImplRef)->copyPersistentReference(pItemRef);
	if (!pItemRef) {
		MacOSError::throwMe(errSecInvalidItemRef);
    }
	const UInt8 *dataPtr = CFDataGetBytePtr(pItemRef);
	CFIndex dataLen = CFDataGetLength(pItemRef);
	CssmData pref(const_cast<void *>(reinterpret_cast<const void *>(dataPtr)), dataLen);
	item->setAttribute(Schema::attributeInfo(kSecGenericItemAttr), pref);
	CFRelease(pItemRef);

    if (add) {
        Keychain keychain = nil;
        try {
            keychain = globals().storageManager.defaultKeychain();
            if (!keychain->exists())
                MacOSError::throwMe(errSecNoSuchKeychain);	// Might be deleted or not available at this time.
        }
        catch(...) {
            keychain = globals().storageManager.defaultKeychainUI(item);
        }

		try {
			keychain->add(item);
		}
		catch (const MacOSError &err) {
			if (err.osStatus() != errSecDuplicateItem)
				throw; // if item already exists, fall through to update
		}
    }
	item->update();

    END_SECCERTAPI
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA) */
OSStatus SecCertificateSetPreferred(
	SecCertificateRef certificate,
	CFStringRef name,
	CFArrayRef keyUsage)
{
	CSSM_KEYUSE keyUse = ConvertArrayToKeyUsage(keyUsage);
	return SecCertificateSetPreference(certificate, name, keyUse, NULL);
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA) */
CFDictionaryRef SecCertificateCopyValues(SecCertificateRef certificate, CFArrayRef keys, CFErrorRef *error)
{
	CFDictionaryRef result = NULL;
	OSStatus __secapiresult;
	SecCertificateRef tmpcert = NULL;
#if SECTRUST_OSX
	// convert input to a new-style certificate reference if necessary,
	// since the implementation of CertificateValues calls SecCertificate API functions
	// which now assume a unified certificate reference.
	if (SecCertificateIsItemImplInstance(certificate)) {
		tmpcert = SecCertificateCreateFromItemImplInstance(certificate);
	}
#endif
	if (certificate && !tmpcert) {
		tmpcert = (SecCertificateRef) CFRetain(certificate);
	}
	try
	{
		CertificateValues cv(tmpcert);
		result = cv.copyFieldValues(keys,error);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	if (tmpcert) { CFRelease(tmpcert); }
	return result;
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA) */
CFStringRef SecCertificateCopyLongDescription(CFAllocatorRef alloc, SecCertificateRef certificate, CFErrorRef *error)
{
	return SecCertificateCopyShortDescription(alloc, certificate, error);
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA) */
CFStringRef SecCertificateCopyShortDescription(CFAllocatorRef alloc, SecCertificateRef certificate, CFErrorRef *error)
{
	CFStringRef result = NULL;
	OSStatus __secapiresult = SecCertificateInferLabel(certificate, &result);
	if (error!=NULL && __secapiresult!=errSecSuccess)
	{
		*error = CFErrorCreate(kCFAllocatorDefault, kCFErrorDomainOSStatus,
			__secapiresult ? __secapiresult : CSSM_ERRCODE_INTERNAL_ERROR, NULL);
	}
	return result;
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA) */
CFDataRef SecCertificateCopySerialNumber(SecCertificateRef certificate, CFErrorRef *error)
{
	CFDataRef result = NULL;
	OSStatus __secapiresult;
	try
	{
		CertificateValues cv(certificate);
		result = cv.copySerialNumber(error);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA) */
CFDataRef SecCertificateCopyNormalizedIssuerContent(SecCertificateRef certificate, CFErrorRef *error)
{
	CFDataRef result = NULL;
	OSStatus __secapiresult;
	try
	{
		CertificateValues cv(certificate);
		result = cv.copyNormalizedIssuerContent(error);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}

/* OS X only: __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA) */
CFDataRef SecCertificateCopyNormalizedSubjectContent(SecCertificateRef certificate, CFErrorRef *error)
{
	CFDataRef result = NULL;
	OSStatus __secapiresult;
	try
	{
		CertificateValues cv(certificate);
		result = cv.copyNormalizedSubjectContent(error);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}

#if !SECTRUST_OSX
CFDataRef SecCertificateCopyIssuerSequence(SecCertificateRef certificate)
{
	CFDataRef result = NULL;
	OSStatus __secapiresult;
	try
	{
		CertificateValues cv(certificate);
		result = cv.copyIssuerSequence(NULL);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}
#endif

#if !SECTRUST_OSX
CFDataRef SecCertificateCopySubjectSequence(SecCertificateRef certificate)
{
	CFDataRef result = NULL;
	OSStatus __secapiresult;
	try
	{
		CertificateValues cv(certificate);
		result = cv.copySubjectSequence(NULL);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}
#endif

#if !SECTRUST_OSX
CFDictionaryRef SecCertificateCopyAttributeDictionary(SecCertificateRef certificate)
{
    CFDictionaryRef result = NULL;
    OSStatus status;
    try
    {
        CertificateValues cv(certificate);
        result = cv.copyAttributeDictionary(NULL);
        status=0;
    }
    catch (const MacOSError &err) { status=err.osStatus(); }
    catch (const CommonError &err) { status=SecKeychainErrFromOSStatus(err.osStatus()); }
    catch (const std::bad_alloc &) { status=errSecAllocate; }
    catch (...) { status=errSecInternalComponent; }

    return result;
}
#endif

#if !SECTRUST_OSX
bool SecCertificateIsValid(SecCertificateRef certificate, CFAbsoluteTime verifyTime)
{
	bool result = NULL;
	OSStatus __secapiresult;
	try
	{
		CFErrorRef error = NULL;
		CertificateValues cv(certificate);
		result = cv.isValid(verifyTime, &error);
		if (error) CFRelease(error);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}
#endif

/* OS X only: __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_9, __IPHONE_NA, __IPHONE_NA) */
bool SecCertificateIsValidX(SecCertificateRef certificate, CFAbsoluteTime verifyTime)
{
    /*
     * deprecated function name
     */
	return SecCertificateIsValid(certificate, verifyTime);
}

#if !SECTRUST_OSX
CFAbsoluteTime SecCertificateNotValidBefore(SecCertificateRef certificate)
{
	CFAbsoluteTime result = 0;
	OSStatus __secapiresult;
	try
	{
		CFErrorRef error = NULL;
		CertificateValues cv(certificate);
		result = cv.notValidBefore(&error);
		if (error) CFRelease(error);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}
#endif

#if !SECTRUST_OSX
CFAbsoluteTime SecCertificateNotValidAfter(SecCertificateRef certificate)
{
	CFAbsoluteTime result = 0;
	OSStatus __secapiresult;
	try
	{
		CFErrorRef error = NULL;
		CertificateValues cv(certificate);
		result = cv.notValidAfter(&error);
		if (error) CFRelease(error);
		__secapiresult=0;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return result;
}
#endif

#if !SECTRUST_OSX
/* new in 10.8 */
SecCertificateRef SecCertificateCreateWithBytes(CFAllocatorRef allocator,
    const UInt8 *bytes, CFIndex length)
{
	SecCertificateRef certificate = NULL;
	OSStatus __secapiresult;
	try {
		CSSM_DATA cssmCertData = { (CSSM_SIZE)length, (uint8 *)bytes };

		//NOTE: there isn't yet a Certificate constructor which accepts a CFAllocatorRef
		SecPointer<Certificate> certificatePtr(new Certificate(cssmCertData, CSSM_CERT_X_509v3, CSSM_CERT_ENCODING_DER));
		certificate = certificatePtr->handle();

		__secapiresult=errSecSuccess;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return certificate;
}
#endif

#if !SECTRUST_OSX
/* new in 10.8 */
CFIndex SecCertificateGetLength(SecCertificateRef certificate)
{
	CFIndex length = 0;
	OSStatus __secapiresult;
	try {
		CssmData output = Certificate::required(certificate)->data();
		length = (CFIndex)output.length();
		__secapiresult=errSecSuccess;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return length;
}
#endif

#if !SECTRUST_OSX
/* new in 10.8 */
const UInt8 *SecCertificateGetBytePtr(SecCertificateRef certificate)
{
	const UInt8 *bytes = NULL;
	OSStatus __secapiresult;
	try {
		CssmData output = Certificate::required(certificate)->data();
		bytes = (const UInt8 *)output.data();
		__secapiresult=errSecSuccess;
	}
	catch (const MacOSError &err) { __secapiresult=err.osStatus(); }
	catch (const CommonError &err) { __secapiresult=SecKeychainErrFromOSStatus(err.osStatus()); }
	catch (const std::bad_alloc &) { __secapiresult=errSecAllocate; }
	catch (...) { __secapiresult=errSecInternalComponent; }
	return bytes;
}
#endif

#if !SECTRUST_OSX
/* not exported */
static CFArrayRef CopyEscrowCertificates(SecCertificateEscrowRootType escrowRootType, CFErrorRef *error)
{
	// Return array of CFDataRef certificates.
    CFArrayRef result = NULL;
	int iCnt;
	int numRoots = 0;

	// Get the hard coded set of production roots
	// static struct RootRecord* kProductionEscrowRoots[] = {&kOldEscrowRootRecord, &kProductionEscrowRootRecord};

	struct RootRecord** pEscrowRoots = NULL;
	switch (escrowRootType) {
		case kSecCertificateBaselineEscrowRoot:
			numRoots = kNumberOfBaseLineEscrowRoots;
			pEscrowRoots = kBaseLineEscrowRoots;
			break;
		case kSecCertificateProductionEscrowRoot:
			numRoots = kNumberOfBaseLineEscrowRoots; //%%% currently, production == baseline on OS X
			pEscrowRoots = kBaseLineEscrowRoots;
			break;
		case kSecCertificateBaselinePCSEscrowRoot:
			numRoots = kNumberOfBaseLinePCSEscrowRoots;
			pEscrowRoots = kBaseLinePCSEscrowRoots;
			break;
		case kSecCertificateProductionPCSEscrowRoot:
			numRoots = kNumberOfBaseLinePCSEscrowRoots; //%%% currently, production == baseline on OS X
			pEscrowRoots = kBaseLinePCSEscrowRoots;
			break;
		default:
			break;
	}

	CFDataRef productionCerts[numRoots];
	struct RootRecord* pRootRecord = NULL;

	for (iCnt = 0; pEscrowRoots != NULL && iCnt < numRoots; iCnt++)
	{
		pRootRecord = pEscrowRoots[iCnt];
		if (NULL != pRootRecord && pRootRecord->_length > 0 && NULL != pRootRecord->_bytes)
		{
			productionCerts[iCnt] = CFDataCreate(kCFAllocatorDefault, pRootRecord->_bytes, pRootRecord->_length);
		}
	}
	result = CFArrayCreate(kCFAllocatorDefault, (const void **)productionCerts, numRoots, &kCFTypeArrayCallBacks);
	for (iCnt = 0; iCnt < numRoots; iCnt++)
	{
		if (NULL != productionCerts[iCnt])
		{
			CFRelease(productionCerts[iCnt]);
		}
	}

	return result;
}
#endif

#if !SECTRUST_OSX
/* new in 10.9 */
CFArrayRef SecCertificateCopyEscrowRoots(SecCertificateEscrowRootType escrowRootType)
{
	CFArrayRef result = NULL;
	int iCnt;
	int numRoots = 0;
	CFDataRef certData = NULL;

	// The request is for the base line certificates.
	// Use the hard coded data to generate the return array
	if (kSecCertificateBaselineEscrowRoot == escrowRootType)
	{
		// Get the hard coded set of roots
		numRoots = kNumberOfBaseLineEscrowRoots;
	    SecCertificateRef baseLineCerts[numRoots];
	    struct RootRecord* pRootRecord = NULL;

	    for (iCnt = 0; iCnt < numRoots; iCnt++)
	    {
	        pRootRecord = kBaseLineEscrowRoots[iCnt];
	        if (NULL != pRootRecord && pRootRecord->_length > 0 && NULL != pRootRecord->_bytes)
	        {
				certData = CFDataCreate(kCFAllocatorDefault, pRootRecord->_bytes, pRootRecord->_length);
				if (NULL != certData)
				{
					baseLineCerts[iCnt] = SecCertificateCreateWithData(kCFAllocatorDefault, certData);
					CFRelease(certData);
				}
	        }
	    }
		result = CFArrayCreate(kCFAllocatorDefault, (const void **)baseLineCerts, numRoots, &kCFTypeArrayCallBacks);
		for (iCnt = 0; iCnt < numRoots; iCnt++)
		{
			if (NULL != baseLineCerts[iCnt])
			{
				CFRelease(baseLineCerts[iCnt]);
			}
		}
	}
	// The request is for the current certificates.
	else
	{
		CFErrorRef error = NULL;
		CFArrayRef cert_datas = CopyEscrowCertificates(escrowRootType, &error);
		if (NULL != error || NULL == cert_datas || 0 == (numRoots = (int)CFArrayGetCount(cert_datas)))
		{
			if (NULL != error)
			{
				CFRelease(error);
			}

			if (NULL != cert_datas)
			{
				CFRelease(cert_datas);
			}
			return result;
		}

		SecCertificateRef assetCerts[numRoots];
		for (iCnt = 0; iCnt < numRoots; iCnt++)
		{
			certData = (CFDataRef)CFArrayGetValueAtIndex(cert_datas, iCnt);
			if (NULL != certData)
			{
				SecCertificateRef aCertRef = SecCertificateCreateWithData(kCFAllocatorDefault, certData);
				assetCerts[iCnt] = aCertRef;
			}
			else
			{
				assetCerts[iCnt] = NULL;
			}
		}

		if (numRoots > 0)
		{
			result = CFArrayCreate(kCFAllocatorDefault, (const void **)assetCerts, numRoots, &kCFTypeArrayCallBacks);
			for (iCnt = 0; iCnt < numRoots; iCnt++)
			{
				if (NULL != assetCerts[iCnt])
				{
					CFRelease(assetCerts[iCnt]);
				}
			}
		}
		CFRelease(cert_datas);
	}

    return result;
}
#endif

#if !SECTRUST_OSX
/* new in 10.11 */
SecSignatureHashAlgorithm SecCertificateGetSignatureHashAlgorithm(SecCertificateRef certificate)
{
	SecSignatureHashAlgorithm result = kSecSignatureHashAlgorithmUnknown;
	CSSM_X509_ALGORITHM_IDENTIFIER_PTR algId = NULL;
	CSSM_DATA_PTR fieldValue = NULL;
	CSSM_OID_PTR algOID = NULL;
    const CSSM_OID *sigAlgOID = &CSSMOID_X509V1SignatureAlgorithm;
	OSStatus status;

	status = SecCertificateCopyFirstFieldValue(certificate, sigAlgOID, &fieldValue);
	if (status || !fieldValue)  {
		return result;
	}
	algId = (CSSM_X509_ALGORITHM_IDENTIFIER_PTR)fieldValue->Data;
	algOID = (algId) ? &algId->algorithm : NULL;

	while (algOID) {
		if (!algOID->Data || !algOID->Length) {
			break;
		}
		/* classify the signature algorithm OID into one of our known types */
		if (cuCompareCssmData(algOID, &CSSMOID_ECDSA_WithSHA512) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA512WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA512)) {
			result = kSecSignatureHashAlgorithmSHA512;
			break;
		}
		if (cuCompareCssmData(algOID, &CSSMOID_ECDSA_WithSHA384) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA384WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA384)) {
			result = kSecSignatureHashAlgorithmSHA384;
			break;
		}
		if (cuCompareCssmData(algOID, &CSSMOID_ECDSA_WithSHA256) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA256WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA256)) {
			result = kSecSignatureHashAlgorithmSHA256;
			break;
		}
		if (cuCompareCssmData(algOID, &CSSMOID_ECDSA_WithSHA224) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA224WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA224)) {
			result = kSecSignatureHashAlgorithmSHA224;
			break;
		}
		if (cuCompareCssmData(algOID, &CSSMOID_ECDSA_WithSHA1) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA1WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA1WithDSA) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA1WithDSA_CMS) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA1WithDSA_JDK) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA1WithRSA_OIW) ||
			cuCompareCssmData(algOID, &CSSMOID_APPLE_FEE_SHA1) ||
			cuCompareCssmData(algOID, &CSSMOID_SHA1)) {
			result = kSecSignatureHashAlgorithmSHA1;
			break;
		}
		if (cuCompareCssmData(algOID, &CSSMOID_MD5WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_APPLE_FEE_MD5) ||
			cuCompareCssmData(algOID, &CSSMOID_MD5)) {
			result = kSecSignatureHashAlgorithmMD5;
			break;
		}
		if (cuCompareCssmData(algOID, &CSSMOID_MD4WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_MD4)) {
			result = kSecSignatureHashAlgorithmMD4;
			break;
		}
		if (cuCompareCssmData(algOID, &CSSMOID_MD2WithRSA) ||
			cuCompareCssmData(algOID, &CSSMOID_MD2)) {
			result = kSecSignatureHashAlgorithmMD2;
			break;
		}
		break;
	}

	(void)SecCertificateReleaseFirstFieldValue(certificate, sigAlgOID, fieldValue);

	return result;
}
#endif

