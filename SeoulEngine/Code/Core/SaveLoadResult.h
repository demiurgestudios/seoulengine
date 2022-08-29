/**
 * \file SaveLoadResult.h
 * \brief Response codes from various save operations and utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SAVE_LOAD_RESULT_H
#define SAVE_LOAD_RESULT_H

#include "Prereqs.h"

namespace Seoul
{

// Note - error code order matters, as the SaveLoadManager uses a Min()
// check to combine the results into a single code. So, kCloudCancelled
// must come first, followed by kSuccess, followed by all errors,
// follwed by non-errors for various cloud operations.
enum class SaveLoadResult
{
	kCloudCancelled,
	kSuccess,

	kErrorBackupCreateDeleteOld,
	kErrorBackupDelete,
	kErrorBackupRestore,
	kErrorBase64,
	kErrorCompression,
	kErrorChecksumCheck,
	kErrorChecksumData,
	kErrorCyclicalMigrations,
	kErrorDirectoryCreate,
	kErrorDiffApply,
	kErrorDiffGenerate,
	kErrorEncryption,
	kErrorExtraData,
	kErrorFileFlush,
	kErrorFileNotFound,
	kErrorFileOp,
	kErrorFileWriteAccess,
	kErrorFileWriteBadFileDescriptor,
	kErrorFileWriteBigFile,
	kErrorFileWriteEOF,
	kErrorFileWriteExist,
	kErrorFileWriteInvalid,
	kErrorFileWriteIo,
	kErrorFileWriteIsDir,
	kErrorFileWriteNameTooLong,
	kErrorFileWriteNoBufferSpace,
	kErrorFileWriteNoEntity,
	kErrorFileWriteNoSpace,
	kErrorFileWriteNotSupported,
	kErrorFileWriteReadOnly,
	kErrorFileWriteTooManyProcess,
	kErrorFileWriteTooManySystem,
	kErrorFileWriteUnknown,
	kErrorFutureMigrationVersion,
	kErrorJsonParse,
	kErrorLookup,
	kErrorMD5Check,
	kErrorMD5Data,
	kErrorMigrationCallback,
	kErrorNetworkFailure,
	kErrorNoMigrations,
	kErrorRenameAccess,
	kErrorRenameBusy,
	kErrorRenameExist,
	kErrorRenameInvalid,
	kErrorRenameIo,
	kErrorRenameNameTooLong,
	kErrorRenameNoEntity,
	kErrorRenameNoSpace,
	kErrorRenameReadOnly,
	kErrorRenameUnknown,
	kErrorSaveCheck,
	kErrorSaveData,
	kErrorSerialization,
	kErrorServerInternalFailure,
	kErrorServerRejection,
	kErrorSessionGuid,
	kErrorSignatureCheck,
	kErrorSignatureData,
	kErrorTooBig,
	kErrorTooSmall,
	kErrorTransactionIdMax,
	kErrorTransactionIdMin,
	kErrorUnknown,
	kErrorUserVersion,
	kErrorVersionCheck,
	kErrorVersionData,

	kCloudDisabled,
	kCloudNeedsFullCheckpoint,
	kCloudNoUpdate,
	kCloudRateLimiting,
};

} // namespace Seoul

#endif // include guard
