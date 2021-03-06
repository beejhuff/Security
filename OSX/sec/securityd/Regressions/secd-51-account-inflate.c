/*
 * Copyright (c) 2013-2014 Apple Inc. All Rights Reserved.
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


#include <Security/SecBase.h>
#include <Security/SecItem.h>

#include <Security/SecureObjectSync/SOSAccount.h>
#include <Security/SecureObjectSync/SOSCloudCircle.h>
#include <Security/SecureObjectSync/SOSInternal.h>
#include <Security/SecureObjectSync/SOSUserKeygen.h>

#include <stdlib.h>
#include <unistd.h>

#include "secd_regressions.h"
#include "SOSTestDataSource.h"

#include "SOSRegressionUtilities.h"
#include <utilities/SecCFWrappers.h>

#include <securityd/SOSCloudCircleServer.h>

#include "SecdTestKeychainUtilities.h"
#include "SOSAccountTesting.h"
#include "SOSTransportTestTransports.h"

#if 0
const uint8_t v6_der[] = {
    0x30, 0x82, 0x06, 0xee, 0x02, 0x01, 0x06, 0x31, 0x4e, 0x30, 0x11, 0x0c,
    0x09, 0x4d, 0x6f, 0x64, 0x65, 0x6c, 0x4e, 0x61, 0x6d, 0x65, 0x0c, 0x04,
    0x69, 0x50, 0x61, 0x64, 0x30, 0x1b, 0x0c, 0x16, 0x4d, 0x65, 0x73, 0x73,
    0x61, 0x67, 0x65, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x63, 0x6f, 0x6c, 0x56,
    0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x02, 0x01, 0x00, 0x30, 0x1c, 0x0c,
    0x0c, 0x43, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x72, 0x4e, 0x61, 0x6d,
    0x65, 0x0c, 0x0c, 0x4d, 0x69, 0x74, 0x63, 0x68, 0x27, 0x73, 0x20, 0x69,
    0x50, 0x61, 0x64, 0x30, 0x82, 0x05, 0xe0, 0x30, 0x82, 0x05, 0xdc, 0x04,
    0x82, 0x04, 0x1c, 0x30, 0x82, 0x04, 0x18, 0x02, 0x01, 0x01, 0x0c, 0x02,
    0x61, 0x6b, 0x02, 0x08, 0x0d, 0x4f, 0xc4, 0x65, 0x00, 0x00, 0x00, 0x03,
    0x30, 0x82, 0x03, 0x2c, 0x30, 0x82, 0x01, 0xa2, 0x31, 0x82, 0x01, 0x56,
    0x30, 0x14, 0x0c, 0x0f, 0x43, 0x6f, 0x6e, 0x66, 0x6c, 0x69, 0x63, 0x74,
    0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x02, 0x01, 0x00, 0x30, 0x2b,
    0x0c, 0x0f, 0x41, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f,
    0x6e, 0x44, 0x61, 0x74, 0x65, 0x04, 0x18, 0x18, 0x16, 0x32, 0x30, 0x31,
    0x35, 0x30, 0x32, 0x32, 0x36, 0x31, 0x37, 0x30, 0x30, 0x35, 0x39, 0x2e,
    0x39, 0x31, 0x35, 0x33, 0x35, 0x38, 0x5a, 0x30, 0x55, 0x0c, 0x10, 0x50,
    0x75, 0x62, 0x6c, 0x69, 0x63, 0x53, 0x69, 0x67, 0x6e, 0x69, 0x6e, 0x67,
    0x4b, 0x65, 0x79, 0x04, 0x41, 0x04, 0xd9, 0x02, 0x0e, 0x16, 0x03, 0x13,
    0xfb, 0xf8, 0x29, 0xbf, 0x83, 0xbd, 0x6b, 0x5a, 0xc4, 0xf0, 0x47, 0x52,
    0xc7, 0x87, 0x81, 0xe9, 0xda, 0xe0, 0xb4, 0x8b, 0x06, 0x73, 0x23, 0x72,
    0xba, 0xba, 0xc4, 0x1a, 0x35, 0xa2, 0xc6, 0x46, 0x61, 0xc9, 0x08, 0x43,
    0x4f, 0x89, 0x08, 0x9d, 0xe1, 0xd5, 0x3d, 0x83, 0x49, 0xe3, 0x0c, 0x8e,
    0xfb, 0x33, 0x37, 0xd7, 0x6d, 0x03, 0xe0, 0x39, 0x11, 0xe1, 0x30, 0x59,
    0x0c, 0x0f, 0x41, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f,
    0x6e, 0x55, 0x73, 0x69, 0x67, 0x04, 0x46, 0x30, 0x44, 0x02, 0x20, 0x66,
    0x9e, 0xd0, 0x80, 0x11, 0xa4, 0x17, 0x94, 0x66, 0x5f, 0x75, 0xee, 0x65,
    0xbf, 0xd0, 0x49, 0x8f, 0xe6, 0x22, 0xf6, 0x3c, 0x2c, 0xdb, 0x46, 0xe5,
    0x3f, 0x4f, 0x85, 0xb9, 0x8a, 0x6c, 0x8a, 0x02, 0x20, 0x0a, 0x37, 0xf0,
    0xf1, 0x9f, 0x13, 0xfd, 0xae, 0x5f, 0xb7, 0xd0, 0xb4, 0x4a, 0x45, 0x02,
    0x8a, 0x2f, 0xdc, 0x8b, 0xd3, 0xfb, 0x09, 0xeb, 0xcf, 0x6b, 0x0b, 0x6b,
    0x18, 0x9c, 0xcf, 0x6c, 0x55, 0x30, 0x5f, 0x0c, 0x0d, 0x44, 0x65, 0x76,
    0x69, 0x63, 0x65, 0x47, 0x65, 0x73, 0x74, 0x61, 0x6c, 0x74, 0x31, 0x4e,
    0x30, 0x11, 0x0c, 0x09, 0x4d, 0x6f, 0x64, 0x65, 0x6c, 0x4e, 0x61, 0x6d,
    0x65, 0x0c, 0x04, 0x69, 0x50, 0x61, 0x64, 0x30, 0x1b, 0x0c, 0x16, 0x4d,
    0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x63,
    0x6f, 0x6c, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x02, 0x01, 0x00,
    0x30, 0x1c, 0x0c, 0x0c, 0x43, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x72,
    0x4e, 0x61, 0x6d, 0x65, 0x0c, 0x0c, 0x4d, 0x69, 0x74, 0x63, 0x68, 0x27,
    0x73, 0x20, 0x69, 0x50, 0x61, 0x64, 0x04, 0x46, 0x30, 0x44, 0x02, 0x20,
    0x72, 0x06, 0xdb, 0xf8, 0x06, 0xff, 0x7f, 0xac, 0xcf, 0xd2, 0xfb, 0x1e,
    0x86, 0x0d, 0x87, 0x4e, 0xb9, 0x1b, 0x26, 0x1e, 0x47, 0x4b, 0xfe, 0xd0,
    0x54, 0x0c, 0xdf, 0x88, 0x5a, 0x27, 0x92, 0x1d, 0x02, 0x20, 0x6b, 0xcf,
    0x7f, 0xb5, 0xfe, 0x19, 0x16, 0xd2, 0x7f, 0xb1, 0x7b, 0xad, 0xf5, 0xb9,
    0xea, 0x23, 0x69, 0x37, 0x19, 0x5e, 0xd3, 0x8c, 0x9e, 0x80, 0xef, 0xb5,
    0x65, 0x9a, 0xd5, 0x42, 0x51, 0x13, 0x30, 0x82, 0x01, 0x82, 0x31, 0x82,
    0x01, 0x35, 0x30, 0x12, 0x0c, 0x0d, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x49,
    0x64, 0x65, 0x6e, 0x74, 0x69, 0x74, 0x79, 0x01, 0x01, 0x01, 0x30, 0x14,
    0x0c, 0x0f, 0x43, 0x6f, 0x6e, 0x66, 0x6c, 0x69, 0x63, 0x74, 0x56, 0x65,
    0x72, 0x73, 0x69, 0x6f, 0x6e, 0x02, 0x01, 0x00, 0x30, 0x29, 0x0c, 0x0d,
    0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x47, 0x65, 0x73, 0x74, 0x61, 0x6c,
    0x74, 0x31, 0x18, 0x30, 0x16, 0x0c, 0x0c, 0x43, 0x6f, 0x6d, 0x70, 0x75,
    0x74, 0x65, 0x72, 0x4e, 0x61, 0x6d, 0x65, 0x0c, 0x06, 0x69, 0x43, 0x6c,
    0x6f, 0x75, 0x64, 0x30, 0x2b, 0x0c, 0x0f, 0x41, 0x70, 0x70, 0x6c, 0x69,
    0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x44, 0x61, 0x74, 0x65, 0x04, 0x18,
    0x18, 0x16, 0x32, 0x30, 0x31, 0x35, 0x30, 0x32, 0x32, 0x36, 0x31, 0x37,
    0x30, 0x30, 0x35, 0x39, 0x2e, 0x39, 0x36, 0x30, 0x38, 0x33, 0x35, 0x5a,
    0x30, 0x55, 0x0c, 0x10, 0x50, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x53, 0x69,
    0x67, 0x6e, 0x69, 0x6e, 0x67, 0x4b, 0x65, 0x79, 0x04, 0x41, 0x04, 0x53,
    0x1d, 0x77, 0x7c, 0x1b, 0x8a, 0x53, 0xad, 0x88, 0xdf, 0x56, 0xf9, 0x11,
    0xfd, 0x40, 0x69, 0x7c, 0x0b, 0xbb, 0x2a, 0xf7, 0x48, 0x7e, 0x69, 0x3d,
    0x0c, 0xfb, 0xc8, 0x90, 0x27, 0xb5, 0x18, 0x88, 0x09, 0x3f, 0x06, 0x37,
    0xca, 0x5d, 0x9c, 0x64, 0x34, 0x13, 0x89, 0x09, 0xe9, 0x0e, 0x99, 0xa3,
    0xc8, 0xc1, 0x86, 0xb3, 0x6e, 0x30, 0xf1, 0x20, 0x38, 0x78, 0x56, 0x40,
    0xf4, 0xd7, 0x36, 0x30, 0x5a, 0x0c, 0x0f, 0x41, 0x70, 0x70, 0x6c, 0x69,
    0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x55, 0x73, 0x69, 0x67, 0x04, 0x47,
    0x30, 0x45, 0x02, 0x20, 0x45, 0x47, 0xbf, 0x5b, 0x4d, 0x57, 0x77, 0x90,
    0x7b, 0x74, 0x90, 0x2f, 0x5d, 0x7b, 0x02, 0x3c, 0x21, 0xe8, 0x5c, 0x24,
    0x0c, 0x77, 0xd8, 0x19, 0xff, 0xce, 0x28, 0x45, 0x76, 0x50, 0xb1, 0xee,
    0x02, 0x21, 0x00, 0xe3, 0xea, 0x0f, 0xcb, 0x72, 0xd7, 0x88, 0x15, 0xa4,
    0x71, 0xbe, 0x9e, 0xa6, 0xf2, 0x9c, 0xa8, 0x9b, 0x93, 0xec, 0x31, 0x7a,
    0xe3, 0x92, 0x8a, 0xf2, 0x2f, 0xfd, 0x98, 0xc9, 0xe1, 0xc4, 0x7c, 0x04,
    0x47, 0x30, 0x45, 0x02, 0x21, 0x00, 0xce, 0x5f, 0x67, 0x9d, 0x78, 0x9f,
    0xc6, 0x3b, 0xb3, 0x51, 0xde, 0x6d, 0x9e, 0xff, 0xf5, 0xab, 0xda, 0xf4,
    0x7e, 0xb2, 0xe9, 0x1a, 0xf2, 0xd9, 0x0d, 0x48, 0x30, 0x07, 0xbc, 0x06,
    0xf5, 0x42, 0x02, 0x20, 0x64, 0x1d, 0x7d, 0x43, 0xc1, 0x3b, 0x50, 0xd6,
    0x7b, 0x3b, 0xce, 0x6f, 0xec, 0x23, 0x21, 0x8a, 0x9b, 0x06, 0x3f, 0x64,
    0xfe, 0xd6, 0x29, 0xaf, 0x5b, 0x0c, 0x69, 0x8f, 0x48, 0x88, 0x08, 0x1e,
    0x30, 0x00, 0x30, 0x00, 0x31, 0x81, 0xd0, 0x30, 0x66, 0x0c, 0x1a, 0x30,
    0x56, 0x4f, 0x73, 0x47, 0x76, 0x56, 0x72, 0x51, 0x46, 0x4e, 0x46, 0x6b,
    0x52, 0x35, 0x37, 0x71, 0x59, 0x73, 0x4c, 0x47, 0x6f, 0x33, 0x49, 0x50,
    0x51, 0x04, 0x48, 0x30, 0x46, 0x02, 0x21, 0x00, 0xf9, 0xc9, 0x69, 0xda,
    0x69, 0xe7, 0x32, 0xeb, 0x14, 0xc1, 0xbd, 0x56, 0xb7, 0x3e, 0xab, 0x08,
    0x6d, 0xe2, 0x16, 0xd9, 0x0a, 0xa8, 0x67, 0x7a, 0x56, 0xdc, 0x64, 0xf2,
    0x2f, 0x2e, 0xe8, 0xb9, 0x02, 0x21, 0x00, 0xac, 0x5c, 0x29, 0x80, 0x60,
    0x57, 0xd7, 0x51, 0x23, 0x1f, 0x00, 0x09, 0x70, 0xf3, 0xd5, 0x1a, 0x49,
    0xad, 0xaf, 0x35, 0x77, 0x07, 0xf4, 0x85, 0x1f, 0x63, 0xe0, 0xea, 0x67,
    0x36, 0xb5, 0x3b, 0x30, 0x66, 0x0c, 0x1a, 0x6e, 0x6b, 0x62, 0x77, 0x33,
    0x2f, 0x57, 0x72, 0x39, 0x4f, 0x33, 0x6a, 0x45, 0x68, 0x4f, 0x48, 0x46,
    0x67, 0x32, 0x34, 0x67, 0x62, 0x4d, 0x61, 0x6a, 0x76, 0x04, 0x48, 0x30,
    0x46, 0x02, 0x21, 0x00, 0x9f, 0x4f, 0x60, 0x2f, 0x54, 0x5e, 0x9f, 0x6a,
    0x7d, 0x37, 0xd9, 0x9f, 0x62, 0x69, 0xd7, 0x8c, 0xfc, 0xf5, 0x78, 0x54,
    0x7d, 0xb6, 0x91, 0x4b, 0x48, 0x88, 0x3d, 0x59, 0x05, 0x70, 0x09, 0xbf,
    0x02, 0x21, 0x00, 0xe2, 0xaf, 0xc9, 0x27, 0x12, 0xfa, 0x64, 0x51, 0x19,
    0x13, 0x1d, 0x57, 0x56, 0x6b, 0x93, 0xa3, 0xc6, 0x31, 0xf8, 0x70, 0x97,
    0x9e, 0x79, 0xf5, 0xc0, 0x2d, 0xf3, 0x3c, 0x8b, 0x86, 0x92, 0x28, 0x04,
    0x82, 0x01, 0xb8, 0x30, 0x82, 0x01, 0xb4, 0x30, 0x82, 0x01, 0xa2, 0x31,
    0x82, 0x01, 0x56, 0x30, 0x14, 0x0c, 0x0f, 0x43, 0x6f, 0x6e, 0x66, 0x6c,
    0x69, 0x63, 0x74, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x02, 0x01,
    0x00, 0x30, 0x2b, 0x0c, 0x0f, 0x41, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61,
    0x74, 0x69, 0x6f, 0x6e, 0x44, 0x61, 0x74, 0x65, 0x04, 0x18, 0x18, 0x16,
    0x32, 0x30, 0x31, 0x35, 0x30, 0x32, 0x32, 0x36, 0x31, 0x37, 0x30, 0x30,
    0x35, 0x39, 0x2e, 0x39, 0x31, 0x35, 0x33, 0x35, 0x38, 0x5a, 0x30, 0x55,
    0x0c, 0x10, 0x50, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x53, 0x69, 0x67, 0x6e,
    0x69, 0x6e, 0x67, 0x4b, 0x65, 0x79, 0x04, 0x41, 0x04, 0xd9, 0x02, 0x0e,
    0x16, 0x03, 0x13, 0xfb, 0xf8, 0x29, 0xbf, 0x83, 0xbd, 0x6b, 0x5a, 0xc4,
    0xf0, 0x47, 0x52, 0xc7, 0x87, 0x81, 0xe9, 0xda, 0xe0, 0xb4, 0x8b, 0x06,
    0x73, 0x23, 0x72, 0xba, 0xba, 0xc4, 0x1a, 0x35, 0xa2, 0xc6, 0x46, 0x61,
    0xc9, 0x08, 0x43, 0x4f, 0x89, 0x08, 0x9d, 0xe1, 0xd5, 0x3d, 0x83, 0x49,
    0xe3, 0x0c, 0x8e, 0xfb, 0x33, 0x37, 0xd7, 0x6d, 0x03, 0xe0, 0x39, 0x11,
    0xe1, 0x30, 0x59, 0x0c, 0x0f, 0x41, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61,
    0x74, 0x69, 0x6f, 0x6e, 0x55, 0x73, 0x69, 0x67, 0x04, 0x46, 0x30, 0x44,
    0x02, 0x20, 0x66, 0x9e, 0xd0, 0x80, 0x11, 0xa4, 0x17, 0x94, 0x66, 0x5f,
    0x75, 0xee, 0x65, 0xbf, 0xd0, 0x49, 0x8f, 0xe6, 0x22, 0xf6, 0x3c, 0x2c,
    0xdb, 0x46, 0xe5, 0x3f, 0x4f, 0x85, 0xb9, 0x8a, 0x6c, 0x8a, 0x02, 0x20,
    0x0a, 0x37, 0xf0, 0xf1, 0x9f, 0x13, 0xfd, 0xae, 0x5f, 0xb7, 0xd0, 0xb4,
    0x4a, 0x45, 0x02, 0x8a, 0x2f, 0xdc, 0x8b, 0xd3, 0xfb, 0x09, 0xeb, 0xcf,
    0x6b, 0x0b, 0x6b, 0x18, 0x9c, 0xcf, 0x6c, 0x55, 0x30, 0x5f, 0x0c, 0x0d,
    0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x47, 0x65, 0x73, 0x74, 0x61, 0x6c,
    0x74, 0x31, 0x4e, 0x30, 0x11, 0x0c, 0x09, 0x4d, 0x6f, 0x64, 0x65, 0x6c,
    0x4e, 0x61, 0x6d, 0x65, 0x0c, 0x04, 0x69, 0x50, 0x61, 0x64, 0x30, 0x1b,
    0x0c, 0x16, 0x4d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x50, 0x72, 0x6f,
    0x74, 0x6f, 0x63, 0x6f, 0x6c, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e,
    0x02, 0x01, 0x00, 0x30, 0x1c, 0x0c, 0x0c, 0x43, 0x6f, 0x6d, 0x70, 0x75,
    0x74, 0x65, 0x72, 0x4e, 0x61, 0x6d, 0x65, 0x0c, 0x0c, 0x4d, 0x69, 0x74,
    0x63, 0x68, 0x27, 0x73, 0x20, 0x69, 0x50, 0x61, 0x64, 0x04, 0x46, 0x30,
    0x44, 0x02, 0x20, 0x72, 0x06, 0xdb, 0xf8, 0x06, 0xff, 0x7f, 0xac, 0xcf,
    0xd2, 0xfb, 0x1e, 0x86, 0x0d, 0x87, 0x4e, 0xb9, 0x1b, 0x26, 0x1e, 0x47,
    0x4b, 0xfe, 0xd0, 0x54, 0x0c, 0xdf, 0x88, 0x5a, 0x27, 0x92, 0x1d, 0x02,
    0x20, 0x6b, 0xcf, 0x7f, 0xb5, 0xfe, 0x19, 0x16, 0xd2, 0x7f, 0xb1, 0x7b,
    0xad, 0xf5, 0xb9, 0xea, 0x23, 0x69, 0x37, 0x19, 0x5e, 0xd3, 0x8c, 0x9e,
    0x80, 0xef, 0xb5, 0x65, 0x9a, 0xd5, 0x42, 0x51, 0x13, 0x04, 0x0c, 0x6b,
    0x65, 0x79, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x41, 0x04, 0x78, 0xea, 0x03, 0x41,
    0x94, 0x6d, 0x46, 0x38, 0xde, 0x37, 0xe0, 0xb6, 0xc8, 0xfa, 0x94, 0x86,
    0xbe, 0xd5, 0x78, 0x08, 0x2f, 0x1f, 0x5e, 0xa4, 0x0c, 0x93, 0xc2, 0x5f,
    0x1e, 0xca, 0x05, 0x21, 0x51, 0xc2, 0xce, 0x7c, 0xcf, 0x58, 0x2f, 0x46,
    0x53, 0x7e, 0x49, 0x97, 0xb8, 0x06, 0x98, 0x9b, 0xe2, 0x12, 0xa9, 0x92,
    0xdd, 0x30, 0xe2, 0x4d, 0xd4, 0x32, 0x09, 0x62, 0x5f, 0xea, 0x0c, 0x0c,
    0x04, 0x41, 0x04, 0x78, 0xea, 0x03, 0x41, 0x94, 0x6d, 0x46, 0x38, 0xde,
    0x37, 0xe0, 0xb6, 0xc8, 0xfa, 0x94, 0x86, 0xbe, 0xd5, 0x78, 0x08, 0x2f,
    0x1f, 0x5e, 0xa4, 0x0c, 0x93, 0xc2, 0x5f, 0x1e, 0xca, 0x05, 0x21, 0x51,
    0xc2, 0xce, 0x7c, 0xcf, 0x58, 0x2f, 0x46, 0x53, 0x7e, 0x49, 0x97, 0xb8,
    0x06, 0x98, 0x9b, 0xe2, 0x12, 0xa9, 0x92, 0xdd, 0x30, 0xe2, 0x4d, 0xd4,
    0x32, 0x09, 0x62, 0x5f, 0xea, 0x0c, 0x0c, 0x04, 0x27, 0x30, 0x25, 0x04,
    0x10, 0x48, 0xd4, 0x7b, 0x6a, 0x50, 0xfb, 0x84, 0xf8, 0xb5, 0x8d, 0x13,
    0x62, 0xcc, 0x42, 0xa5, 0x42, 0x02, 0x03, 0x00, 0xc3, 0x50, 0x02, 0x02,
    0x01, 0x00, 0x06, 0x08, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x07,
    0x31, 0x00 };
#endif

#define kCompatibilityTestCount 0
#if 0
static void test_v6(void) {
    SOSDataSourceFactoryRef ak_factory = SOSTestDataSourceFactoryCreate();
    SOSDataSourceRef test_source = SOSTestDataSourceCreate();
    SOSTestDataSourceFactorySetDataSource(ak_factory, CFSTR("ak"), test_source);
    CFErrorRef error = NULL;

    const uint8_t *der_p = v6_der;
    
    SOSAccountRef convertedAccount = SOSAccountCreateFromDER(kCFAllocatorDefault, ak_factory, &error, &der_p, v6_der + sizeof(v6_der));
    
    ok(convertedAccount, "inflate v6 account (%@)", error);
    CFReleaseSafe(error);
    
    is(kSOSCCInCircle, SOSAccountGetCircleStatus(convertedAccount, &error), "in the circle");
    
    CFReleaseSafe(convertedAccount);
    ak_factory->release(ak_factory);
    SOSDataSourceRelease(test_source, NULL);
}
#endif

static int kTestTestCount = 11 + kSecdTestSetupTestCount;
static void tests(void)
{

    CFErrorRef error = NULL;
    CFDataRef cfpassword = CFDataCreate(NULL, (uint8_t *) "FooFooFoo", 10);
    CFStringRef cfaccount = CFSTR("test@test.org");
    
    SOSDataSourceFactoryRef test_factory = SOSTestDataSourceFactoryCreate();
    SOSDataSourceRef test_source = SOSTestDataSourceCreate();
    SOSTestDataSourceFactorySetDataSource(test_factory, CFSTR("TestType"), test_source);
    
    SOSAccountRef account = CreateAccountForLocalChanges(CFSTR("Test Device"), CFSTR("TestType"));
    ok(SOSAccountAssertUserCredentialsAndUpdate(account, cfaccount, cfpassword, &error), "Credential setting (%@)", error);
    CFReleaseNull(error);
    CFReleaseNull(cfpassword);
    
    ok(NULL != account, "Created");
    
    ok(SOSAccountResetToOffering_wTxn(account, &error), "Reset to offering (%@)", error);
    CFReleaseNull(error);

    ok(testAccountPersistence(account), "Test Account->DER->Account Equivalence");
    CFReleaseNull(account);

    SOSUnregisterAllTransportMessages();
    SOSUnregisterAllTransportCircles();
    SOSUnregisterAllTransportKeyParameters();
    CFArrayRemoveAllValues(key_transports);
    CFArrayRemoveAllValues(circle_transports);
    CFArrayRemoveAllValues(message_transports);
    
    test_factory->release(test_factory);
    SOSDataSourceRelease(test_source, NULL);
}

int secd_51_account_inflate(int argc, char *const *argv)
{
    plan_tests(kTestTestCount + kCompatibilityTestCount);

    secd_test_setup_temp_keychain(__FUNCTION__, NULL);
    tests();

    /* we can't re-inflate the v6 DER since we don't have a viable private key for it. */
    // test_v6();

    return 0;
}
