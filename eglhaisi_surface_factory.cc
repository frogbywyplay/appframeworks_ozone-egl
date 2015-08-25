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
#include "EGL/fbdev_window.h"
#include <sys/ioctl.h>
#include <fcntl.h>

#define EGLHAISI_WINDOW_WIDTH 1280
#define EGLHAISI_WINDOW_HEIGTH 720

namespace ui {


/////////////////////////////////////////////////////////////


class EglHaisiOzoneCanvas: public ui::SurfaceOzoneCanvas {
 public:
  EglHaisiOzoneCanvas();
  virtual ~EglHaisiOzoneCanvas();
  virtual void ResizeCanvas(const gfx::Size& viewport_size) override;
  //virtual skia::RefPtr<SkCanvas> GetCanvas() override {
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
  //ozone_egl_UserData userDate_;
};

EglHaisiOzoneCanvas::EglHaisiOzoneCanvas()
{
//    memset(&userDate_,0,sizeof(userDate_));
}
EglHaisiOzoneCanvas::~EglHaisiOzoneCanvas()
{
//    ozone_egl_textureShutDown (&userDate_);
}

void EglHaisiOzoneCanvas::ResizeCanvas(const gfx::Size& viewport_size)
{
  /*if(userDate_.width == viewport_size.width() && userDate_.height==viewport_size.height())
  {
      return;
  }
  else if(userDate_.width != 0 && userDate_.height !=0)
  {
      ozone_egl_textureShutDown (&userDate_);
  }
  surface_ = skia::AdoptRef(SkSurface::NewRaster(
        SkImageInfo::Make(viewport_size.width(),
                                   viewport_size.height(),
                                   kPMColor_SkColorType,
                                   kPremul_SkAlphaType)));
  userDate_.width = viewport_size.width();
  userDate_.height = viewport_size.height();
  userDate_.colorType = GL_BGRA_EXT;
  ozone_egl_textureInit ( &userDate_);
  */
}

void EglHaisiOzoneCanvas::PresentCanvas(const gfx::Rect& damage)
{
    /*SkImageInfo info;
    size_t row_bytes;
    userDate_.data = (char *) surface_->peekPixels(&info, &row_bytes);
    ozone_egl_textureDraw(&userDate_);
    ozone_egl_swap();*/
}


/////////////////////////////////////////////////////
//
namespace {

class SurfaceOzoneEglhaisi : public SurfaceOzoneEGL {
 public:
  SurfaceOzoneEglhaisi(gfx::AcceleratedWidget window_id){
    native_window_ = (fbdev_window *) malloc( sizeof(fbdev_window));
    if (NULL != native_window_)
    {
        native_window_->width  = EGLHAISI_WINDOW_WIDTH;
        native_window_->height = EGLHAISI_WINDOW_HEIGTH;
    }
  }
  virtual ~SurfaceOzoneEglhaisi() {
    if(native_window_ != NULL)
    {
       free(native_window_);
       native_window_ = NULL;
    }
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
    if(native_window_ != NULL)
    {
       free(native_window_);
       native_window_ = NULL;
    }
    native_window_ = (fbdev_window *) malloc( sizeof(fbdev_window));
    if (NULL != native_window_)
    {
     
        native_window_->width  = viewport_size.width();
        native_window_->height = viewport_size.height();
    }
    return true;
  }

  virtual scoped_ptr<gfx::VSyncProvider> CreateVSyncProvider() {
    return scoped_ptr<gfx::VSyncProvider>(new EglhaisiVSyncProvider());
  }

  virtual bool OnSwapBuffersAsync(const SwapCompletionCallback& callback) {
    return false;
  }

 private:
  fbdev_window * native_window_;
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
  //return (intptr_t)((EGLNativeDisplayType)1);
}

scoped_ptr<SurfaceOzoneEGL> SurfaceFactoryEglhaisi::CreateEGLSurfaceForWidget(gfx::AcceleratedWidget widget) {
  return make_scoped_ptr<SurfaceOzoneEGL>(new SurfaceOzoneEglhaisi(widget));
}

scoped_ptr<SurfaceOzoneCanvas> SurfaceFactoryEglhaisi::CreateCanvasForWidget(gfx::AcceleratedWidget widget) {
  //scoped_ptr<EglOzoneCanvas> canvas(new EglOzoneCanvas());
  //return canvas.PassAs<ui::SurfaceOzoneCanvas>();   
  return make_scoped_ptr<SurfaceOzoneCanvas>(new EglHaisiOzoneCanvas());
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

const int32* SurfaceFactoryEglhaisi::GetEGLSurfaceProperties(
    const int32* desired_list) {
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

}  // namespace ui
