////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  WjCryptLib_AesCfb
//
//  Implementation of AES CFB cipher.
//
//  Depends on: CryptoLib_Aes
//
//  AES CFB is a cipher using AES in Cipher FeedBack mode. Encryption and decryption must be performed in
//  multiples of the AES block size (128 bits).
//  This implementation works on both little and big endian architectures.
//
//  This is free and unencumbered software released into the public domain - January 2020 Joseph A. Zupko <jazupko@jazupko.com>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  IMPORTS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include "WjCryptLib_Aes.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  TYPES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define AES_CFB_IV_SIZE             AES_BLOCK_SIZE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  TYPES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// AesCfbContext
// Do not modify the contents of this structure directly.
typedef struct
{
    AesContext      Aes;
    uint8_t         PreviousCipherBlock [AES_BLOCK_SIZE];
} AesCfbContext;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  AesCfbInitialise
//
//  Initialises an AesCfbContext with an already initialised AesContext and a IV. This function can quickly be used
//  to change the IV without requiring the more lengthy processes of reinitialising an AES key.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
    AesCfbInitialise
    (
        AesCfbContext*      Context,                // [out]
        AesContext const*   InitialisedAesContext,  // [in]
        uint8_t const       IV [AES_CFB_IV_SIZE]    // [in]
    );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  AesCfbInitialiseWithKey
//
//  Initialises an AesCfbContext with an AES Key and an IV. This combines the initialising an AES Context and then
//  running AesCfbInitialise. KeySize must be 16, 24, or 32 (for 128, 192, or 256 bit key size)
//  Returns 0 if successful, or -1 if invalid KeySize provided
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
    AesCfbInitialiseWithKey
    (
        AesCfbContext*      Context,                // [out]
        uint8_t const*      Key,                    // [in]
        uint32_t            KeySize,                // [in]
        uint8_t const       IV [AES_CFB_IV_SIZE]    // [in]
    );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  AesCfbEncrypt
//
//  Encrypts a buffer of data using an AES CFB context. The data buffer must be a multiple of 16 bytes (128 bits)
//  in size. The "position" of the context will be advanced by the buffer amount. A buffer can be encrypted in one
//  go or in smaller chunks at a time. The result will be the same as long as data is fed into the function in the
//  same order.
//  InBuffer and OutBuffer can point to the same location for in-place encrypting.
//  Returns 0 if successful, or -1 if Size is not a multiple of 16 bytes.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
    AesCfbEncrypt
    (
        AesCfbContext*      Context,                // [in out]
        void const*         InBuffer,               // [in]
        void*               OutBuffer,              // [out]
        uint32_t            Size                    // [in]
    );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  AesCfbDecrypt
//
//  Decrypts a buffer of data using an AES CFB context. The data buffer must be a multiple of 16 bytes (128 bits)
//  in size. The "position" of the context will be advanced by the buffer amount.
//  InBuffer and OutBuffer can point to the same location for in-place decrypting.
//  Returns 0 if successful, or -1 if Size is not a multiple of 16 bytes.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
    AesCfbDecrypt
    (
        AesCfbContext*      Context,                // [in out]
        void const*         InBuffer,               // [in]
        void*               OutBuffer,              // [out]
        uint32_t            Size                    // [in]
    );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  AesCfbEncryptWithKey
//
//  This function combines AesCfbInitialiseWithKey and AesCfbEncrypt. This is suitable when encrypting data in one go
//  with a key that is not going to be reused.
//  InBuffer and OutBuffer can point to the same location for inplace encrypting.
//  Returns 0 if successful, or -1 if invalid KeySize provided or BufferSize not a multiple of 16 bytes.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
    AesCfbEncryptWithKey
    (
        uint8_t const*      Key,                    // [in]
        uint32_t            KeySize,                // [in]
        uint8_t const       IV [AES_CFB_IV_SIZE],   // [in]
        void const*         InBuffer,               // [in]
        void*               OutBuffer,              // [out]
        uint32_t            BufferSize              // [in]
    );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  AesCfbDecryptWithKey
//
//  This function combines AesCfbInitialiseWithKey and AesCfbDecrypt. This is suitable when decrypting data in one go
//  with a key that is not going to be reused.
//  InBuffer and OutBuffer can point to the same location for inplace decrypting.
//  Returns 0 if successful, or -1 if invalid KeySize provided or BufferSize not a multiple of 16 bytes.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
    AesCfbDecryptWithKey
    (
        uint8_t const*      Key,                    // [in]
        uint32_t            KeySize,                // [in]
        uint8_t const       IV [AES_CFB_IV_SIZE],   // [in]
        void const*         InBuffer,               // [in]
        void*               OutBuffer,              // [out]
        uint32_t            BufferSize              // [in]
    );
