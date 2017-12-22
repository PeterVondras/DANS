#ifndef DANS02_DSTAGE_JOB_H
#define DANS02_DSTAGE_JOB_H

#include <vector>
#include "common/dstage/priority.h"

namespace duplicate_aware_scheduling {

// This is an example of a specific data type for a specific dstage. In the
// future we will either define these in a more separate place or put the
// template classes implementations all in a header file.
typedef double JData;

typedef unsigned JobId;

class JobIdFactory {
 public:
  JobIdFactory(unsigned seed);
  JobId CreateJobId();

 private:
  unsigned _last_id;
};

template <typename T>
struct Job {
  Job(T job_data, JobId job_id, Priority priority,
      unsigned requested_duplication);

  T job_data;
  JobId job_id;
  Priority priority;
  unsigned requested_duplication;
};

template <typename T>
struct JobMap {
  Job<T> job;
  std::vector<JobMap<T>**> instances;
};
}  // namespace duplicate_aware_scheduling

#endif  // DANS02_DSTAGE_JOB_H