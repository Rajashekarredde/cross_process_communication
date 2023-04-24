#include <atomic>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "mpi.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using mpi::MPIService;
using mpi::SendRequest;
using mpi::SendResponse;
using mpi::RecvRequest;
using mpi::RecvResponse;
using mpi::BarrierRequest;
using mpi::BarrierResponse;
using mpi::IBarrierRequest;
using mpi::IBarrierResponse;

class MPIServiceImpl final : public MPIService::Service {
  std::map<int32_t, std::string> messages_;
  std::atomic<int> barrier_count_{0};
  std::atomic<int> ibarrier_count_{0};
  int comm_size_;

 public:
  MPIServiceImpl(int comm_size) : comm_size_(comm_size) {}

  Status Send(ServerContext* context, const SendRequest* request, SendResponse* response) override {
    messages_[request->receiver_rank()] = request->message();
    response->set_success(true);
    return Status::OK;
  }

  Status Recv(ServerContext* context, const RecvRequest* request, RecvResponse* response) override {
    auto it = messages_.find(request->receiver_rank());
    if (it != messages_.end()) {
      response->set_sender_rank(it->first);
      response->set_message(it->second);
      messages_.erase(it);
    } else {
      return Status(grpc::StatusCode::NOT_FOUND, "Message not found");
    }
    return Status::OK;
  }

  Status Barrier(ServerContext* context, const BarrierRequest* request, BarrierResponse* response) override {
    int process_rank = request->process_rank();
    barrier_count_++;
    while (barrier_count_.load() != comm_size_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    response->set_success(true);
    return Status::OK;
  }

  Status IBarrier(ServerContext* context, const IBarrierRequest* request, IBarrierResponse* response) override {
    int process_rank = request->process_rank();
    ibarrier_count_++;
    response->set_success(true);
    return Status::OK;
  }

  bool WaitForIBarrier() {
    while (ibarrier_count_.load() != comm_size_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
  }
};

void RunServer(int comm_size) {
  std::string server_address("0.0.0.0:50051");
  MPIServiceImpl service(comm_size);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  server->Wait();
}

int main(int argc, char** argv) {
  int comm_size = 4;
  RunServer(comm_size);
  return 0;
}

