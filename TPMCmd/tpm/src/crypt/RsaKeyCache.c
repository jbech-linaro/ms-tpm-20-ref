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
//** Introduction
// This file contains the functions to implement the RSA key cache that can be used
// to speed up simulation.
//
// Only one key is created for each supported key size and it is returned whenever
// a key of that size is requested.
//
// If desired, the key cache can be populated from a file. This allows multiple
// TPM to run with the same RSA keys. Also, when doing simulation, the DRBG will
// use preset sequences so it is not too hard to repeat sequences for debug or
// profile or stress.
//
// When the key cache is enabled, a call to CryptRsaGenerateKey() will call the
// GetCachedRsaKey(). If the cache is enabled and populated, then the cached key
// of the requested size is returned. If a key of the requested size is not
// available, the no key is loaded and the requested key will need to be generated.
// If the cache is not populated, the TPM will open a file that has the appropriate
// name for the type of keys required (CRT or no-CRT). If the file is the right
// size, it is used. If the file doesn't exist or the file does not have the correct
// size, the TMP will populate the cache with new keys of the required size and
// write the cache data to the file so that they will be available the next time.
//
// Currently, if two simulations are being run with TPM's that have different RSA
// key sizes (e.g,, one with 1024 and 2048 and another with 2048 and 3072, then the
// files will not match for the both of them and they will both try to overwrite
// the other's cache file. I may try to do something about this if necessary.

//** Includes, Types, Locals, and Defines

#include "Tpm.h"

#if SIMULATION && USE_RSA_KEY_CACHE
#include  <stdio.h>

#include "Platform_fp.h"
#include "RsaKeyCache_fp.h"

#if CRT_FORMAT_RSA == YES 
#define CACHE_FILE_NAME "RsaKeyCacheCrt.data"
#else
#define CACHE_FILE_NAME "RsaKeyCacheNoCrt.data"
#endif

typedef struct _RSA_KEY_CACHE_
{
    TPM2B_PUBLIC_KEY_RSA        publicModulus;
    TPM2B_PRIVATE_KEY_RSA       privatePrime;
    privateExponent_t           privateExponent;
} RSA_KEY_CACHE;

// Determine the number of RSA key sizes for the cache
#ifdef RSA_KEY_SIZE_BITS_1024
#define RSA_1024    YES
#else
#define RSA_1024    NO
#endif
#ifdef RSA_KEY_SIZE_BITS_2048
#define RSA_2048    YES
#else
#define RSA_2048    NO
#endif
#ifdef RSA_KEY_SIZE_BITS_3072
#define RSA_3072    YES
#else
#define RSA_3072    NO
#endif
#ifdef RSA_KEY_SIZE_BITS_4096
#define RSA_4096    YES
#else
#define RSA_4096    NO
#endif


#define comma
TPMI_RSA_KEY_BITS       SupportedRsaKeySizes[] = {
#if RSA_1024
    1024
#   undef comma
#   define comma ,
#endif
#if RSA_2048
    comma 2048
#   undef comma
#   define comma ,
#endif
#if RSA_3072
    comma 3072
#   undef comma
#   define comma ,
#endif
#if RSA_4096
    comma 4096
#endif
};

#define RSA_KEY_CACHE_ENTRIES (RSA_1024 + RSA_2048 + RSA_3072 + RSA_4096)

// The key cache holds one entry for each of the supported key sizes
RSA_KEY_CACHE        s_rsaKeyCache[RSA_KEY_CACHE_ENTRIES];
// Indicates if the key cache is loaded. It can be loaded and enabled or disabled.
BOOL                 s_keyCacheLoaded = 0;

// Indicates if the key cache is enabled
int                  s_rsaKeyCacheEnabled = FALSE;

//*** RsaKeyCacheControl()
// Used to enable and disable the RSA key cache.
LIB_EXPORT void
RsaKeyCacheControl(
    int             state
    )
{
    s_rsaKeyCacheEnabled = state;
}

//*** InitializeKeyCache()
// This will initialize the key cache and attempt to write it to a file for later
// use.
static BOOL
InitializeKeyCache(
    OBJECT              *rsaKey,            // IN/OUT: The object structure in which
                                            //          the key is created.
    RAND_STATE          *rand               // IN: if not NULL, the deterministic
                                            //     RNG state
    )
{
    int                  index;
    TPM_KEY_BITS         keySave = rsaKey->publicArea.parameters.rsaDetail.keyBits;
    const char          *fn = CACHE_FILE_NAME;
    BOOL                 OK = TRUE;
//
    s_rsaKeyCacheEnabled = FALSE;
    for(index = 0; OK && index < RSA_KEY_CACHE_ENTRIES; index++)
    {
        rsaKey->publicArea.parameters.rsaDetail.keyBits
            = SupportedRsaKeySizes[index];
        OK = (CryptRsaGenerateKey(rsaKey, rand) == TPM_RC_SUCCESS);
        if(OK)
        {
            s_rsaKeyCache[index].publicModulus = rsaKey->publicArea.unique.rsa;
            s_rsaKeyCache[index].privatePrime = rsaKey->sensitive.sensitive.rsa;
            s_rsaKeyCache[index].privateExponent = rsaKey->privateExponent;
        }
    }
    rsaKey->publicArea.parameters.rsaDetail.keyBits = keySave;
    s_keyCacheLoaded = OK;
#if SIMULATION && USE_RSA_KEY_CACHE && USE_KEY_CACHE_FILE
    if(OK)
    {
        FILE                *cacheFile;
#if defined _MSC_VER && 1
        if(fopen_s(&cacheFile, fn, "w+b") != 0)
#else
        cacheFile = fopen(fn, "w+b");
        if(NULL == cacheFile)
#endif
        {
            printf("Can't open %s for write.\n", fn);
        }
        else
        {
            fseek(cacheFile, 0, SEEK_SET);
            if(fwrite(s_rsaKeyCache, 1, sizeof(s_rsaKeyCache), cacheFile)
               != sizeof(s_rsaKeyCache))
            {
                printf("Error writing cache to %s.", fn);
            }
        }
        if(cacheFile)
            fclose(cacheFile);
    }
#endif
    return s_keyCacheLoaded;
}

static BOOL
KeyCacheLoaded(
    OBJECT              *rsaKey,            // IN/OUT: The object structure in which
                                            //          the key is created.
    RAND_STATE          *rand               // IN: if not NULL, the deterministic
                                            //     RNG state
    )
{
#if SIMULATION && USE_RSA_KEY_CACHE && USE_KEY_CACHE_FILE
    if(!s_keyCacheLoaded)
    {
        FILE            *cacheFile;
        const char *     fn = CACHE_FILE_NAME;
#if defined _MSC_VER && 1
        if(fopen_s(&cacheFile, fn, "r+b") == 0)
#else
        cacheFile = fopen(fn, "r+b");
        if(NULL != cacheFile)
#endif
        {
            fseek(cacheFile, 0L, SEEK_END);
            if(ftell(cacheFile) == sizeof(s_rsaKeyCache))
            {
                fseek(cacheFile, 0L, SEEK_SET);
                s_keyCacheLoaded = (
                    fread(&s_rsaKeyCache, 1, sizeof(s_rsaKeyCache), cacheFile)
                    == sizeof(s_rsaKeyCache));
            }
            fclose(cacheFile);
        }
    }
#endif
    if(!s_keyCacheLoaded)
        s_rsaKeyCacheEnabled = InitializeKeyCache(rsaKey, rand);
    return s_keyCacheLoaded;
}

BOOL
GetCachedRsaKey(
    OBJECT              *key,
    RAND_STATE          *rand               // IN: if not NULL, the deterministic
                                            //     RNG state
    )
{
    int                      keyBits = key->publicArea.parameters.rsaDetail.keyBits;
    int                      index;
//
    if(KeyCacheLoaded(key, rand))
    {
        for(index = 0; index < RSA_KEY_CACHE_ENTRIES; index++)
        {
            if((s_rsaKeyCache[index].publicModulus.t.size * 8) == keyBits)
            {
                key->publicArea.unique.rsa = s_rsaKeyCache[index].publicModulus;
                key->sensitive.sensitive.rsa = s_rsaKeyCache[index].privatePrime;
                key->privateExponent = s_rsaKeyCache[index].privateExponent;
                key->attributes.privateExp = SET;
                return TRUE;
            }
        }
        return FALSE;
    }
    return s_keyCacheLoaded;
}
#endif  // SIMULATION && USE_RSA_KEY_CACHE
