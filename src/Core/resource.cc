
#include "resource.h"

#include "event_manager.h"

namespace cyclus {

int Resource::nextid_ = 1;

void Resource::BumpId() {
  id_ = nextid_;
  nextid_++;
}

} // namespace cyclus
