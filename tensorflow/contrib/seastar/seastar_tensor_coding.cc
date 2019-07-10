#include "tensorflow/contrib/seastar/seastar_tensor_coding.h"

#include "tensorflow/core/common_runtime/device.h"
#include "tensorflow/core/platform/logging.h"

namespace tensorflow {

void SeastarTensorResponse::InitAlloc(Device* d,
                                      const AllocatorAttributes& aa) {
  Clear();
  device_ = d;
  alloc_attrs_ = aa;
  const DeviceAttributes& da = d->attributes();
  if (alloc_attrs_.on_host() || da.device_type() == "CPU") {
    on_host_ = true;
  }
  allocator_ = device_->GetAllocator(alloc_attrs_);
}

void SeastarTensorResponse::Clear() {
  on_host_ = false;
  device_ = nullptr;
  alloc_attrs_ = AllocatorAttributes();
  allocator_ = nullptr;
  tensor_ = Tensor();
  tensor_proto_ = TensorProto();
}

void SeastarFuseTensorResponse::Clear() {
  SeastarTensorResponse::Clear();
  fuse_count_ = 0;
  tensors_.clear();
  tensor_protos_.clear();
  is_deads_.clear();
}

}  // namespace tensorflow
