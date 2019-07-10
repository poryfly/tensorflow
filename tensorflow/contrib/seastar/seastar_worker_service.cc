#include "tensorflow/contrib/seastar/seastar_worker_service.h"

#include <memory>

#include "tensorflow/contrib/seastar/seastar_server_tag.h"
#include "tensorflow/contrib/seastar/seastar_tag_factory.h"
#include "tensorflow/core/common_runtime/device_factory.h"
#include "tensorflow/core/distributed_runtime/rendezvous_mgr_interface.h"
#include "tensorflow/core/framework/rendezvous.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/monitoring/cat_reporter.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/public/session_options.h"

namespace tensorflow {

namespace {

template<class RequestMessage, class ResponseMessage>

class SeastarCall {
 public:
  RequestMessage req_;
  ResponseMessage resp_;
};

}  // namespace

using HandleRequestFunction = void (SeastarWorkerService::*)(SeastarServerTag*);

SeastarWorkerService::SeastarWorkerService(SeastarWorker* worker)
    : worker_(worker) {
  handler_map_[SeastarWorkerServiceMethod::kRunGraph] =
      &SeastarWorkerService::RunGraphHandler;
  handler_map_[SeastarWorkerServiceMethod::kRecvTensor] =
      &SeastarWorkerService::RecvTensorHandlerRaw;
  handler_map_[SeastarWorkerServiceMethod::kGetStatus] =
      &SeastarWorkerService::GetStatusHandler;
  handler_map_[SeastarWorkerServiceMethod::kCreateWorkerSession] =
      &SeastarWorkerService::CreateWorkerSessionHandler;
  handler_map_[SeastarWorkerServiceMethod::kDeleteWorkerSession] =
      &SeastarWorkerService::DeleteWorkerSessionHandler;
  handler_map_[SeastarWorkerServiceMethod::kRegisterGraph] =
      &SeastarWorkerService::RegisterGraphHandler;
  handler_map_[SeastarWorkerServiceMethod::kDeregisterGraph] =
      &SeastarWorkerService::DeregisterGraphHandler;
  handler_map_[SeastarWorkerServiceMethod::kCleanupGraph] =
      &SeastarWorkerService::CleanupGraphHandler;
  handler_map_[SeastarWorkerServiceMethod::kCleanupAll] =
      &SeastarWorkerService::CleanupAllHandler;
  handler_map_[SeastarWorkerServiceMethod::kLogging] =
      &SeastarWorkerService::LoggingHandler;
  handler_map_[SeastarWorkerServiceMethod::kTracing] =
      &SeastarWorkerService::TracingHandler;
  handler_map_[SeastarWorkerServiceMethod::kRecvBuf] =
      &SeastarWorkerService::RecvBufHandler;
  handler_map_[SeastarWorkerServiceMethod::kCompleteGroup] =
      &SeastarWorkerService::CompleteGroupHandler;
  handler_map_[SeastarWorkerServiceMethod::kCompleteInstance] =
      &SeastarWorkerService::CompleteInstanceHandler;
  handler_map_[SeastarWorkerServiceMethod::kGetStepSequence] =
      &SeastarWorkerService::GetStepSequenceHandler;
  handler_map_[SeastarWorkerServiceMethod::kFuseRecvTensor] =
      &SeastarWorkerService::FuseRecvTensorHandlerRaw;
}

HandleRequestFunction SeastarWorkerService::GetHandler(
    SeastarWorkerServiceMethod methodId) {
  return handler_map_[methodId];
}

void SeastarWorkerService::RunGraphHandler(SeastarServerTag* tag) {
  int64 delta_micros = 0;
  if (CAT_LOG_IS_ON(1)) {
    delta_micros = Env::Default()->NowMicros();
  }
  Schedule([this, tag, delta_micros]() {
    auto* call = new SeastarCall<RunGraphRequest, RunGraphResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    auto* call_opts = new CallOptions;
    call_opts->SetDeltaMicros(delta_micros);
    auto* wrapped_request = new ProtoRunGraphRequest(&call->req_);
    auto* wrapped_response = new NonOwnedProtoRunGraphResponse(&call->resp_);

    worker_->RunGraphAsync(call_opts, wrapped_request, wrapped_response,
                           [tag, call, call_opts, wrapped_request,
                               wrapped_response](const Status& s) {
                             tag->ProcessDone(s);
                             delete call_opts;
                             delete wrapped_request;
                             delete wrapped_response;
                             delete call;
                           });
  });
}

void SeastarWorkerService::GetStatusHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<GetStatusRequest, GetStatusResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->GetStatus(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::CreateWorkerSessionHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<CreateWorkerSessionRequest,
                                 CreateWorkerSessionResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->CreateWorkerSession(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::DeleteWorkerSessionHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<DeleteWorkerSessionRequest,
                                 DeleteWorkerSessionResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->DeleteWorkerSession(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::CleanupAllHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<CleanupAllRequest, CleanupAllResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->CleanupAll(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::RegisterGraphHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<RegisterGraphRequest, RegisterGraphResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->RegisterGraph(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::DeregisterGraphHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<DeregisterGraphRequest, DeregisterGraphResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->DeregisterGraph(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::CleanupGraphHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<CleanupGraphRequest, CleanupGraphResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->CleanupGraph(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::LoggingHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<LoggingRequest, LoggingResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);

    Status s = worker_->Logging(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::TracingHandler(SeastarServerTag* tag) {
  Schedule([this, tag]() {
    auto* call = new SeastarCall<TracingRequest, TracingResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag);
    Status s = worker_->Tracing(&call->req_, &call->resp_);
    tag->ProcessDone(s);
    delete call;
  });
}

void SeastarWorkerService::RecvBufHandler(SeastarServerTag* tag) {
  tag->ProcessDone(
      errors::Unimplemented("SeastarWorkerService::RecvBufHandler()"));
}

void SeastarWorkerService::CompleteGroupHandler(SeastarServerTag* tag) {
  tag->ProcessDone(
      errors::Unimplemented("SeastarWorkerService::CompleteGroupHandler()"));
}

void SeastarWorkerService::CompleteInstanceHandler(SeastarServerTag* tag) {
  tag->ProcessDone(
      errors::Unimplemented("SeastarWorkerService::CompleteInstanceHandler()"));
}

void SeastarWorkerService::GetStepSequenceHandler(SeastarServerTag* tag) {
  tag->ProcessDone(
      errors::Unimplemented("SeastarWorkerService::GetStepSequenceHandler()"));
}

void SeastarWorkerService::FuseRecvTensorHandlerRaw(SeastarServerTag *tag) {
  int64 handle_start_micros = 0;
  if (CAT_LOG_IS_ON(3)) {
    handle_start_micros = Env::Default()->NowMicros();
  }
  Schedule([this, tag, handle_start_micros]() {
    auto *call_opts = new CallOptions;
    auto *call =new SeastarCall<FuseRecvTensorRequest, SeastarFuseTensorResponse>();
    InitSeastarServerTag(&call->req_,
                         &call->resp_,
                         tag,
                         [call](const Status &s) { delete call; });

    int64 delta_micros = 0;
    if (CAT_LOG_IS_ON(3)) {
      // delta_micros records the timestamp's different between client/server
      // we assumed network communication takes about 50us (statistic from ping)
      delta_micros = handle_start_micros - call->req_.recv_req_start_micros() - 50;
      CAT_LOG(3)::logDuration(CAT_REPORTER::seastar_time_trace,
                              "FuseRecvReqStartToRpcHandle", delta_micros);
      call_opts->SetDeltaMicros(delta_micros);
    }

    worker_->FuseRecvTensorAsync(call_opts, &call->req_, &call->resp_,
                                 [tag, call, call_opts](const Status &s) {
                                   delete call_opts;
                                   tag->ProcessDone(s);
                                 });
  });
}

void SeastarWorkerService::RecvTensorHandlerRaw(SeastarServerTag* tag) {
  int64 handle_start_micros = 0;
  if (CAT_LOG_IS_ON(3)) {
    handle_start_micros = Env::Default()->NowMicros();
  }
  Schedule([this, tag, handle_start_micros]() {
    auto* call_opts = new CallOptions;
    auto* call = new SeastarCall<RecvTensorRequest, SeastarTensorResponse>();
    InitSeastarServerTag(&call->req_, &call->resp_, tag,
                         [call](const Status& s) { delete call; });
    int64 delta_micros = 0;
    if (CAT_LOG_IS_ON(3)) {
      // delta_micros records the timestamp's different between client/server
      // we assumed network communication takes about 50us (statistic from ping)
      delta_micros = handle_start_micros - call->req_.recv_req_start_micros() - 50;
      CAT_LOG(3)::logDuration(CAT_REPORTER::seastar_time_trace,
                              "RecvReqStartToRpcHandle", delta_micros);
      call_opts->SetDeltaMicros(delta_micros);
    }

    worker_->RecvTensorAsync(call_opts, &call->req_, &call->resp_,
                             [tag, call, call_opts](const Status& s) {
                               delete call_opts;
                               tag->ProcessDone(s);
                             });
  });
}

void SeastarWorkerService::Schedule(std::function<void()> f) {
  worker_->env()->compute_pool->Schedule(std::move(f));
}

WorkerEnv* SeastarWorker::env() { return env_; }

SeastarWorker::SeastarWorker(WorkerEnv* worker_env) : Worker(worker_env) {}

void SeastarWorker::RecvTensorAsync(CallOptions* opts,
                                    const RecvTensorRequest* request,
                                    SeastarTensorResponse* response,
                                    StatusCallback done) {
  if (CAT_LOG_IS_ON(3)) {
    int64 recv_start_micros = request->recv_req_start_micros();
    int64 delta_micros = opts->GetDeltaMicros();
    if (recv_start_micros > 0) {
      CAT_LOG(3)::logDuration(CAT_REPORTER::seastar_time_trace,
                              "RecvReqStartToSched",
                              Env::Default()->NowMicros() -
                                  delta_micros - recv_start_micros);
    }
  }

  const int64 step_id = request->step_id();
  const string& key = request->rendezvous_key();
  Rendezvous::ParsedKey parsed;

  Status s = Rendezvous::ParseKey(key, &parsed);
  Device* src_dev = nullptr;
  if (s.ok()) {
    s = PrepareRecvTensor(parsed, &src_dev);
  }
  if (!s.ok()) {
    LOG(WARNING) << "PrepareRecvTensor failed, tensor:" << key;
    done(s);
    abort();
  }

  // Request the tensor associated with the rendezvous key.
  // Note that we log the cancellation here but do not abort the current step.
  // gRPC can generate cancellations in response to transient network failures,
  // and aborting the step eliminates the opportunity for client side retries.
  // Repeated client failures will eventually cause the step to be aborted by
  // the client.
  opts->SetCancelCallback(
      [step_id]() { LOG(WARNING) << "Seastar RecvTensor cancelled for " << step_id; });
  env_->rendezvous_mgr->RecvLocalAsync(
      step_id, parsed,
      [opts, request, response, done, src_dev, key](
          const Status& status, const Rendezvous::Args& send_args,
          const Rendezvous::Args& recv_args, const Tensor& val,
          const bool is_dead) {
        opts->ClearCancelCallback();
        if (status.ok()) {
          response->SetIsDead(is_dead);
          response->set_send_start_micros(Env::Default()->NowMicros());
          bool can_memcpy = DataTypeCanUseMemcpy(val.dtype());

          if (src_dev->tensorflow_gpu_device_info() &&
              (!send_args.alloc_attrs.on_host())) {
            CHECK(send_args.device_context)
                << "send dev name: " << src_dev->name()
                << " gpu_info: " << src_dev->tensorflow_gpu_device_info();

            AllocatorAttributes alloc_attrs;
            alloc_attrs.set_gpu_compatible(true);
            alloc_attrs.set_on_host(true);
            Allocator* alloc = src_dev->GetAllocator(alloc_attrs);
            auto* cpu_copy = new Tensor(alloc, val.dtype(), val.shape());

            send_args.device_context->CopyDeviceTensorToCPU(
                &val, request->rendezvous_key(), src_dev, cpu_copy,
                [response, cpu_copy, done](const Status& s) {
                  CHECK(s.ok()) << "copy tensor from gpu sync";
                  response->SetTensor(*cpu_copy);
                  delete cpu_copy;
                  done(s);
                });
          } else {
            // tensor is in CPU memory.
            response->SetTensor(val);
            if (!can_memcpy) {
              val.AsProtoTensorContent(&response->GetTensorProto());
            }
            done(Status());
          }
        } else {
          // !s.ok()
          done(status);
        }
        CAT_LOG(3)::logDuration(CAT_REPORTER::seastar_time_trace,
                                "RecvLocalDoneToRespSend",
                                Env::Default()->NowMicros() -
                                    std::max(send_args.rendezvous_micros,
                                             recv_args.rendezvous_micros));
      });
}

void SeastarWorker::FuseRecvTensorAsync(CallOptions* opts,
                                        const FuseRecvTensorRequest* request,
                                        SeastarFuseTensorResponse* response,
                                        StatusCallback done) {
  if (CAT_LOG_IS_ON(3)) {
    int64 recv_start_micros = request->recv_req_start_micros();
    int64 delta_micros = opts->GetDeltaMicros();
    if (recv_start_micros > 0) {
      CAT_LOG(3)::logDuration(CAT_REPORTER::seastar_time_trace,
                              "FuseRecvReqStartToSched",
                              Env::Default()->NowMicros() -
                                  delta_micros - recv_start_micros);
    }
  }

  const int64 step_id = request->step_id();
  int fuse_count = request->rendezvous_key_size();
  std::vector<Rendezvous::ParsedKey> parsed_keys(fuse_count);
  std::vector<Device*>* src_devs = new std::vector<Device*>(fuse_count, nullptr);

  for (int idx = 0; idx < fuse_count; ++idx) {
    const string& key = request->rendezvous_key(idx);
    Status s = Rendezvous::ParseKey(key, &parsed_keys[idx]);
    if (s.ok()) {
      s = PrepareRecvTensor(parsed_keys[idx], &(*src_devs)[idx]);
    }

    if (!s.ok()) {
      LOG(WARNING) << "PrepareRecvTensor failed, tensor:" << key;
      delete src_devs;
      done(s);
      return;
    }
  }

  opts->SetCancelCallback(
      [step_id]() {
        LOG(WARNING) << "Seastar FuseRecvTensor cancelled for " << step_id;
      });
  int64 prepare_start = Env::Default()->NowMicros();
  env_->rendezvous_mgr->FuseRecvLocalAsync(
      step_id, parsed_keys,
      [opts, request, response, done, fuse_count, src_devs, step_id, prepare_start](
          const Status& status,
          const std::vector<Rendezvous::Args>& send_argses,
          const Rendezvous::Args& recv_args,
          const std::vector<Tensor>& vals,
          const std::vector<bool>& is_deads) {

        //log total data prepare time
        CAT_LOG(3)::logDuration(CAT_REPORTER::seastar_time_trace,
                                "FuseRecvDataPrepareCost",
                                Env::Default()->NowMicros() - prepare_start);

        opts->ClearCancelCallback();
        if (!status.ok()) {
          VLOG(1) << "env_->rendezvous_mgr->FuseRecvLocalAsync failed, error msg is: "
                  << status.error_message();
        }
        if (status.ok()) {
          int64 start_prepare_resp = Env::Default()->NowMicros();
          response->Init(fuse_count);
          int *fuse_counter = new int(fuse_count);

          for (int idx = 0; idx < fuse_count; ++idx) {
            response->SetIsDeadByIndex(idx, is_deads[idx]);

            bool can_memcpy = DataTypeCanUseMemcpy(vals[idx].dtype());

            if ((*src_devs)[idx]->tensorflow_gpu_device_info() &&
                (!send_argses[idx].alloc_attrs.on_host())) {
              done(errors::Internal("No GPU device in process"));
            } else {
              // tensor is in CPU memory.
              response->SetTensorByIndex(idx, vals[idx]);
              if (!can_memcpy) {
                vals[idx].AsProtoTensorContent(&response->GetTensorProtoByIndex(idx));
              }
              if (__sync_sub_and_fetch(fuse_counter, 1) == 0) {
                response->set_send_start_micros(Env::Default()->NowMicros());
                done(Status());
                delete src_devs;
                delete fuse_counter;
              }
            }
          } // end of cycle for with fuse_count

          CAT_LOG(3)::logDuration(CAT_REPORTER::seastar_time_trace,
                                  "FuseRecvLocalDoneToRespSend",
                                  Env::Default()->NowMicros() - start_prepare_resp);
        } else {
          delete src_devs;
          done(status);
        }
      });
}

std::unique_ptr<SeastarWorker> NewSeastarWorker(WorkerEnv* worker_env) {
  return std::make_unique<SeastarWorker>(worker_env);
}

std::unique_ptr<SeastarWorkerService> NewSeastarWorkerService(
    SeastarWorker* worker) {
  return std::make_unique<SeastarWorkerService>(worker);
}

}  // namespace tensorflow
