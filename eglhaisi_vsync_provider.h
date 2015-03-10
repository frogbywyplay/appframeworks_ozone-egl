#ifndef UI_OZONE_PLATFORM_IMPL_DRI_VSYNC_PROVIDER_H_
#define UI_OZONE_PLATFORM_IMPL_DRI_VSYNC_PROVIDER_H_

#include "base/memory/weak_ptr.h"
#include "ui/gfx/vsync_provider.h"

namespace ui {

class EglhaisiVSyncProvider : public gfx::VSyncProvider {
 public:
  EglhaisiVSyncProvider();
  virtual ~EglhaisiVSyncProvider();

  virtual void GetVSyncParameters(const UpdateVSyncCallback& callback);

 private:

  DISALLOW_COPY_AND_ASSIGN(EglhaisiVSyncProvider);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_IMPL_EGLHAISI_VSYNC_PROVIDER_H_
