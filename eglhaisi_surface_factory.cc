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

#include <directfb.h>
#include <EGL/egl.h>

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

class SurfaceOzoneEglhaisi : public SurfaceOzoneEGL {
 public:
  SurfaceOzoneEglhaisi(gfx::AcceleratedWidget window_id)
      : layer_(NULL),
        surface_(NULL) {
    DFBResult result;
    IDirectFB *dfb = NULL;
    IDirectFBDisplayLayer *layer = NULL;
    IDirectFBSurface *surface = NULL;

    result = DirectFBCreate(&dfb);
    if (result != DFB_OK) {
      LOG(ERROR) << "cannot create DirectFB super interface: "
          << DirectFBErrorString(result);
      goto l_exit;
    }

    result = dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer);
    if (result != DFB_OK) {
      LOG(ERROR) << "cannot get primary layer: "
          << DirectFBErrorString(result);
      goto l_exit;
    }

    result = layer->GetSurface(layer, &surface);
    if (result != DFB_OK) {
      LOG(ERROR) << "cannot get surface of primary layer: "
          << DirectFBErrorString(result);
      goto l_exit;
    }

l_exit:
    if (result == DFB_OK) {
      layer_ = layer;
      surface_ = surface;
    } else {
      if (surface) {
        surface->Release(surface);
      }
      if (layer) {
        layer->Release(layer);
      }
    }
    if (dfb) {
      dfb->Release(dfb);
    }
  }

  virtual ~SurfaceOzoneEglhaisi() {
    if (surface_) {
      surface_->Release(surface_);
    }
    if (layer_) {
      layer_->Release(layer_);
    }
  }

  virtual intptr_t GetNativeWindow()
  {
    return (intptr_t)surface_;
  }

  virtual bool OnSwapBuffers()
  {
    return true;
  }

  virtual bool ResizeNativeWindow(const gfx::Size& viewport_size) {
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
  }

  virtual scoped_ptr<gfx::VSyncProvider> CreateVSyncProvider() {
    return scoped_ptr<gfx::VSyncProvider>(new EglhaisiVSyncProvider());
  }

  virtual void OnSwapBuffersAsync(const SwapCompletionCallback& callback) {
  }

 private:
  IDirectFBDisplayLayer *layer_;
  IDirectFBSurface *surface_;
};

}  // namespace

SurfaceFactoryEglhaisi::SurfaceFactoryEglhaisi()
{
}
SurfaceFactoryEglhaisi::~SurfaceFactoryEglhaisi()
{
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

  setprocaddress.Run(get_proc_address);
  add_gl_library.Run(egl_library);
  add_gl_library.Run(gles_library);
  return true;
}

}  // namespace ui
