/* Microsoft Reference Implementation for TPM 2.0
 *
 *  The copyright in this software is being made available under the BSD License,
 *  included below. This software may be subject to other third party and
 *  contributor rights, including patent rights, and no such rights are granted
 *  under this license.
 *
 *  Copyright (c) Microsoft Corporation
 *
 *  All rights reserved.
 *
 *  BSD License
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this list
 *  of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice, this
 *  list of conditions and the following disclaimer in the documentation and/or
 *  other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ""AS IS""
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*(Auto-generated)
 *  Created by TpmStructures; Version 4.1 Dec 8, 2018
 *  Date: Jan 28, 2019  Time: 12:39:24AM
 */

#ifdef TPM_CC_GetCommandAuditDigest // Command must be defined
#ifndef _GETCOMMANDAUDITDIGEST_FP_H_
#define _GETCOMMANDAUDITDIGEST_FP_H_

// Input structure definition
typedef struct {
    TPMI_RH_ENDORSEMENT                 privacyHandle;
    TPMI_DH_OBJECT                      signHandle;
    TPM2B_DATA                          qualifyingData;
    TPMT_SIG_SCHEME                     inScheme;
} GetCommandAuditDigest_In;

// Output structure definition
typedef struct {
    TPM2B_ATTEST                        auditInfo;
    TPMT_SIGNATURE                      signature;
} GetCommandAuditDigest_Out;

// Response code modifiers
#define RC_GetCommandAuditDigest_privacyHandle  (TPM_RC_H + TPM_RC_1)
#define RC_GetCommandAuditDigest_signHandle (TPM_RC_H + TPM_RC_2)
#define RC_GetCommandAuditDigest_qualifyingData (TPM_RC_P + TPM_RC_1)
#define RC_GetCommandAuditDigest_inScheme   (TPM_RC_P + TPM_RC_2)

// Function prototype
TPM_RC
TPM2_GetCommandAuditDigest(
    GetCommandAuditDigest_In            *in,
    GetCommandAuditDigest_Out           *out
);

#endif  // _GETCOMMANDAUDITDIGEST_FP_H_
#endif  // TPM_CC_GetCommandAuditDigest
