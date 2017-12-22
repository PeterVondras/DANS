#include "common/dstage/duplicatestage.h"

#include <vector>

#include "common/dstage/dstage.h"
#include "common/dstage/job.h"

namespace duplicate_aware_scheduling {

template <typename T>
DuplicateStage<T>::DuplicateStage(unsigned max_duplication_level,
                                  std::unique_ptr<Dispatcher<T>> dispatcher)
    : _max_duplication_level(max_duplication_level),
      _dispatcher(dispatcher),
      _multi_q(_max_duplication_level) {
  _dispatcher.LinkMultiQ(&_multi_q);
}

// As long as template implementation is in .cpp file, must explicitly tell
// compiler which types to compile...
template class Dispatcher<JData>;

}  // namespace duplicate_aware_scheduling