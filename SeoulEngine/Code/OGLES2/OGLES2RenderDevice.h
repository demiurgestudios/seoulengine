/**
 * \file OGLES2RenderDevice.h
 * \brief Specialization of RenderDevice for the OGLES2 platform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_RENDER_DEVICE_H
#define OGLES2_RENDER_DEVICE_H

#include "AtomicRingBuffer.h"
#include "Delegate.h"
#include "Engine.h"
#include "FixedArray.h"
#include "JobsManager.h"
#include "RenderDevice.h"
#include "OGLES2StateManager.h"
#include "OGLES2Util.h"
#include "Once.h"
#include "ScopedPtr.h"

// Forward declarations.
#if SEOUL_PLATFORM_ANDROID
struct ANativeWindow;
typedef void* EGLConfig;
typedef void* EGLDisplay;
typedef void* EGLSurface;
#endif

namespace Seoul
{

/** Large double used for initial absolute minimum time value. */
Double const kfInitialAbsoluteMinimumTimeMs = 10000.0;

// Forward declarations
class OGLES2DepthStencilSurface;
class OGLES2IndexBuffer;
class OGLES2RenderTarget;
class OGLES2VertexFormat;

#if SEOUL_PLATFORM_ANDROID
/**
 * Convenience utility for maintaining a reference to a window handle.
 */
class OGLES2WindowHandlePtr SEOUL_SEALED
{
public:
	OGLES2WindowHandlePtr();
	explicit OGLES2WindowHandlePtr(ANativeWindow* p);
	OGLES2WindowHandlePtr(const OGLES2WindowHandlePtr& b);
	OGLES2WindowHandlePtr& operator=(const OGLES2WindowHandlePtr& b);
	~OGLES2WindowHandlePtr();

	ANativeWindow* GetPtr() const
	{
		return m_p;
	}

	Bool IsValid() const;

	void Reset(ANativeWindow* pIn = nullptr);

private:
	CheckedPtr<ANativeWindow> m_p;
}; // class OGLES2WindowHandlePtr
static inline Bool operator==(const OGLES2WindowHandlePtr& a, const OGLES2WindowHandlePtr& b)
{
	return (a.GetPtr() == b.GetPtr());
}
static inline Bool operator!=(const OGLES2WindowHandlePtr& a, const OGLES2WindowHandlePtr& b)
{
	return !(a == b);
}
static inline Bool operator==(const OGLES2WindowHandlePtr& a, ANativeWindow const* b)
{
	return (a.GetPtr() == b);
}
static inline Bool operator!=(const OGLES2WindowHandlePtr& a, ANativeWindow const* b)
{
	return !(a == b);
}
static inline Bool operator==(ANativeWindow const* a, const OGLES2WindowHandlePtr& b)
{
	return (a == b.GetPtr());
}
static inline Bool operator!=(ANativeWindow const* a, const OGLES2WindowHandlePtr& b)
{
	return !(a == b);
}

/** Structure that describes the current settings of the Android hardware scalar. */
struct OGLES2RenderDeviceHardwareScalarState SEOUL_SEALED
{
	OGLES2RenderDeviceHardwareScalarState()
		: m_iWindowHeight(0)
		, m_iBufferHeight(0)
	{
	}

	/** @return Portion of scaling applied by the hardware scalar. */
	Float GetScalingFactor() const
	{
		return (0 == m_iWindowHeight
			? 1.0f
			: ((Float)m_iBufferHeight / (Float)m_iWindowHeight));
	}

	/** @return True if the hardware scalar is scaling the back buffer, false otherwise. */
	Bool IsScaling() const
	{
		return (m_iWindowHeight != m_iBufferHeight);
	}

	Int32 m_iWindowHeight;
	Int32 m_iBufferHeight;
}; // struct OGLES2RenderDeviceHardwareScalarState
#endif // /#if SEOUL_PLATFORM_ANDROID

class OGLES2RenderDevice SEOUL_SEALED : public RenderDevice
{
public:
	SEOUL_DELEGATE_TARGET(OGLES2RenderDevice);

	static CheckedPtr<OGLES2RenderDevice> Get()
	{
		if (RenderDevice::Get() && RenderDevice::Get()->GetType() == RenderDeviceType::kOGLES2)
		{
			return (OGLES2RenderDevice*)RenderDevice::Get().Get();
		}
		else
		{
			return CheckedPtr<OGLES2RenderDevice>();
		}
	}

	// Platform specific constructors.
#if SEOUL_PLATFORM_ANDROID
	OGLES2RenderDevice(ANativeWindow* pMainWindow, const RefreshRate& refreshRate, Bool bDesireBGRA);
#elif SEOUL_PLATFORM_IOS
	OGLES2RenderDevice(void* pLayer);
#else
	OGLES2RenderDevice();
#endif
	virtual ~OGLES2RenderDevice();

	virtual RenderDeviceType GetType() const SEOUL_OVERRIDE { return RenderDeviceType::kOGLES2; }

	// Generic graphics object create method
	template <typename T>
	SharedPtr<T> Create(MemoryBudgets::Type eType)
	{
		SharedPtr<T> pReturn(SEOUL_NEW(eType) T);
		InternalAddObject(pReturn);
		return pReturn;
	}

	virtual RenderCommandStreamBuilder* CreateRenderCommandStreamBuilder(UInt32 zInitialCapacity = 0u) const SEOUL_OVERRIDE;

	virtual Bool BeginScene() SEOUL_OVERRIDE;

	virtual void EndScene() SEOUL_OVERRIDE;

	// Get the viewport of the back buffer.
	virtual const Viewport& GetBackBufferViewport() const SEOUL_OVERRIDE;

	// Get the screen refresh rate in hertz.
	virtual RefreshRate GetDisplayRefreshRate() const SEOUL_OVERRIDE;

	/**
	 * @return A shadow post projection matrix for OGLES2 rendering - OGLES2 does not
	 * use a half-pixel offset, and the texture origin is in the lower left, so the transform
	 * is always a constant scale and shift by 0.5 to convert from clip space on [-1, 1] to
	 * texture space on [0, 1].
	 */
	virtual Matrix4D GetShadowPostProjectionTransform(const Vector2D& /*vShadowTextureDimensions*/) const SEOUL_OVERRIDE
	{
		// OGLES2 does not use a half pixel offset, and textures have a lower-left origin.
		const Matrix4D kShadowPostLightProjectionTransform(
			0.5f,  0.0f, 0.0f, 0.5f,
			0.0f,  0.5f, 0.0f, 0.5f,
			0.0f,  0.0f, 1.0f, 0.0f,
			0.0f,  0.0f, 0.0f, 1.0f);

		return kShadowPostLightProjectionTransform;
	}

	// Return a VertexFormat described by pElements.
	virtual SharedPtr<VertexFormat> CreateVertexFormat(
		VertexElement const* pElements) SEOUL_OVERRIDE;

	// Create a new DepthStencilSurface as described by the json file section
	// configuration. The caller owns the returned object and must
	// destroy it with SEOUL_DELETE.
	virtual SharedPtr<DepthStencilSurface> CreateDepthStencilSurface(
		const DataStoreTableUtil& configSettings) SEOUL_OVERRIDE;

	// Create a new RenderTarget as described by the json file section
	// configuration. The caller owns the returned object and must
	// destroy it with SEOUL_DELETE.
	virtual SharedPtr<RenderTarget> CreateRenderTarget(
		const DataStoreTableUtil& configSettings) SEOUL_OVERRIDE;

	virtual SharedPtr<IndexBuffer> CreateIndexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat) SEOUL_OVERRIDE;

	virtual SharedPtr<IndexBuffer> CreateDynamicIndexBuffer(
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat) SEOUL_OVERRIDE;

	virtual SharedPtr<VertexBuffer> CreateVertexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		UInt32 zStrideInBytes) SEOUL_OVERRIDE;

	virtual SharedPtr<VertexBuffer> CreateDynamicVertexBuffer(
		UInt32 zTotalSizeInBytes,
		UInt32 zStrideInBytes) SEOUL_OVERRIDE;

	// Android conditionally supports async texture creation.
	// iOS supports async texture creation always, but we
	// still use the conditional. It is just always expected
	// to be true.
	virtual Bool SupportsAsyncCreateTexture() const SEOUL_OVERRIDE
	{
		return m_bSupportsAsyncTextureCreate;
	}

	virtual SharedPtr<BaseTexture> AsyncCreateTexture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat) SEOUL_OVERRIDE;

	virtual SharedPtr<BaseTexture> CreateTexture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat) SEOUL_OVERRIDE;

	// Effects
	virtual SharedPtr<Effect> CreateEffectFromFileInMemory(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes) SEOUL_OVERRIDE;

	// For some platforms, implements specific handling in the render system
	// on enter/exit background (on mobile devices, when the app becomes inactive,
	// it has entered the background).
	virtual void OnEnterBackground() SEOUL_OVERRIDE;
	virtual void OnLeaveBackground() SEOUL_OVERRIDE;

	virtual void SetDesiredVsyncInterval(Int iInterval) SEOUL_OVERRIDE
	{
		// There are two possibilities - on iOS,
		// currently, the interval is not supported, so this desired
		// value will never be committed to the active value. On Android,
		// we emulate the value, so it could be any range. 0-4 was
		// selected to match D3D11.
		iInterval = Clamp(iInterval, 0, 4);
		RenderDevice::SetDesiredVsyncInterval(iInterval);
	}

	// Apply any changes to the active depth-stencil and color targets.
	void CommitRenderSurface();

	/**
	 * @return The state manager associated with this OGLES2RenderDevice.
	 */
	OGLES2StateManager& GetStateManager()
	{
		return m_OGLES2StateManager;
	}

	GLuint GetFrameBuffer() const
	{
		return m_CurrentRenderSurface.m_Framebuffer;
	}

	GLuint GetOnePixelWhiteTexture() const
	{
		return m_OnePixelWhiteTexture;
	}

	/**
	 * @return True if the device is in the reset state, false otherwise.
	 */
	Bool IsReset() const
	{
		// Lost is only a state on Android.
#if SEOUL_PLATFORM_ANDROID
		return (nullptr != m_Surface);
#else
		return true;
#endif
	}

#if SEOUL_PLATFORM_ANDROID
	const OGLES2RenderDeviceHardwareScalarState& GetHardwareScalarState() const
	{
		return m_HardwareScalarState;
	}
	Bool RedrawBegin();
	void RedrawBlack();
	void RedrawEnd(Bool bFinishGl = true);
	void UpdateWindow(ANativeWindow* pMainWindow);
#endif // /#if SEOUL_PLATFORM_ANDROID

	/** Wrapper around push and pop group markers, for debugging. */
	void PopGroupMarker()
	{
		if (nullptr != m_glPopGroupMarkerEXT && nullptr != m_glPushGroupMarkerEXT)
		{
			m_glPopGroupMarkerEXT();
		}
	}

	void PushGroupMarker(const String& s)
	{
		if (nullptr != m_glPopGroupMarkerEXT && nullptr != m_glPushGroupMarkerEXT)
		{
			// On some devices/platforms (iOS has this issue), the length parameter
			// appears to not follow the spec. and is expected to *include* the null
			// terminator. The best workaround we've found so far is to use
			// 0, which is supposed to tell the API to check for a null terminated
			// string, and appears to work as expected.
			m_glPushGroupMarkerEXT(0, s.CStr());
		}
	}

	class MaliLock SEOUL_SEALED
	{
	public:
		MaliLock()
		{
			// Driver bug, ARM Mali-Gxx devices, introduced in Android 10.
			// Specifically, after r16 driver (Android 9) and at or before r19 driver (Android 10).
			//
			// After blackbox testing (see https://github.com/demiurgestudios/mali_gxx_bug),
			// we've determined that we can workaround this issue by synchronizing texture upload
			// calls (glTexImage2D, glCompressedTexImage2D, etc.)
			auto& r = *OGLES2RenderDevice::Get();
			if (r.m_bMaliGxxTextureCorruptionBug)
			{
				r.m_MaliGxxTextureCorruptionBugMutex.Lock();
			}
		}

		~MaliLock()
		{
			auto& r = *OGLES2RenderDevice::Get();
			if (r.m_bMaliGxxTextureCorruptionBug)
			{
				r.m_MaliGxxTextureCorruptionBugMutex.Unlock();
			}
		}

	private:
		SEOUL_DISABLE_COPY(MaliLock);
	};

	void CompressedTexImage2D(
		GLenum target,
		GLint level,
		GLenum internalformat,
		GLsizei width,
		GLsizei height,
		GLint border,
		GLsizei imageSize,
		const GLvoid* data)
	{
		MaliLock lock;
		glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
	}

	void CompressedTexSubImage2D(
		GLenum target,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLsizei imageSize,
		const GLvoid* data)
	{
		MaliLock lock;
		glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
	}

	void TexImage2D(
		GLenum target,
		GLint level,
		GLint internalformat,
		GLsizei width,
		GLsizei height,
		GLint border,
		GLenum format,
		GLenum type,
		const GLvoid* pixels) const
	{
		MaliLock lock;
		glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
	}

	void TexSubImage2D(
		GLenum target,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		const GLvoid* pixels) const
	{
		MaliLock lock;
		glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
	}

private:
	Mutex m_MaliGxxTextureCorruptionBugMutex;
	Bool m_bMaliGxxTextureCorruptionBug = false;

	// Only accesible to OGLES2DepthStencilSurface and OGLES2RenderTarget
	friend class DepthStencilSurface;
	friend class RenderTarget;
	friend class OGLES2DepthStencilSurface;
	friend class OGLES2RenderTarget;

	// Set pSurface as the depth-stencil surface that will be used for
	// rendering. Must only be called by OGLES2DepthStencilSurface.
	void SetDepthStencilSurface(OGLES2DepthStencilSurface const* pSurface);

	// Set pTarget as the color surface that will be used for rendering.
	// Must only be called by OGLES2RenderTarget.
	void SetRenderTarget(OGLES2RenderTarget const* pTarget);

	enum class PresentResult
	{
		kSuccess,
		kFailure,
		kInterrupted,
	};

	PresentResult InternalPresent();

	void InternalGetDeviceSpecificBugs();
	void InternalGetExtensions();
	void InternalInitializeOpenGl();
	void InternalShutdownOpenGl();
	void InternalDoReset();
	void InternalDoLost();

	Viewport InternalCreateDefaultViewport();
	void InternalReportDeviceData();

	Once m_ReportOnce;

	PopGroupMarkerEXT m_glPopGroupMarkerEXT;
	PushGroupMarkerEXT m_glPushGroupMarkerEXT;

	typedef Vector< SharedPtr<BaseGraphicsObject>, MemoryBudgets::Rendering> GraphicsObjects;
	GraphicsObjects m_vGraphicsObjects;

	typedef AtomicRingBuffer<BaseGraphicsObject*> PendingGraphicsObjects;
	PendingGraphicsObjects m_PendingGraphicsObjects;

	OGLES2StateManager m_OGLES2StateManager;

	Vector< SharedPtr<OGLES2VertexFormat> > m_vpVertexFormats;
	Mutex m_VertexFormatsMutex;

	friend class OGLES2RenderCommandStreamBuilder;

#if SEOUL_PLATFORM_ANDROID
	OGLES2RenderDeviceHardwareScalarState m_HardwareScalarState;
	OGLES2WindowHandlePtr m_pMainWindow;
	OGLES2WindowHandlePtr m_pPendingMainWindow;
	EGLDisplay m_Display;
	EGLConfig m_Config;
	EGLSurface m_Surface;
	Int m_iNativeVisualID;
	Int m_iNativeVisualType;
#endif // /#if SEOUL_PLATFORM_ANDROID

#if SEOUL_PLATFORM_IOS
	GLuint m_BackBufferColorBuffer;
	void* m_pLayer;
#endif

	Viewport m_BackBufferViewport;
	RefreshRate const m_RefreshRate;

	struct Surface
	{
		Surface()
			: m_RenderTarget(0)
			, m_Depth(0)
			, m_Stencil(0)
			, m_Framebuffer(0)
		{
		}

		GLuint m_RenderTarget;
		GLuint m_Depth;
		GLuint m_Stencil;
		GLuint m_Framebuffer;
	};

	Vector2D m_vPPI;

	GLuint m_OnePixelWhiteTexture;

	Surface m_CurrentRenderSurface;
	Bool m_bCurrentRenderSurfaceIsDirty;

	Bool m_bHasFrameToPresent;
	Atomic32Value<Bool> m_bInScene;
	Atomic32Value<Bool> m_bRecalculateBackBufferViewport;
	Bool m_bSupportsES3;
	Bool m_bSupportsAsyncTextureCreate;
	Bool m_bHasContext;
	Atomic32Value<Bool> m_bPresentInterrupt;
	Atomic32Value<Bool> m_bInPresent;
	Atomic32Value<Bool> m_bInBackground;

	GLuint InternalCreateOnePixelWhiteTexture(Bool bSupportsBGRA);

	void InternalAddObject(const SharedPtr<BaseGraphicsObject>& pObject);
	Bool InternalPerFrameMaintenance();

	// Called in the destructor, loops until the object count
	// does not change or until the graphics object count is
	// 0. Returns true if the graphics object count is 0, false
	// otherwise.
	Bool InternalDestructorMaintenance();

	void InternalRecalculateBackBufferViewport();

	void InternalRenderThreadEnterBackground();
	void InternalRenderThreadLeaveBackground();

	Bool ReadBackBufferPixel(Int32 iX, Int32 iY, ColorARGBu8& rcColor);

	SEOUL_DISABLE_COPY(OGLES2RenderDevice);
}; // class OGLES2RenderDevice

/**
 * @return The global singleton reference to the current OGLES2RenderDevice.
 *
 * \pre OGLES2RenderDevice must have been created by initializing Engine.
 */
inline OGLES2RenderDevice& GetOGLES2RenderDevice()
{
	SEOUL_ASSERT(Engine::Get());
	return *OGLES2RenderDevice::Get();
}

} // namespace Seoul

#endif // include guard
