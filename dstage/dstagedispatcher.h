#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <memory>

#include "dstage/applicationrequest.h"
#include "dstage/jobid"
#include "dstage/jobmap"
#include "dstage/priority.h"
#include "dstage/pcqueue"
#include "util/status.h"

namespace duplicate_aware_scheduling {
class DStageDispatcher {
public:
  // Introduces an ApplicationRequest to a DStage. base_prio is the incoming
  // Priority of the ApplicationRequest. The Dispatcher will make
  // duplication_level duplicates of the request and set job_id for referencing
  // this job in the future.
  virtual Status Dispatch(unique_ptr<ApplicationRequest> app_req,
                          Priority incoming_priority,
                          uint duplication_level);

  // Purge will remove all instances of the Job linked to job_id that
  // have not already been scheduled. Returns OK on success.
  virtual Status Purge(JobId job_id);

  // Returns the highest priority job. This is a thread safe function.
  // GetNextJob() will block indefinitely while no Job exists.
  virtual unique_ptr<ApplicationRequest> GetNextJob();

};
} // namespace duplicate_aware_scheduling

#endif // DISPATCHER_H