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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  IMPORTS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "WjCryptLib_AesCfb.h"
#include "WjCryptLib_Aes.h"
#include <stdint.h>
#include <memory.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  MACROS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MIN( x, y ) ( ((x)<(y))?(x):(y) )

#define STORE64H( x, y )                                                       \
   { (y)[0] = (uint8_t)(((x)>>56)&255); (y)[1] = (uint8_t)(((x)>>48)&255);     \
     (y)[2] = (uint8_t)(((x)>>40)&255); (y)[3] = (uint8_t)(((x)>>32)&255);     \
     (y)[4] = (uint8_t)(((x)>>24)&255); (y)[5] = (uint8_t)(((x)>>16)&255);     \
     (y)[6] = (uint8_t)(((x)>>8)&255);  (y)[7] = (uint8_t)((x)&255); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  INTERNAL FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  XorAesBlock
//
//  Takes two source blocks (size AES_BLOCK_SIZE) and XORs them together and puts the result in first block
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static
void
    XorAesBlock
    (
        uint8_t*            Block1,          // [in out]
        uint8_t const*      Block2           // [in]
    )
{
    uint32_t    i;

    for( i=0; i<AES_BLOCK_SIZE; i++ )
    {
        Block1[i] ^= Block2[i];
    }
}

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
    )
{
    // Setup context values
    Context->Aes = *InitialisedAesContext;
    memcpy( Context->PreviousCipherBlock, IV, sizeof(Context->PreviousCipherBlock) );
}

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
    )
{
    AesContext aes;

    // Initialise AES Context
    if( 0 != AesInitialise( &aes, Key, KeySize ) )
    {
        return -1;
    }

    // Now set-up AesCfbContext
    AesCfbInitialise( Context, &aes, IV );
    return 0;
}

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
    )
{
    const int   inPlace = (InBuffer == OutBuffer);
    uint32_t    numBlocks = Size / AES_BLOCK_SIZE;
    uint32_t    offset = 0;
    uint32_t    i;

    if( 0 != Size % AES_BLOCK_SIZE )
    {
        // Size not a multiple of AES block size (16 bytes).
        return -1;
    }

    for( i=0; i<numBlocks; i++ )
    {
        // Encrypt the previous/initial block.
        AesEncryptInPlace( &Context->Aes, Context->PreviousCipherBlock );

        // Copy the input into the output.
        if ( !inPlace )
        {
            memmove( (uint8_t*)OutBuffer + offset, (uint8_t const*)InBuffer + offset, AES_BLOCK_SIZE );
        }

        // XOR the ciphered previous/initial block with the new data.
        XorAesBlock( (uint8_t*)OutBuffer + offset, Context->PreviousCipherBlock );

        // Prep the resulting block for the next pass.
        memcpy( Context->PreviousCipherBlock, (uint8_t const*)OutBuffer + offset, AES_BLOCK_SIZE );

        offset += AES_BLOCK_SIZE;
    }

    return 0;
}

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
    )
{
    const int   inPlace = (InBuffer == OutBuffer);
    uint32_t    numBlocks = Size / AES_BLOCK_SIZE;
    uint32_t    offset = 0;
    uint32_t    i;
    uint8_t     previousCipherBlock [AES_BLOCK_SIZE];

    if( 0 != Size % AES_BLOCK_SIZE )
    {
        // Size not a multiple of AES block size (16 bytes).
        return -1;
    }

    for( i=0; i<numBlocks; i++ )
    {
        // Copy previous cipher block and place current one in context
        memcpy( previousCipherBlock, Context->PreviousCipherBlock, AES_BLOCK_SIZE );
        memcpy( Context->PreviousCipherBlock, (uint8_t const*)InBuffer + offset, AES_BLOCK_SIZE );

        // "Encrypt" the cipher block - CFB inverts the operation relative to CBC.
        AesEncrypt( &Context->Aes, previousCipherBlock, previousCipherBlock );

        // Copy the input into the output.
        if (!inPlace)
        {
            memcpy((uint8_t*)OutBuffer + offset, Context->PreviousCipherBlock, AES_BLOCK_SIZE);
        }

        // XOR on previous cipher block
        XorAesBlock( (uint8_t*)OutBuffer + offset, previousCipherBlock );

        offset += AES_BLOCK_SIZE;
    }

    return 0;
}

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
    )
{
    int             error;
    AesCfbContext   context;

    error = AesCfbInitialiseWithKey( &context, Key, KeySize, IV );
    if( 0 == error )
    {
        error = AesCfbEncrypt( &context, InBuffer, OutBuffer, BufferSize );
    }

    return error;
}

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
    )
{
    int             error;
    AesCfbContext   context;

    error = AesCfbInitialiseWithKey( &context, Key, KeySize, IV );
    if( 0 == error )
    {
        error = AesCfbDecrypt( &context, InBuffer, OutBuffer, BufferSize );
    }

    return error;
}
