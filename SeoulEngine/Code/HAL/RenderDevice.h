/**
 * \file RenderDevice.h
 * \brief Abstract base class for platform-specific device implementations.
 * RenderDevice and the other graphics objects in HAL provide
 * a platform-independent layer on top of platform-dependent graphics hardware
 * functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#include "Atomic32.h"
#include "ClearFlags.h"
#include "Color.h"
#include "Delegate.h"
#include "DepthStencilFormat.h"
#include "Effect.h"
#include "FilePath.h"
#include "IndexBuffer.h"
#include "IndexBufferDataFormat.h"
#include "Prereqs.h"
#include "PrimitiveType.h"
#include "Singleton.h"
#include "Texture.h"
#include "VertexElement.h"
#include "VertexBuffer.h"
#include "VertexFormat.h"
#include "Viewport.h"
namespace Seoul { class DataStoreTableUtil; }
namespace Seoul { class DepthStencilSurface; }
namespace Seoul { class RenderTarget; }
namespace Seoul { struct TextureConfig; }

namespace Seoul
{

// Forward declarations
class RenderCommandStreamBuilder;

/** Minimum width of a resolution the game will use. */
static const UInt32 kMinimumResolutionWidth = 320u;

/** Minimum height of a resolution the game will use. */
static const UInt32 kMinimumResolutionHeight = 240u;

/**
 * Tracking of optional capabilities that may or may not be supported
 * by the current graphics device.
 */
struct RenderDeviceCaps SEOUL_SEALED
{
	RenderDeviceCaps()
		: m_bBlendMinMax(false)
		, m_bBackBufferWithAlpha(false)
		, m_bBGRA(false)
		, m_bETC1(false)
		, m_bIncompleteMipChain(false)
	{
	}

	/** Supports the min/max blend equations for alpha blending. */
	Bool m_bBlendMinMax;

	/** Supports and is using a back buffer with an alpha channel. */
	Bool m_bBackBufferWithAlpha;

	/** Supports textures of the 32-bit BGRA format. */
	Bool m_bBGRA;

	/** Hardware support for textures compressed with the ETC1 format. */
	Bool m_bETC1;

	/** Hardware support for setting a max texture mip level, to support incomplete mip chains. */
	Bool m_bIncompleteMipChain;
}; // struct RenderDeviceCaps

/** Structure used to accurately represent a display refresh - default is 60 Hz. */
struct RefreshRate SEOUL_SEALED
{
	RefreshRate()
		: m_uNumerator(60000u)
		, m_uDenominator(1000u)
	{
	}

	RefreshRate(UInt32 uNumerator, UInt32 uDenominator)
		: m_uNumerator(uNumerator)
		, m_uDenominator(uDenominator)
	{
	}

	Bool IsZero() const
	{
		return (0u == m_uNumerator || 0u == m_uDenominator);
	}

	Double ToHz() const
	{
		if (IsZero())
		{
			return 0.0;
		}
		else
		{
			return ((Double)m_uNumerator) / ((Double)m_uDenominator);
		}
	}

	UInt32 m_uNumerator;
	UInt32 m_uDenominator;
}; // struct RefreshRate

enum class RenderDeviceType
{
	kD3D9,
	kD3D11Headless,
	kD3D11Window,
	kNull,
	kOGLES2,
};

class RenderDevice SEOUL_ABSTRACT : public Singleton<RenderDevice>
{
public:
	SEOUL_DELEGATE_TARGET(RenderDevice);

	/**
	 * Helper structure, contains all the parameters
	 * necessary to define the application window, graphics viewport,
	 * and various global rendering settings, such as vsync.
	 *
	 * Not all parameters in this structure are used on all
	 * platforms. This is noted in the documentation for
	 * the parameter.
	 */
	struct GraphicsParameters
	{
		GraphicsParameters()
			: m_iWindowViewportX(0)
			, m_iWindowViewportY(0)
			, m_iWindowViewportWidth(0)
			, m_iWindowViewportHeight(0)
			, m_iFullscreenWidth(-1)
			, m_iFullscreenHeight(-1)
			, m_iVsyncInterval(0)
			, m_bFullscreenOnMaximize(true)
			, m_bStartFullscreen(false)
			, m_iWindowXOffset(0)
			, m_iWindowYOffset(0)
			, m_bWindowedFullscreen(false)
#if !SEOUL_SHIP
			, m_bVirtualizedDesktop(false)
#endif // /!SEOUL_SHIP
		{
		}

		/**
		 * X position of the client rendering viewport of the total
		 * application window.
		 */
		Int m_iWindowViewportX;

		/**
		 * Y position of the client rendering viewport of the total
		 * application window.
		 */
		Int m_iWindowViewportY;

		/**
		 * Width of the client rendering viewport of the total
		 * application window.
		 */
		Int m_iWindowViewportWidth;

		/**
		 * Height of the client rendering viewport of the total
		 * application window.
		 */
		Int m_iWindowViewportHeight;

		/**
		 * Width of the rendering viewport when the game is
		 * in fullscreen.
		 */
		Int m_iFullscreenWidth;

		/**
		 * Height of the rendering viewport when the game is
		 * in fullscreen.
		 */
		Int m_iFullscreenHeight;

		/**
		 * If true, screen refresh will be synced to the vertical retrace.
		 */
		Int m_iVsyncInterval;

		/**
		 * Windows Only - if true, when the application window
		 * is maximized, the game will enter fullscreen mode.
		 */
		Bool m_bFullscreenOnMaximize;

		/**
		 * Windows Only - if true, the application should start
		 * in full screen mode.
		 */
		Bool m_bStartFullscreen;

		/**
		 * Windows Only - in absolute pixel coordinates, the X of the
		 * initial position of the upper-left corner of the application window.
		 */
		Int m_iWindowXOffset;

		/**
		 * Windows Only - in absolute pixel coordinates, the Y of the
		 * initial position of the upper-left corner of the application window.
		 */
		Int m_iWindowYOffset;

		/**
		 * Windows Only - if true, the game will run in windowed mode in
		 * full screen (using a borderless window), instead of taking exclusive ownership
		 * of the display. This mode is useful to allow fullscreen play with convenient
		 * task switching.
		 */
		Bool m_bWindowedFullscreen;

#if !SEOUL_SHIP
		/**
		 * If true and supported, all decoration will be removed from the OS
		 * window (the chrome will be removed), the window will be resized
		 * to fill the entire desktop, and it will be the responsibility of
		 * client code to use RenderCommandStreamBuilder::UpdateOsWindowRegions()
		 * to "punch through" this virtualized window in spots where there
		 * is no content to render. Effectively, this allows easy implementation
		 * of "pop out" windows, etc.
		 */
		Bool m_bVirtualizedDesktop;
#endif // /!SEOUL_SHIP
	};

	RenderDevice();
	virtual ~RenderDevice();

	virtual RenderDeviceType GetType() const = 0;

	// Return a command stream builder - must be used to accumulate render commands on threads
	// other than the render thread, and then executed on the render thread.
	virtual RenderCommandStreamBuilder* CreateRenderCommandStreamBuilder(UInt32 zInitialCapacity) const = 0;

	// Scene management
	virtual Bool BeginScene() = 0;
	virtual void EndScene() = 0;

	// Viewport control.
	virtual const Viewport& GetBackBufferViewport() const = 0;

	// Get the screen refresh rate in hertz.
	virtual RefreshRate GetDisplayRefreshRate() const = 0;

	// On supported platforms, returns the maximum rectangle that the given
	// rectangle can be resized to without overlapping OS components
	// (e.g. taskbar on Windows).
	virtual Bool GetMaximumWorkAreaForRectangle(const Rectangle2DInt& input, Rectangle2DInt& output) const
	{
		return false;
	}
	virtual Bool GetMaximumWorkAreaOnPrimary(Rectangle2DInt& output) const
	{
		return false;
	}

	/**
	 * @return A shadow post projection transform, used to remap clip space to texcoord
	 * lookups. Dependent on whether the current platform uses a half-pixel offset
	 * or not.
	 */
	virtual Matrix4D GetShadowPostProjectionTransform(const Vector2D& vShadowTextureDimensions) const
	{
		// Apply a half pixel offset by defualt. Assume texture origin is the upper left.
		const Matrix4D kShadowPostLightProjectionTransform(
			0.5f,  0.0f, 0.0f, 0.5f + (0.5f * (1.0f / vShadowTextureDimensions.X)),
			0.0f, -0.5f, 0.0f, 0.5f + (0.5f * (1.0f / vShadowTextureDimensions.Y)),
			0.0f,  0.0f, 1.0f, 0.0f,
			0.0f,  0.0f, 0.0f, 1.0f);

		return kShadowPostLightProjectionTransform;
	}

	// Vertex formats

	virtual SharedPtr<VertexFormat> CreateVertexFormat(VertexElement const* pElements) = 0;

	// Surfaces
	virtual SharedPtr<DepthStencilSurface> CreateDepthStencilSurface(const DataStoreTableUtil& configSettings) = 0;
	virtual SharedPtr<RenderTarget> CreateRenderTarget(const DataStoreTableUtil& configSettings) = 0;

	// Index and Vertex buffers
	virtual SharedPtr<IndexBuffer> CreateIndexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat) = 0;

	// On platforms on which the distinction matters, this creates a buffer
	// suitable for frequent per-frame updates.
	virtual SharedPtr<IndexBuffer> CreateDynamicIndexBuffer(
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat) = 0;

	virtual SharedPtr<VertexBuffer> CreateVertexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		UInt32 zStrideInBytes) = 0;

	// On platforms on which the distinction matters, this creates a buffer
	// suitable for frequent per-frame updates.
	virtual SharedPtr<VertexBuffer> CreateDynamicVertexBuffer(
		UInt32 zTotalSizeInBytes,
		UInt32 zStrideInBytes) = 0;

	// Textures

	/**
	 * @return True if the current render device supports immediate texture creation
	 * off the render thread, false otherwise.
	 */
	virtual Bool SupportsAsyncCreateTexture() const
	{
		return false;
	}

	/**
	 * Immediate asynchronous texture create - always returns a nullptr
	 * result if not supported.
	 *
	 * "Async" here may be confusing - this is asynchronous with regards to
	 * the render thread. Most RenderDevice API must be called on the render thread,
	 * and object creation occurs sequentially on that thread. Async* API supports
	 * instantaneous creation of graphics object on other threads.
	 */
	virtual SharedPtr<BaseTexture> AsyncCreateTexture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat)
	{
		return SharedPtr<BaseTexture>();
	}

	// If specified, pBaseImageData and pMipImageData will be used to populate the texture.
	// These must obey the following:
	// - pBaseImageData specifies mip 0
	// - pMipImageData size must be equal to nLevels - 1
	// - data in pBaseImageData and pMipImageData must be tightly packed according to
	//   eFormat (pitch must be equal to (width * bytes_per_pixel(eFormat))
	virtual SharedPtr<BaseTexture> CreateTexture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat) = 0;

	// Effects

	/**
	 * @return True if the device will take ownership of the file
	 * data passed into CreateEffectFromFileInMemory(), false otherwise.
	 * If this method returns false, the caller is responsible for
	 * destroying the file data after the call, otherwise it will be
	 * destroyed by the RenderDevice.
	 */
	virtual Bool TakesOwnershipOfEffectFileData() const
	{
		return true;
	}

	virtual SharedPtr<Effect> CreateEffectFromFileInMemory(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes) = 0;

	/**
	 * Get the list of resolution names available to be selected. The indices of these are
	 * meaningful for SetRenderModeByIndex.
	 */
	const Vector<String>& GetAvailableRenderModeNames() const { return m_vAvailableRenderModeNames; }

	/**
	 * @return The index of the currently active render mode, or -1 if no render mode is currently set.
	 */
	virtual Int GetActiveRenderModeIndex() const { return -1; }

	/*
	 * Reset the rendering mode based on an index (selected by the user, probably, as a render mode name)
	 *
	 * @return True if the mode was set to a new mode, false if the given index is not
	 * a valid render mode index.
	 */
	virtual Bool SetRenderModeByIndex(Int nRenderMode) { return false; }

	/**
	 * Returns if the render window is active or not.
	 */
	virtual Bool IsActive() const { return true; }

	/**
	 * @return True if the renderer is currently in a maximized window, false otherwise.
	 *
	 * Not all platforms support a "windowed" mode.
	 */
	virtual Bool IsMaximized() const
	{
		return false;
	}

	/**
	 * @return True if the renderer is currently in a minimized window, false otherwise.
	 *
	 * Not all platforms support a "windowed" mode.
	 */
	virtual Bool IsMinimized() const
	{
		return false;
	}

	/** If supported, bring the hardware window into the foreground of other windows. */
	virtual Bool ForegroundOsWindow()
	{
		return false;
	}

	/**
	 * @return True if the renderer is currently in windowed mode, false otherwise.
	 *
	 * Not all platforms support a "windowed" mode.
	 */
	virtual Bool IsWindowed() const
	{
		return false;
	}

	// Toggles the current display between full screen and windowed mode,
	// if supported on the current platform.
	virtual void ToggleFullscreenMode()
	{}

	// On supported platform, toggle maximization of the main viewport window.
	virtual void ToggleMaximized()
	{}

	// On supported platform, toggle minimization of the main viewport window.
	virtual void ToggleMinimized()
	{}

	/**
	 * @return The current vertical sync interval.
	 *
	 * An interval of 0 disables vertical sync. Any value > 0
	 * attempts to sync to that multiple of the display's refresh
	 * interval (e.g. a value of 1 on a 60 hz display will synchronize
	 * at 1/60 of a second, a value of 2 will synchronize at 1/30
	 * of a second, etc.).
	 *
	 * \remarks Not all backends support all vertical sync intervals or
	 * changing of vertical sync intervals on the fly. This function
	 * is expected to always return the *current* sync interval, while
	 * GetDesiredVsyncInterval() will return the requested interval,
	 * which may or may not ever become the actual interval.
	 */
	Int GetVsyncInterval() const
	{
		return m_GraphicsParameters.m_iVsyncInterval;
	}
	Int GetDesiredVsyncInterval() const
	{
		return m_iDesiredVsyncInterval;
	}

	/**
	 * Attempt to update the vsync interval - sets the value
	 * of desired vsync interval which may or may not ever become
	 * the actual vsync interval, depending on backend.
	 */
	virtual void SetDesiredVsyncInterval(Int iInterval)
	{
		m_iDesiredVsyncInterval = iInterval;
	}

	// For some platforms, implements specific handling in the render system
	// on enter/exit background (on mobile devices, when the app becomes inactive,
	// it has entered the background).
	virtual void OnEnterBackground()
	{
	}

	virtual void OnLeaveBackground()
	{
	}

	/** @return The optional render capabilities supported by the current graphics device. */
	const RenderDeviceCaps& GetCaps() const
	{
		return m_Caps;
	}

	/**
	 * @return The depth-stencil format of the back buffer.
	 */
	DepthStencilFormat GetBackBufferDepthStencilFormat() const
	{
		return m_eBackBufferDepthStencilFormat;
	}

	/**
	 * @return The pixel format of the back buffer.
	 */
	PixelFormat GetBackBufferPixelFormat() const
	{
		return m_eBackBufferPixelFormat;
	}

	/**
	 * @return A 4-channel 32-bit compatible color format for this platform.
	 */
	PixelFormat GetCompatible32bit4colorRenderTargetFormat() const
	{
		return m_eCompatible32bit4colorRenderTargetFormat;
	}

	// Not supported on all platforms. Nop if app is not in a windowed mode.
	virtual Bool GetOsWindowRegion(Point2DInt& rPos, Point2DInt& rSize) const { return false; }
	virtual void SetOsWindowRegion(const Point2DInt& pos, const Point2DInt& size) {}

	// Not supported on all platforms. When supported, returns the human readable
	// title string of the application's main window.
	virtual const String& GetOsWindowTitle() const;

	Int64 GetPresentDeltaInTicks() const { return m_iPresentDeltaInTicks; }

	// Virtualized desktop is a developer only feature.
#if !SEOUL_SHIP
	/**
	 * Valid only if IsVirtualizedDesktop() is true. Returns the main monitor
	 * relative coordinates of the virtualized desktop. This can be used
	 * to (e.g.) adjust internal game render coordinates after a virtualization
	 * toggle to avoid growing/shifting content.
	 */
	virtual Rectangle2DInt GetVirtualizedDesktopRect() const
	{
		return Rectangle2DInt();
	}

	/**
	 * Beyond the request, a platform must support a virtualized desktop. By default,
	 * it is not supported.
	 */
	virtual Bool IsVirtualizedDesktop() const
	{
		return false;
	}

	/**
	 * Request virtualized desktop for devices that support the mode.
	 */
	virtual void SetVirtualizedDesktop(Bool bVirtualized)
	{
	}

	/** Whether a virtualized desktop is supported or not. */
	virtual Bool SupportsVirtualizedDesktop() const
	{
		return false;
	}
#endif // /!SEOUL_SHIP

protected:
	// Shared utility functions, should be implemented in most graphics drivers. Used to
	// avoid stalling in the driver during vsync when there is useful work the render thread could be doing.
	void InternalPrePresent();
	void InternalPostPresent();

	RenderDeviceCaps m_Caps;
	Vector<String> m_vAvailableRenderModeNames;

	GraphicsParameters m_GraphicsParameters;
	DepthStencilFormat m_eBackBufferDepthStencilFormat;
	PixelFormat m_eBackBufferPixelFormat;
	PixelFormat m_eCompatible32bit4colorRenderTargetFormat;
	Atomic32Value<Int32> m_iDesiredVsyncInterval;

	Int64 GetPresentMarkerInTicks() const { return m_iPresentMarkerInTicks; }

private:
	void InternalInitializeGraphicsParameters();

	Int64 m_iPresentMarkerInTicks;
	Int64 m_iPresentDeltaInTicks;
	friend class RenderDeviceScopedWait;
	Atomic32 m_WaitHint;
	String const m_sPlaceholderEmptyTitle;

	SEOUL_DISABLE_COPY(RenderDevice);
}; // class RenderDevice

/**
 * Used for separate render threads. Normally, the render device
 * will spend time in present processing to process render thread
 * work. The wait hint lock can be used to signal to the render
 * thread that another thread (typically the game/main thread)
 * is waiting for the render thread to finish, so it should
 * complete processing as quickly as possible.
 */
class RenderDeviceScopedWait SEOUL_SEALED
{
public:
	RenderDeviceScopedWait()
	{
		++RenderDevice::Get()->m_WaitHint;
	}

	~RenderDeviceScopedWait()
	{
		--RenderDevice::Get()->m_WaitHint;
	}

private:
	SEOUL_DISABLE_COPY(RenderDeviceScopedWait);
}; // class RenderDeviceScopedWait

} // namespace Seoul

#endif // include guard
