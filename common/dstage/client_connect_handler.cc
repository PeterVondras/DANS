#include <memory>
#include <string>

#include "common/dstage/client_connect_handler.h"
#include "glog/logging.h"

namespace {
using CallBack2 = dans::CommunicationHandlerInterface::CallBack2;
using ReadyFor = dans::CommunicationHandlerInterface::ReadyFor;

const std::string kLowPrioPort = "5010";
const std::string kHighPrioPort = "5011";
}  // namespace

namespace dans {

ConnectDispatcher::ConnectDispatcher(Priority max_priority)
    : Dispatcher<ConnectData, ConnectDataInternal>(max_priority) {}

void ConnectDispatcher::DuplicateAndEnqueue(UniqJobPtr<ConnectData> job_in,
                                            Priority max_prio,
                                            unsigned duplication) {
  VLOG(4) << __PRETTY_FUNCTION__ << " max_prio=" << max_prio
          << ", duplication= " << duplication;
  CHECK(job_in->job_data.ip_addresses.size() > duplication)
      << "There are " << job_in->job_data.ip_addresses.size()
      << " ip_addresses, but we need at least " << duplication + 1;

  Priority prio;
  int i;
  CHECK_EQ(job_in->job_data.ip_addresses.size(), _max_priority + 1);
  auto purge_state = std::make_shared<PurgeState>();
  for (i = 0, prio = job_in->priority; prio <= max_prio; i++, prio++) {
    VLOG(3) << "duplicate_job=" << i << ", prio=" << prio;
    ConnectDataInternal req_data_internal = {
        job_in->job_data.ip_addresses[i], job_in->job_data.object_id,
        job_in->job_data.done, purge_state};
    auto duplicate_job_p = std::make_unique<Job<ConnectDataInternal>>(
        req_data_internal, job_in->job_id, prio, duplication);
    _multi_q_p->Enqueue(std::move(duplicate_job_p));
  }
}

ConnectScheduler::ConnectScheduler(
    std::vector<unsigned> threads_per_prio, bool set_thread_priority,
    CommunicationHandlerInterface* comm_interface,
    BaseDStage<RequestData>* request_dstage)
    : Scheduler<ConnectDataInternal>(threads_per_prio, set_thread_priority),
      _comm_interface(comm_interface),
      _request_dstage(request_dstage),
      _destructing(false) {
  VLOG(4) << __PRETTY_FUNCTION__;
}

ConnectScheduler::~ConnectScheduler() {
  VLOG(4) << __PRETTY_FUNCTION__;
  {
    std::unique_lock<std::shared_timed_mutex> lock(_destructing_lock);
    _destructing = true;
  }

  // Releasing threads from blocking MultiQueue call.
  if (_running) {
    _multi_q_p->ReleaseQueues();
  }
}

unsigned ConnectScheduler::Purge(JobId job_id) {
  VLOG(4) << __PRETTY_FUNCTION__ << " job_id=" << job_id;
  return _request_dstage->Purge(job_id);
}

void ConnectScheduler::StartScheduling(Priority prio) {
  VLOG(4) << __PRETTY_FUNCTION__ << " prio=" << prio;
  while (true) {
    {
      std::shared_lock<std::shared_timed_mutex> lock(_destructing_lock);
      if (_destructing) return;
    }

    // Convert the UniqJobPtr to SharedJobPtr to allow capture in
    // closure. Note that uniq_ptr's are are hard/impossible to capture using
    // std::bind as they do not have a copy constructor.
    SharedJobPtr<ConnectDataInternal> job = _multi_q_p->Dequeue(prio);
    if (job == nullptr) continue;
    VLOG(1) << "Connect Handler Scheduler got job_id=" << job->job_id
            << ", ip=" << job->job_data.ip;

    // Check if job has been purged
    if (job->job_data.purge_state->IsPurged()) {
      VLOG(2) << "Purged job_id=" << job->job_id
              << ", Priority=" << job->priority;
      continue;
    }

    CallBack2 connected(std::bind(&dans::ConnectScheduler::ConnectCallback,
                                  this, job, std::placeholders::_1,
                                  std::placeholders::_2));

    _comm_interface->Connect(job->job_data.ip,
                             job->priority == 0 ? kHighPrioPort : kLowPrioPort,
                             connected);
  }
}

void ConnectScheduler::ConnectCallback(
    SharedJobPtr<ConnectDataInternal> old_job, int soc, ReadyFor ready_for) {
  VLOG(4) << __PRETTY_FUNCTION__ << " soc=" << soc;
  if (ready_for.err != 0) {
    PLOG(WARNING) << "Failed to create TCP connection for jobid="
                  << old_job->job_id << " prio=" << old_job->priority
                  << " socket=" << soc;
    return;
  }
  CHECK(ready_for.out);

  auto connection = std::make_shared<Connection>(soc);
  // Pass on job if it is not complete.
  if (!old_job->job_data.purge_state->IsPurged() && soc >= 0) {
    auto request_job = std::make_unique<Job<RequestData>>(
        RequestData{std::move(connection), old_job->job_data.object_id,
                    old_job->job_data.done, old_job->job_data.purge_state},
        old_job->job_id, old_job->priority, old_job->duplication);
    _request_dstage->Dispatch(std::move(request_job),
                              /*requested_dulpication=*/0);
  } else if (soc < 0) {
    errno = -soc;
    PLOG(WARNING) << "Dropped socket for job_id=" << old_job->job_id;
  } else {
    VLOG(2) << "Purged job_id=" << old_job->job_id
            << ", Priority=" << old_job->priority;
  }
}

}  // namespace dans
