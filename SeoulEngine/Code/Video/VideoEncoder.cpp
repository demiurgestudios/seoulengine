/**
 * \file VideoEncoder.cpp
 * \brief Utility for writing encoded video streams of various codecs
 * and PCM encoded audio streams into an AVI container.
 *
 * \sa https://msdn.microsoft.com/en-us/library/windows/desktop/dd318189(v=vs.85).aspx
 * \sa https://msdn.microsoft.com/en-us/library/windows/desktop/dd388641%28v=vs.85%29.aspx
 * \sa https://msdn.microsoft.com/en-us/library/ms783421.aspx
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "Logger.h"
#include "SeoulFile.h"
#include "VideoEncoder.h"

namespace Seoul::Video
{

namespace
{

#define SEOUL_MAKEFOURCC(ch0, ch1, ch2, ch3) \
	((UInt32)(UInt8)(ch0) | ((UInt32)(UInt8)(ch1) << 8) |   \
	((UInt32)(UInt8)(ch2) << 16) | ((UInt32)(UInt8)(ch3) << 24 ))

static const UInt32 kuAviHasIndex      = 0x00000010;
static const UInt32 kuAviIsInterleaved = 0x00000100;
static const UInt32 kuAviKeyFrame      = 0x00000010;
static const UInt32 kuAviTrustCkType   = 0x00000800;
static const UInt32 kuBitsPerSample    = 16u; // TODO: Don't hardcode 16-bit audio depth.

static const UInt32 GetBlockAlign(UInt32 uChannels, UInt32 uBitsPerSample)
{
	return (uChannels * uBitsPerSample) / 8u;
}

template <typename T>
inline T QuantizeAudioSample(Float f);
template <>
inline Int16 QuantizeAudioSample<Int16>(Float f)
{
	// See FMOD source code, fmod_dsp_convert.cpp, DSPI::convert(),
	// out format set to FMOD_SOUND_FORMAT_PCM16.
	return (Int16)Clamp((Int32)(f * ((Float)(1 << 15))), -32768, 32767);
}

static const UInt32 kAudsFourCC = SEOUL_MAKEFOURCC('a', 'u', 'd', 's');
static const UInt32 kAudioDataFourCC = SEOUL_MAKEFOURCC('0', '1', 'w', 'b');
static const UInt32 kAviFileTypeFourCC = SEOUL_MAKEFOURCC('A', 'V', 'I', ' ');
static const UInt32 kAviHeaderFourCC = SEOUL_MAKEFOURCC('a', 'v', 'i', 'h');
static const UInt32 kCompressedFrameFourCC = SEOUL_MAKEFOURCC('0', '0', 'd', 'c');
static const UInt32 kDibFourCC = SEOUL_MAKEFOURCC('D', 'I', 'B', ' ');
static const UInt32 kDmlhFourCC = SEOUL_MAKEFOURCC('d', 'm', 'l', 'h');
static const UInt32 kHeaderListFourCC = SEOUL_MAKEFOURCC('h', 'd', 'r', 'l');
static const UInt32 kIndexSectionFourCC = SEOUL_MAKEFOURCC('i', 'd', 'x', '1');
static const UInt32 kListFourCC = SEOUL_MAKEFOURCC('L', 'I', 'S', 'T');
static const UInt32 kMovieListFourCC = SEOUL_MAKEFOURCC('m', 'o', 'v', 'i');
static const UInt32 kOdmlFourCC = SEOUL_MAKEFOURCC('o', 'd', 'm', 'l');
static const UInt32 kRecFourCC = SEOUL_MAKEFOURCC('r', 'e', 'c', ' ');
static const UInt32 kRiffFourCC = SEOUL_MAKEFOURCC('R', 'I', 'F', 'F');
static const UInt32 kStreamFormatFourCC = SEOUL_MAKEFOURCC('s', 't', 'r', 'f');
static const UInt32 kStreamHeaderFourCC = SEOUL_MAKEFOURCC('s', 't', 'r', 'h');
static const UInt32 kStreamListFourCC = SEOUL_MAKEFOURCC('s', 't', 'r', 'l');
static const UInt32 kVidsFourCC = SEOUL_MAKEFOURCC('v', 'i', 'd', 's');

static const UInt16 kWaveFormatPCM = 1u;

static inline UInt32 ToCodecFourCC(Codec eCodec)
{
	switch (eCodec)
	{
		case Codec::kLossless: return kDibFourCC;
		default:
			SEOUL_FAIL("Out-of-sync enum.");
			return kDibFourCC;
	};
}

static inline UInt32 ToCompressionFourCC(Codec eCodec)
{
	switch (eCodec)
	{
	case Codec::kLossless: return 0u;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return 0u;
	};
}

// We write out AVI files, which are RIFF files of the following format:
//
// "
// RIFF ('AVI '
//     LIST('hdrl' ...)
//     LIST('movi' ...)
//     ['idx1' (<AVI Index>)]
//     )

//
// "A list has the following form:
//
// 'LIST' listSize listType listData
// where 'LIST' is the literal FOURCC code 'LIST', listSize is a 4 - byte value giving the size of the list, listType is a FOURCC code, and listData consists of chunks or lists, in any order.The value of listSize includes the size of listType plus the size of listData; it does not include the 'LIST' FOURCC or the size of listSize."
template <typename T>
struct RIFFList SEOUL_SEALED
{
	UInt32 m_uId;
	UInt32 m_uSize;
	UInt32 m_uType;
	T m_Data;

	RIFFList(UInt32 uType, const T& data)
		: m_uId(kListFourCC)
		, m_uSize(sizeof(m_uType) + sizeof(data))
		, m_uType(uType)
		, m_Data(data)
	{
	}
}; // struct RIFFList

template <>
struct RIFFList<void> SEOUL_SEALED
{
	UInt32 m_uId;
	UInt32 m_uSize;
	UInt32 m_uType;

	RIFFList(UInt32 uType, UInt32 uSize)
		: m_uId(kListFourCC)
		, m_uSize(sizeof(m_uType) + uSize)
		, m_uType(uType)
	{
	}
}; // struct RIFFList

// "A chunk has the following form :
//
// "ckID ckSize ckData
//
// "where ckID is a FOURCC that identifies the data contained in the chunk, ckSize is a 4 - byte value giving the size of the data in ckData, and ckData is zero or more bytes of data.The data is always padded to nearest WORD boundary.ckSize gives the size of the valid data in the chunk; it does not include the padding, the size of ckID, or the size of ckSize."
template <typename T>
struct RIFFChunk SEOUL_SEALED
{
	UInt32 m_uId;
	UInt32 m_uSize;
	T m_Data;

	RIFFChunk(UInt32 uId, const T& data)
		: m_uId(uId)
		, m_uSize(sizeof(data))
		, m_Data(data)
	{
	}
}; // struct RIFFChunk

template <>
struct RIFFChunk<void> SEOUL_SEALED
{
	UInt32 m_uId;
	UInt32 m_uSize;

	RIFFChunk(UInt32 uId, UInt32 uSize)
		: m_uId(uId)
		, m_uSize(uSize)
	{
		// Sanity check to verify padding to nearest WORD boundary.
		SEOUL_ASSERT(uSize % sizeof(UInt16) == 0);
	}
}; // struct RIFFChunk

struct AVIMainHeader SEOUL_SEALED
{
	UInt32 m_uMicrosecondsPerFrame;
	UInt32 m_uMaxBytesPerSec;
	UInt32 m_uPaddingGranularity;
	UInt32 m_uFlags;
	UInt32 m_uTotalFrames;
	UInt32 m_uInitialFrames;
	UInt32 m_uStreams;
	UInt32 m_uSuggestedBufferSize;
	UInt32 m_uWidth;
	UInt32 m_uHeight;
	UInt32 m_uReserved0;
	UInt32 m_uReserved1;
	UInt32 m_uReserved2;
	UInt32 m_uReserved3;

	AVIMainHeader(
		UInt32 uStreams,
		UInt32 uFrames,
		UInt32 uWidth,
		UInt32 uHeight,
		UInt32 uMicrosecondsPerFrame,
		UInt32 uTotalDataSizeInBytes,
		UInt32 uMaxBytesPerFrame)
		: m_uMicrosecondsPerFrame(uMicrosecondsPerFrame)
		, m_uMaxBytesPerSec(1000000u * (uTotalDataSizeInBytes / Max(uFrames, 1u)) / uMicrosecondsPerFrame)
		, m_uPaddingGranularity(0u)
		, m_uFlags(kuAviHasIndex | kuAviIsInterleaved | kuAviTrustCkType)
		, m_uTotalFrames(uFrames)
		, m_uInitialFrames(0u)
		, m_uStreams(uStreams)
		, m_uSuggestedBufferSize(uMaxBytesPerFrame)
		, m_uWidth(uWidth)
		, m_uHeight(uHeight)
		, m_uReserved0(0u)
		, m_uReserved1(0u)
		, m_uReserved2(0u)
		, m_uReserved3(0u)
	{
	}
}; // struct AVIMainHeader

/** Base value of the timebase denominator - a second worth of microseconds. */
static const UInt32 kuBaseRate = 1000000u;

// TODO: Break out into a utility function - probably
// a more clever/fast implementation of this as well.

/** GCD of two unsigned integers. */
static inline UInt32 GreatestCommonDivisor(UInt32 a, UInt32 b)
{
	while (b > 0u)
	{
		auto const u = (a % b);
		a = b;
		b = u;
	}

	return a;
}

/** Numerator of the timebase of the AVI. */
static inline UInt32 MicrosecondsToVideoScale(UInt32 u)
{
	return (u / GreatestCommonDivisor(kuBaseRate, u));
}

/** Denominator of the timebase of the AVI. */
static inline UInt32 MicrosecondsToVideoRate(UInt32 u)
{
	return (kuBaseRate / GreatestCommonDivisor(kuBaseRate, u));
}

struct AVIStreamHeader SEOUL_SEALED
{
	UInt32 m_uType;
	UInt32 m_uHandler;
	UInt32 m_uFlags;
	UInt16 m_uPriority;
	UInt16 m_uLanguage;
	UInt32 m_uInitialFrames;
	UInt32 m_uScale;
	UInt32 m_uRate;
	UInt32 m_uStart;
	UInt32 m_uLength;
	UInt32 m_uSuggestedBufferSize;
	UInt32 m_uQuality;
	UInt32 m_uSampleSize;
	UInt16 m_uLeft;
	UInt16 m_uTop;
	UInt16 m_uRight;
	UInt16 m_uBottom;

	AVIStreamHeader(
		UInt32 uTotalSizeInBytes,
		UInt32 uSamplesPerSecond,
		UInt32 uBlockAlignment,
		UInt32 uMaxBytesPerFrame)
		: m_uType(kAudsFourCC)
		, m_uHandler(0u)
		, m_uFlags(0u)
		, m_uPriority(0u)
		, m_uLanguage(0u)
		, m_uInitialFrames(0u)
		, m_uScale(uBlockAlignment)
		, m_uRate(uSamplesPerSecond * uBlockAlignment)
		, m_uStart(0u)
		, m_uLength(uTotalSizeInBytes / uBlockAlignment)
		, m_uSuggestedBufferSize(uMaxBytesPerFrame)
		, m_uQuality((UInt32)-1)
		, m_uSampleSize(uBlockAlignment)
		, m_uLeft(0u)
		, m_uTop(0u)
		, m_uRight(0u)
		, m_uBottom(0u)
	{
	}

	AVIStreamHeader(
		Codec eCodec,
		UInt32 uWidth,
		UInt32 uHeight,
		UInt32 uFrames,
		UInt32 uMicrosecondsPerFrame,
		UInt32 uMaxBytesPerFrame)
		: m_uType(kVidsFourCC)
		, m_uHandler(ToCodecFourCC(eCodec))
		, m_uFlags(0u)
		, m_uPriority(0u)
		, m_uLanguage(0u)
		, m_uInitialFrames(0u)
		, m_uScale(MicrosecondsToVideoScale(uMicrosecondsPerFrame))
		, m_uRate(MicrosecondsToVideoRate(uMicrosecondsPerFrame))
		, m_uStart(0u)
		, m_uLength(uFrames)
		, m_uSuggestedBufferSize(uMaxBytesPerFrame)
		, m_uQuality((UInt32)-1)
		, m_uSampleSize(0u)
		, m_uLeft(0u)
		, m_uTop(0u)
		, m_uRight(uWidth)
		, m_uBottom(uHeight)
	{
	}
}; // struct AVIStreamHeader

struct AVIBitmapInfoHeader SEOUL_SEALED
{
	UInt32 m_uBufferSize;
	UInt32 m_uWidth;
	UInt32 m_uHeight;
	UInt16 m_uPlanes;
	UInt16 m_uBitCount;
	UInt32 m_uCompression;
	UInt32 m_uImageSize;
	UInt32 m_uXPixelsPerMeter;
	UInt32 m_uYPixelsPerMeter;
	UInt32 m_uUsedColors;
	UInt32 m_uImportantColors;

	AVIBitmapInfoHeader(Codec eCodec, UInt32 uWidth, UInt32 uHeight)
		: m_uBufferSize(sizeof(AVIBitmapInfoHeader))
		, m_uWidth(uWidth)
		, m_uHeight(uHeight)
		, m_uPlanes(1u)
		, m_uBitCount(24u) // TODO:
		, m_uCompression(ToCompressionFourCC(eCodec))
		, m_uImageSize(uWidth * uHeight * 3u) // TODO:
		, m_uXPixelsPerMeter(0u)
		, m_uYPixelsPerMeter(0u)
		, m_uUsedColors(0u)
		, m_uImportantColors(0u)
	{
	}
}; // struct AVIBitmapInfoHeader

struct AVIWaveFormatExHeader SEOUL_SEALED
{
	UInt16 m_uFormatTag;
	UInt16 m_uChannels;
	UInt32 m_uSamplesPerSec;
	UInt32 m_uAvgBytesPerSec;
	UInt16 m_uBlockAlign;
	UInt16 m_uBitsPerSample;
	/* TODO: Annoying - need this if not kWaveFormatPCM, alignment issue otherwise
	   UInt16 m_uExtraSize; */

	AVIWaveFormatExHeader(UInt32 uSamplesPerSecond, UInt32 uChannels)
		: m_uFormatTag(kWaveFormatPCM)
		, m_uChannels(uChannels)
		, m_uSamplesPerSec(uSamplesPerSecond)
		, m_uAvgBytesPerSec(uSamplesPerSecond * GetBlockAlign(uChannels, kuBitsPerSample))
		, m_uBlockAlign(GetBlockAlign(uChannels, kuBitsPerSample))
		, m_uBitsPerSample(kuBitsPerSample)
	{
	}
}; // struct AVIWaveFormatExHeader

struct AVIAudioStreamHeader SEOUL_SEALED
{
	RIFFChunk<AVIStreamHeader> m_StreamHeader;
	RIFFChunk<AVIWaveFormatExHeader> m_StreamFormat;

	AVIAudioStreamHeader(UInt32 uTotalSizeInBytes, UInt32 uSamplesPerSecond, UInt32 uChannels, UInt32 uMaxBytesPerFrame)
		: m_StreamHeader(kStreamHeaderFourCC, AVIStreamHeader(uTotalSizeInBytes, uSamplesPerSecond, GetBlockAlign(uChannels, kuBitsPerSample), uMaxBytesPerFrame))
		, m_StreamFormat(kStreamFormatFourCC, AVIWaveFormatExHeader(uSamplesPerSecond, uChannels))
	{
	}
}; // struct FAVIListStream

struct AVIVideoStreamHeader SEOUL_SEALED
{
	RIFFChunk<AVIStreamHeader> m_StreamHeader;
	RIFFChunk<AVIBitmapInfoHeader> m_StreamFormat;

	AVIVideoStreamHeader(Codec eCodec, UInt32 uFrames, UInt32 uWidth, UInt32 uHeight, UInt32 uMicrosecondsPerFrame, UInt32 uMaxBytesPerFrame)
		: m_StreamHeader(kStreamHeaderFourCC, AVIStreamHeader(eCodec, uWidth, uHeight, uFrames, uMicrosecondsPerFrame, uMaxBytesPerFrame))
		, m_StreamFormat(kStreamFormatFourCC, AVIBitmapInfoHeader(eCodec, uWidth, uHeight))
	{
	}
}; // struct FAVIListStream

struct AVIVideAudioListHeader SEOUL_SEALED
{
	RIFFChunk<AVIMainHeader> m_MainHeader;
	RIFFList<AVIVideoStreamHeader> m_VideoStreamHeader;
	RIFFList<AVIAudioStreamHeader> m_AudioStreamHeader;
	RIFFList< RIFFChunk<UInt32> > m_OdmlExtendedHeader;

	AVIVideAudioListHeader(
		Codec eCodec,
		UInt32 uAudioTotalSizeInBytes,
		UInt32 uSamplesPerSecond,
		UInt32 uChannels,
		UInt32 uVideoFrames,
		UInt32 uWidth,
		UInt32 uHeight,
		UInt32 uMicrosecondsPerFrame,
		UInt32 uTotalDataSizeInBytes,
		UInt32 uMaxTotalBytesPerFrame,
		UInt32 uMaxAudioBytesPerFrame,
		UInt32 uMaxVideoBytesPerFrame)
		: m_MainHeader(kAviHeaderFourCC, AVIMainHeader(2u, uVideoFrames, uWidth, uHeight, uMicrosecondsPerFrame, uTotalDataSizeInBytes, uMaxTotalBytesPerFrame))
		, m_VideoStreamHeader(kStreamListFourCC, AVIVideoStreamHeader(eCodec, uVideoFrames, uWidth, uHeight, uMicrosecondsPerFrame, uMaxVideoBytesPerFrame))
		, m_AudioStreamHeader(kStreamListFourCC, AVIAudioStreamHeader(uAudioTotalSizeInBytes, uSamplesPerSecond, uChannels, uMaxAudioBytesPerFrame))
		, m_OdmlExtendedHeader(kOdmlFourCC, RIFFChunk<UInt32>(kDmlhFourCC, uVideoFrames))
	{
	}
}; // struct AVIVideAudioListHeader

struct AVIVideOnlyListHeader SEOUL_SEALED
{
	RIFFChunk<AVIMainHeader> m_MainHeader;
	RIFFList<AVIVideoStreamHeader> m_VideoStreamHeader;
	RIFFList< RIFFChunk<UInt32> > m_OdmlExtendedHeader;

	AVIVideOnlyListHeader(
		Codec eCodec,
		UInt32 uFrames,
		UInt32 uWidth,
		UInt32 uHeight,
		UInt32 uMicrosecondsPerFrame,
		UInt32 uTotalDataSizeInBytes,
		UInt32 uMaxBytesPerFrame)
		: m_MainHeader(kAviHeaderFourCC, AVIMainHeader(1u, uFrames, uWidth, uHeight, uMicrosecondsPerFrame, uTotalDataSizeInBytes, uMaxBytesPerFrame))
		, m_VideoStreamHeader(kStreamListFourCC, AVIVideoStreamHeader(eCodec, uFrames, uWidth, uHeight, uMicrosecondsPerFrame, uMaxBytesPerFrame))
		, m_OdmlExtendedHeader(kOdmlFourCC, RIFFChunk<UInt32>(kDmlhFourCC, uFrames))
	{
	}
}; // struct AVIListHeader

struct AVIIdx1Entry SEOUL_SEALED
{
	UInt32 m_uId;
	UInt32 m_uFlags;
	UInt32 m_uOffset;
	UInt32 m_uSize;

	AVIIdx1Entry(
		UInt32 uId,
		UInt32 uFlags,
		UInt32 uOffset,
		UInt32 uSize)
		: m_uId(uId)
		, m_uFlags(uFlags)
		, m_uOffset(uOffset)
		, m_uSize(uSize)
	{
	}
}; // struct AVIIdx1Entry

} // namespace anonymous

Encoder::Encoder(
	Codec eCodec,
	const String& sOutput,
	UInt32 uFPS,
	UInt32 uWidth,
	UInt32 uHeight,
	Bool bAudio,
	UInt32 uSamplingRate,
	UInt32 uChannels)
	: m_eCodec(eCodec)
	, m_sOutput(sOutput)
	, m_uFPS(uFPS)
	, m_uWidth(uWidth)
	, m_uHeight(uHeight)
	, m_bAudio(bAudio)
	, m_uSamplingRate(uSamplingRate)
	, m_uChannels(uChannels)
	, m_iMovieDataStart(0)
	, m_uAudioSizeInBytes(0u)
	, m_uVideoFrames(0u)
	, m_uTotalDataSizeInBytes(0u)
	, m_uMaxAudioBytesPerFrame(0u)
	, m_uMaxTotalBytesPerFrame(0u)
	, m_uMaxVideoBytesPerFrame(0u)
	, m_vSoundScratch()
	, m_pFile()
	, m_bOk(true)
	, m_vFrames()
{
}

Encoder::~Encoder()
{
	// Finish and close.
	if (m_bOk)
	{
		Finish();
	}
	m_pFile.Reset();
}

Bool Encoder::AddVideoFrame(
	void const* pData,
	UInt32 uDataInBytes)
{
	// Must have a file first.
	CheckFile();
	if (!m_bOk)
	{
		return false;
	}

	WriteVideoFrame(pData, uDataInBytes);
	m_uMaxVideoBytesPerFrame = Max(m_uMaxVideoBytesPerFrame, uDataInBytes);
	m_uMaxTotalBytesPerFrame = Max(m_uMaxTotalBytesPerFrame, uDataInBytes);
	return m_bOk;
}

Bool Encoder::AddAudioVideoFrame(
	void const* pVideoData,
	UInt32 uVideoDataInBytes,
	Float const* pfAudioSamples,
	UInt32 uAudioSizeInSamples)
{
	// Audio size is expected to be exactly the number of samples
	// we need for one video frame.
	if (uAudioSizeInSamples != (m_uSamplingRate / m_uFPS))
	{
		SEOUL_WARN("%s: input video frame %u received audio samples of size %u, expected size %u.",
			m_sOutput.CStr(),
			m_uVideoFrames,
			uAudioSizeInSamples,
			(m_uSamplingRate / m_uFPS));
		return false;
	}

	// No audio, fail.
	if (!m_bAudio)
	{
		SEOUL_WARN("%s: called with audio data but no configured for audio.",
			m_sOutput.CStr());

		m_bOk = false;
		return m_bOk;
	}

	// Must have a file first.
	CheckFile();
	if (!m_bOk)
	{
		SEOUL_WARN("%s: failed with previous error, possible file open failure.",
			m_sOutput.CStr());

		return false;
	}

	// Rec list header, will contain 2 chunks.
	// Audio + video, plus the header sizes.
	auto const uAudioDataInBytes = (UInt32)(uAudioSizeInSamples * m_uChannels * sizeof(SoundSampleType));
	Write(RIFFList<void>(kRecFourCC,
		uVideoDataInBytes +
		uAudioDataInBytes +
		2 * sizeof(RIFFChunk<void>)));

	// Now write video, then audio data.
	WriteVideoFrame(pVideoData, uVideoDataInBytes);
	WriteAudioSamples(pfAudioSamples, uAudioSizeInSamples);
	m_uMaxAudioBytesPerFrame = Max(m_uMaxAudioBytesPerFrame, uAudioDataInBytes);
	m_uMaxTotalBytesPerFrame = Max(m_uMaxTotalBytesPerFrame, uVideoDataInBytes + uAudioDataInBytes);
	m_uMaxVideoBytesPerFrame = Max(m_uMaxVideoBytesPerFrame, uVideoDataInBytes);

	// Align to a 2k boundary.
	Align(2048);

	return m_bOk;
}

Bool Encoder::Finish()
{
	// Nothing to finish if never started.
	if (!m_pFile.IsValid())
	{
		return m_bOk;
	}

	// Compute total - this is movie data
	// start *plus* the size of a type record,
	// since the move data start (oddly) includes
	// the type record, but data starts with
	// the first video/audio frame. So we need
	// to subtract out an additional 4 bytes
	// (the size of m_uType) to get the area
	// used for storing actual frame/audio data.
	m_uTotalDataSizeInBytes = (UInt32)(
		Pos() - m_iMovieDataStart - sizeof(RIFFList<void>::m_uType));

	// Sanity check that padding/alignment to WORD boundary
	// was enforced correctly.
	SEOUL_ASSERT(m_uTotalDataSizeInBytes % sizeof(UInt16) == 0);

	// Writer index data.
	UInt32 const uFrames = m_vFrames.GetSize();
	Write(RIFFChunk<void>(kIndexSectionFourCC, uFrames * sizeof(AVIIdx1Entry)));
	for (auto const& e : m_vFrames)
	{
		Write(AVIIdx1Entry(e.m_uTypeFourCC, kuAviKeyFrame, e.m_uOffsetInBytes, e.m_uSizeInBytes));
	}

	// Now that we've fully written the file, rewrite
	// the header - this will rewind to the head
	// and commit final values.
	WriteHeader();
	return m_bOk;
}

void Encoder::CheckFile()
{
	// Early out if already opened.
	if (m_pFile.IsValid() && m_pFile->CanWrite())
	{
		return;
	}

	// Make sure the directory structure exists for the file.
	m_bOk = m_bOk && FileManager::Get()->CreateDirPath(Path::GetDirectoryName(m_sOutput));
	m_bOk = m_bOk && FileManager::Get()->OpenFile(m_sOutput, File::kWriteTruncate, m_pFile);
	m_bOk = m_bOk && m_pFile->CanWrite();

	// Write out the placeholder header data - will be
	// replaced once the file has been populated.
	WriteHeader();
}

void Encoder::WriteHeader()
{
	// Measure file size, then seek to beginning.
	m_bOk = m_bOk && m_pFile->Seek(0, File::kSeekFromEnd);
	auto const iSize = Pos();
	m_bOk = m_bOk && m_pFile->Seek(0, File::kSeekFromStart);

	// "The value of fileSize includes the size of
	// the fileType FOURCC plus the size of the data
	// that follows, but does not include the size of
	// the 'RIFF' FOURCC or the size of fileSize."
	UInt32 const uHeaderFileSize = (UInt32)(
		Max(iSize, (Int64)sizeof(RIFFChunk<void>)) - sizeof(RIFFChunk<void>));

	// Header chunk
	//
	// "The RIFF header has the following form:
	//   'RIFF' fileSize fileType(data) "
	//
	Write(RIFFChunk<void>(kRiffFourCC, uHeaderFileSize));
	Write(kAviFileTypeFourCC);

	// Computed check for list header.
	UInt32 const uMicrosecondsPerFrame = (1000000u / m_uFPS);

	// List header - different type depending on whether this
	// AVI will include audio or not.
	if (m_bAudio)
	{
		Write(RIFFList<AVIVideAudioListHeader>(
			kHeaderListFourCC,
			AVIVideAudioListHeader(
				m_eCodec,
				m_uAudioSizeInBytes,
				m_uSamplingRate,
				m_uChannels,
				m_uVideoFrames,
				m_uWidth,
				m_uHeight,
				uMicrosecondsPerFrame,
				m_uTotalDataSizeInBytes,
				m_uMaxTotalBytesPerFrame,
				m_uMaxAudioBytesPerFrame,
				m_uMaxVideoBytesPerFrame)));
	}
	else
	{
		Write(RIFFList<AVIVideOnlyListHeader>(
			kHeaderListFourCC,
			AVIVideOnlyListHeader(
				m_eCodec,
				m_uVideoFrames,
				m_uWidth,
				m_uHeight,
				uMicrosecondsPerFrame,
				m_uTotalDataSizeInBytes,
				m_uMaxTotalBytesPerFrame)));
	}

	// Write the movie list header.
	Write(RIFFList<void>(
		kMovieListFourCC,
		// Total movie data size is the written data plus all the
		// chunk headers.
		m_uTotalDataSizeInBytes));

	// Capture the header end offset - the offset actually
	// starts at the CC of the 'movi' chunk, so we need to
	// subtract the size of a type field from the current position.
	m_iMovieDataStart = Pos() - sizeof(RIFFList<void>::m_uType);
}

/**
 * Pad the stream state to be an even
 * multiple of the given number of bytes.
 */
UInt32 Encoder::Align(UInt32 uBytes)
{
	UInt32 uReturn = 0u;

	auto iOffset = Pos();
	auto const iAlignment = (Int64)uBytes;
	while (iOffset % iAlignment != 0)
	{
		Write((UInt8)'\0');
		++iOffset;
		++uReturn;
	}

	return uReturn;
}

/** Retrieve current stream position - 0 on failure. */
Int64 Encoder::Pos()
{
	Int64 i = 0;
	m_bOk = m_bOk && m_pFile->GetCurrentPositionIndicator(i);
	return i;
}

template <typename T>
void Encoder::Write(const T& v)
{
	m_bOk = m_bOk && (sizeof(v) == m_pFile->WriteRawData(&v, sizeof(v)));
}
void Encoder::Write(void const* p, UInt32 u)
{
	m_bOk = m_bOk && (u == m_pFile->WriteRawData(p, u));
}

void Encoder::WriteAudioSamples(
	Float const* pfSamples,
	UInt32 uSizeInSamples)
{
	// Should have been enforced by the caller.
	SEOUL_ASSERT(m_pFile.IsValid());

	// Cache offset.
	auto const iOffsetInBytes = Pos();

	auto const uTotalSamples = (uSizeInSamples * m_uChannels);
	m_vSoundScratch.Clear();
	m_vSoundScratch.Reserve(uTotalSamples);
	for (auto i = 0u; i < uTotalSamples; ++i)
	{
		auto const f = pfSamples[i];
		auto const u = QuantizeAudioSample<Scratch::ValueType>(f);
		m_vSoundScratch.PushBack(u);
	}

	// Chunk header.
	auto const uDataInBytes = (UInt32)(uTotalSamples * sizeof(SoundSampleType));
	Write(RIFFChunk<void>(kAudioDataFourCC, uDataInBytes));

	// Data.
	if (uDataInBytes > 0u)
	{
		Write(m_vSoundScratch.Data(), m_vSoundScratch.GetSizeInBytes());
	}

	// Tracking.
	FrameEntry const entry = {
		kAudioDataFourCC,
		(UInt32)(iOffsetInBytes - m_iMovieDataStart),
		uDataInBytes,
	};
	m_vFrames.PushBack(entry);
	m_uAudioSizeInBytes += uDataInBytes;

	// Align to 4 bytes
	m_uAudioSizeInBytes += Align(2u);
}

void Encoder::WriteVideoFrame(
	void const* pData,
	UInt32 uDataInBytes)
{
	// Should have been enforced by the caller.
	SEOUL_ASSERT(m_pFile.IsValid());

	// Cache offset.
	auto const iOffsetInBytes = Pos();

	// Chunk header.
	Write(RIFFChunk<void>(kCompressedFrameFourCC, uDataInBytes));

	// Replace JFIF with AVI1.
	Write(pData, uDataInBytes);

	// Tracking.
	FrameEntry const entry = {
		kCompressedFrameFourCC,
		(UInt32)(iOffsetInBytes - m_iMovieDataStart),
		uDataInBytes,
	};
	m_vFrames.PushBack(entry);
	++m_uVideoFrames;

	// Align to 2 bytes
	Align(2u);
}

#undef SEOUL_MAKEFOURCC

} // namespace Seoul::Video
