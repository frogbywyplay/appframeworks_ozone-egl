#include "ui/ozone/platform/eglhaisi/eglhaisi_vsync_provider.h"
#include "base/time/time.h"

namespace ui {

EglhaisiVSyncProvider::EglhaisiVSyncProvider() {
}

EglhaisiVSyncProvider::~EglhaisiVSyncProvider() {}

void EglhaisiVSyncProvider::GetVSyncParameters(const UpdateVSyncCallback& callback) {
  base::TimeTicks timebase = base::TimeTicks::Now();
  base::TimeDelta interval = base::TimeDelta::FromMilliseconds(40);

  callback.Run(timebase, interval);
}

}  // namespace ui
