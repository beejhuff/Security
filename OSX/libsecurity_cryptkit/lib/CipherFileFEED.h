/* Copyright (c) 1998,2011,2014 Apple Inc.  All Rights Reserved.
 *
 * NOTICE: USE OF THE MATERIALS ACCOMPANYING THIS NOTICE IS SUBJECT
 * TO THE TERMS OF THE SIGNED "FAST ELLIPTIC ENCRYPTION (FEE) REFERENCE
 * SOURCE CODE EVALUATION AGREEMENT" BETWEEN APPLE, INC. AND THE
 * ORIGINAL LICENSEE THAT OBTAINED THESE MATERIALS FROM APPLE,
 * INC.  ANY USE OF THESE MATERIALS NOT PERMITTED BY SUCH AGREEMENT WILL
 * EXPOSE YOU TO LIABILITY.
 ***************************************************************************
 *
 * CipherFileFEED.h - FEED and FEEDExp related cipherfile support
 *
 * Revision History
 * ----------------
 * 18 Feb 97 at Apple
 *	Created.
 */

#ifndef	_CK_CFILEFEED_H_
#define _CK_CFILEFEED_H_

#include "ckconfig.h"

#if CRYPTKIT_CIPHERFILE_ENABLE

#include "Crypt.h"
#include "feeCipherFile.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private functions.
 */
feeReturn createFEED(feePubKey sendPrivKey,
	feePubKey recvPubKey,
	const unsigned char *plainText,
	unsigned plainTextLen,
	int genSig,				// 1 ==> generate signature
	unsigned userData,			// for caller's convenience
	feeCipherFile *cipherFile);		// RETURNED if successful
feeReturn decryptFEED(feeCipherFile cipherFile,
	feePubKey recvPrivKey,
	feePubKey sendPubKey,
	unsigned char **plainText,		// RETURNED
	unsigned *plainTextLen,			// RETURNED
	feeSigStatus *sigStatus);		// RETURNED
feeReturn createFEEDExp(feePubKey sendPrivKey,
	feePubKey recvPubKey,
	const unsigned char *plainText,
	unsigned plainTextLen,
	int genSig,				// 1 ==> generate signature
	unsigned userData,			// for caller's convenience
	feeCipherFile *cipherFile);		// RETURNED if successful
feeReturn decryptFEEDExp(feeCipherFile cipherFile,
	feePubKey recvPrivKey,
	feePubKey sendPubKey,			// optional
	unsigned char **plainText,		// RETURNED
	unsigned *plainTextLen,			// RETURNED
	feeSigStatus *sigStatus);		// RETURNED

#ifdef __cplusplus
}
#endif

#endif	/* CRYPTKIT_CIPHERFILE_ENABLE */

#endif	/*_CK_CFILEFEED_H_*/
