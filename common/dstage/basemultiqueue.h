#ifndef DANS02_DSTAGE_BASE_MULTIQUEUE_H
#define DANS02_DSTAGE_BASE_MULTIQUEUE_H

#include <list>
#include <memory>

#include "common/dstage/job.h"

namespace dans {

template <typename T>
class BaseMultiQueue {
 public:
  virtual ~BaseMultiQueue(){};

  // Adds a job_id to all priority queues referenced in prio_list.
  // This function is thread safe.
  virtual void Enqueue(UniqJobPtr<T> job_p) = 0;

  // Thread safe and blocking dequeue function will dequeue from the queue
  // associated to "prio."
  virtual UniqJobPtr<T> Dequeue(Priority prio) = 0;

  virtual unsigned Purge(JobId job_id) = 0;

  // Returns the size of the queue related to Priority prio. There is no
  // guarantee that this value is valid even at the time of the return.
  virtual unsigned Size(Priority prio) = 0;

  virtual bool Empty(Priority prio) = 0;

  // All q's release all threads calling Dequeue by continuously returning
  // nullptr's.
  virtual void ReleaseQueues() = 0;

  // Returns a nullptr to the next in line for the Priority `prio` queue.
  virtual void ReleaseOne(Priority prio) = 0;

  virtual unsigned MaxPriority() = 0;
};

}  // namespace dans

#endif  // DANS02_DSTAGE_BASE_MULTIQUEUE_H
