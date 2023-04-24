#include <atomic>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "mpi.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using mpi::MPIService;
using mpi::BarrierRequest;
using mpi::BarrierResponse;
using mpi::IBarrierRequest;
using mpi::IBarrierResponse;

class MPIServiceImpl final : public MPIService::Service {
  std::atomic<int> barrier_count_{0};
  std::atomic<int> ibarrier_count_{0};
  int comm_size_; // Total number of processes

 public:
  MPIServiceImpl(int comm_size) : comm_size_(comm_size) {}

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
