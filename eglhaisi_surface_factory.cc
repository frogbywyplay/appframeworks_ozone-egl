#include "base/files/file_path.h"
#include "base/native_library.h"
#include "ui/ozone/platform/eglhaisi/eglhaisi_surface_factory.h"
#include "ui/ozone/platform/eglhaisi/eglhaisi_vsync_provider.h"
#include "ui/ozone/public/surface_ozone_egl.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#include "ui/gfx/vsync_provider.h"



#include "ui/ozone/public/surface_ozone_canvas.h"
#include "ui/gfx/skia_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"

#include <EGL/egl.h>

#if defined(OZONE_PLATFORM_EGLHAISI_DIRECTFB)
#include <directfb.h>
#elif defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
#include <nexus_display.h>
#include <nexus_platform.h>
#include <default_nexus.h>
#include <nxclient.h>

#define EGLHAISI_WINDOW_WIDTH 1280
#define EGLHAISI_WINDOW_HEIGTH 720
#else
#error unknown backend
#endif

namespace ui {

class EglHaisiOzoneCanvas: public ui::SurfaceOzoneCanvas {
 public:
  EglHaisiOzoneCanvas();
  virtual ~EglHaisiOzoneCanvas();
  virtual void ResizeCanvas(const gfx::Size& viewport_size) override;
  virtual skia::RefPtr<SkCanvas> GetCanvas() {
    return skia::SharePtr<SkCanvas>(surface_->getCanvas());
  }
  virtual void PresentCanvas(const gfx::Rect& damage) override;
  virtual scoped_ptr<gfx::VSyncProvider> CreateVSyncProvider() override {
    return scoped_ptr<gfx::VSyncProvider>();
  }
  virtual skia::RefPtr<SkSurface> GetSurface() override {
    return surface_;
  }
 private:
  skia::RefPtr<SkSurface> surface_;
};

EglHaisiOzoneCanvas::EglHaisiOzoneCanvas() {
}
EglHaisiOzoneCanvas::~EglHaisiOzoneCanvas() {
}

void EglHaisiOzoneCanvas::ResizeCanvas(const gfx::Size& viewport_size) {
}

void EglHaisiOzoneCanvas::PresentCanvas(const gfx::Rect& damage) {
}

namespace {

#if defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
static bool joined_nexus = false;
static NXPL_PlatformHandle nxpl_handle;
#endif // OZONE_PLATFORM_EGLHAISI_NEXUS

class SurfaceOzoneEglhaisi : public SurfaceOzoneEGL {
 public:
  SurfaceOzoneEglhaisi(gfx::AcceleratedWidget window_id) {
#if defined(OZONE_PLATFORM_EGLHAISI_DIRECTFB)
    DFBResult result;
    IDirectFB *dfb = NULL;

    layer_ = NULL;
    native_window_ = NULL;

    result = DirectFBCreate(&dfb);
    if (result != DFB_OK) {
      LOG(ERROR) << "cannot create DirectFB super interface: "
          << DirectFBErrorString(result);
      goto l_exit;
    }

    result = dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer_);
    if (result != DFB_OK) {
      LOG(ERROR) << "cannot get primary layer: "
          << DirectFBErrorString(result);
      goto l_exit;
    }

    result = layer_->GetSurface(layer_, &native_window_);
    if (result != DFB_OK) {
      LOG(ERROR) << "cannot get surface of primary layer: "
          << DirectFBErrorString(result);
      goto l_exit;
    }

l_exit:
    if (result != DFB_OK) {
      if (native_window_) {
        native_window_->Release(native_window_);
        native_window_ = NULL;
      }
      if (layer_) {
        layer_->Release(layer_);
        layer_ = NULL;
      }
    }
    if (dfb) {
      dfb->Release(dfb);
    }
#elif defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
    NXPL_NativeWindowInfo window_info;

    window_info.x = window_info.y = 0;
    window_info.width = EGLHAISI_WINDOW_WIDTH;
    window_info.height = EGLHAISI_WINDOW_HEIGTH;
    window_info.stretch = true;
    window_info.clientID = 1;
    window_info.zOrder = 0;

    native_window_ = NXPL_CreateNativeWindow(&window_info);
    if (!native_window_) {
      LOG(ERROR) << "failed to create Nexus native window";
      return;
    }
#endif
  }

  virtual ~SurfaceOzoneEglhaisi() {
#if defined(OZONE_PLATFORM_EGLHAISI_DIRECTFB)
    if (native_window_) {
      native_window_->Release(native_window_);
    }
    if (layer_) {
      layer_->Release(layer_);
    }
#elif defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
    if (native_window_) {
      NXPL_DestroyNativeWindow(native_window_);
    }
#endif
  }

  virtual intptr_t GetNativeWindow()
  {
    return (intptr_t)native_window_;
  }

  virtual bool OnSwapBuffers()
  {
    return true;
  }

  virtual bool ResizeNativeWindow(const gfx::Size& viewport_size) {
#if defined(OZONE_PLATFORM_EGLHAISI_DIRECTFB)
    DFBResult result, result2;

    result = layer_->SetCooperativeLevel(layer_, DLSCL_ADMINISTRATIVE);
    if (result != DFB_OK) {
      LOG(ERROR) << "cannot change cooperative level of layer: "
          << DirectFBErrorString(result);
      return false;
    }

    result = layer_->SetScreenRectangle(layer_, 0, 0,
        viewport_size.width(), viewport_size.height());
    if (result == DFB_UNSUPPORTED) {
      LOG(WARNING) << "cannot change dimensions of layer as operation is not "
          << "supported ";
    } else if (result != DFB_OK) {
      LOG(ERROR) << "cannot change dimensions of layer: "
          << DirectFBErrorString(result);
    }

    result2 = layer_->SetCooperativeLevel(layer_, DLSCL_SHARED);
    if (result2 != DFB_OK) {
      LOG(ERROR) << "cannot restore cooperative level of layer: "
          << DirectFBErrorString(result);
    }

    return (result == DFB_OK);
#elif defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
    if (native_window_) {
      NXPL_NativeWindowInfo window_info;

      window_info.x = window_info.y = 0;
      window_info.width = viewport_size.width();
      window_info.height = viewport_size.height();
      window_info.stretch = true;
      window_info.clientID = 1;
      window_info.zOrder = 0;
      NXPL_UpdateNativeWindow(native_window_, &window_info);
    }

    return true;
#endif
  }

  virtual scoped_ptr<gfx::VSyncProvider> CreateVSyncProvider() {
    return scoped_ptr<gfx::VSyncProvider>(new EglhaisiVSyncProvider());
  }

  virtual void OnSwapBuffersAsync(const SwapCompletionCallback& callback) {
  }

 private:
#if defined(OZONE_PLATFORM_EGLHAISI_DIRECTFB)
  IDirectFBDisplayLayer *layer_;
#endif
  /* We use NativeWindowType instead of IDirectFBSurface on purpose as this
   * will raise type-cast errors if EGL does not use DirectFB surfaces. */
  NativeWindowType native_window_;
};

}  // namespace

SurfaceFactoryEglhaisi::SurfaceFactoryEglhaisi()
{
#if defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
  /* TODO: registration must be moved to the hosting application. */
  NEXUS_Error nexus_err;
  struct NxClient_JoinSettings jsets;

  NxClient_GetDefaultJoinSettings(&jsets);
  strncpy(jsets.name, "browser", sizeof(jsets.name));
  jsets.name[sizeof(jsets.name) - 1] = '\0';
  jsets.timeout = 60;
  jsets.session = 1;
  jsets.ignoreStandbyRequest = true;
  jsets.mode = NEXUS_ClientMode_eProtected;
  strncpy((char *)jsets.certificate.data, "nxclient_certificate", sizeof(jsets.certificate.data));
  jsets.certificate.data[sizeof(jsets.certificate.data) - 1] = '\0';
  jsets.certificate.length = strlen((char *)jsets.certificate.data);

  nexus_err = NxClient_Join(&jsets);
  if (nexus_err != NEXUS_SUCCESS) {
    LOG(ERROR) << "failed to join Nexus";
    return;
  }

  NXPL_RegisterNexusDisplayPlatform(&nxpl_handle, EGL_DEFAULT_DISPLAY);
#endif // OZONE_PLATFORM_EGLHAISI_NEXUS
}

SurfaceFactoryEglhaisi::~SurfaceFactoryEglhaisi()
{
#if defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
  if (nxpl_handle) {
    NXPL_UnregisterNexusDisplayPlatform(nxpl_handle);
    nxpl_handle = NULL;
  }

  if (joined_nexus) {
    NxClient_Uninit();
    joined_nexus = false;
  }
#endif // OZONE_PLATFORM_EGLHAISI_NEXUS
}

intptr_t SurfaceFactoryEglhaisi::GetNativeDisplay() {
  return (intptr_t)EGL_DEFAULT_DISPLAY;
}

scoped_ptr<SurfaceOzoneEGL> SurfaceFactoryEglhaisi::CreateEGLSurfaceForWidget(
    gfx::AcceleratedWidget widget) {
  return make_scoped_ptr<SurfaceOzoneEGL>(new SurfaceOzoneEglhaisi(widget));
}

scoped_ptr<SurfaceOzoneCanvas> SurfaceFactoryEglhaisi::CreateCanvasForWidget(
    gfx::AcceleratedWidget widget) {
  return make_scoped_ptr<SurfaceOzoneCanvas>(new EglHaisiOzoneCanvas());
}

const int32_t* SurfaceFactoryEglhaisi::GetEGLSurfaceProperties(
    const int32_t* desired_list) {
  static const EGLint kConfigAttribs[] = {
    EGL_BUFFER_SIZE, 32,
    EGL_ALPHA_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_NONE
  };

  return kConfigAttribs;
}

bool SurfaceFactoryEglhaisi::LoadEGLGLES2Bindings(
    AddGLLibraryCallback add_gl_library,
    SetGLGetProcAddressProcCallback setprocaddress) {
  base::NativeLibraryLoadError error;
#if defined(OZONE_PLATFORM_EGLHAISI_DIRECTFB)
  base::NativeLibrary gles_library = base::LoadNativeLibrary(
    base::FilePath("libGLESv2.so.2"), &error);

  if (!gles_library) {
    LOG(WARNING) << "Failed to load GLES library: " << error.ToString();
    return false;
  }

  base::NativeLibrary egl_library = base::LoadNativeLibrary(
    base::FilePath("libEGL.so.1"), &error);

  if (!egl_library) {
    LOG(WARNING) << "Failed to load EGL library: " << error.ToString();
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  GLGetProcAddressProc get_proc_address =
      reinterpret_cast<GLGetProcAddressProc>(
          base::GetFunctionPointerFromNativeLibrary(
              egl_library, "eglGetProcAddress"));

  if (!get_proc_address) {
    LOG(ERROR) << "eglGetProcAddress not found.";
    base::UnloadNativeLibrary(egl_library);
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  add_gl_library.Run(egl_library);
  add_gl_library.Run(gles_library);
#elif defined(OZONE_PLATFORM_EGLHAISI_NEXUS)
  base::NativeLibrary library = base::LoadNativeLibrary(
    base::FilePath("libv3ddriver.so"), &error);

  if (!library) {
    LOG(WARNING) << "Failed to load v3ddriver library: " << error.ToString();
    return false;
  }

  GLGetProcAddressProc get_proc_address =
      reinterpret_cast<GLGetProcAddressProc>(
          base::GetFunctionPointerFromNativeLibrary(
              library, "eglGetProcAddress"));

  if (!get_proc_address) {
    LOG(ERROR) << "eglGetProcAddress not found.";
    base::UnloadNativeLibrary(library);
    return false;
  }

  add_gl_library.Run(library);
#endif

  setprocaddress.Run(get_proc_address);
  return true;
}

}  // namespace ui
