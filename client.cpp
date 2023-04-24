#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "mpi.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using mpi::MPIService;
using mpi::BarrierRequest;
using mpi::BarrierResponse;
using mpi::IBarrierRequest;
using mpi::IBarrierResponse;

class MPIClient {
 public:
  MPIClient(std::shared_ptr<Channel> channel) : stub_(MPIService::NewStub(channel)) {}

  bool Barrier(int32_t process_rank) {
    BarrierRequest request;
    request.set_process_rank(process_rank);

    BarrierResponse response;
    ClientContext
    ClientContext context;
    Status status = stub_->Barrier(&context, request, &response);
    if (status.ok()) {
      return response.success();
    } else {
      std::cout << "Barrier RPC failed." << std::endl;
      return false;
    }
  }

  bool IBarrier(int32_t process_rank) {
    IBarrierRequest request;
    request.set_process_rank(process_rank);

    IBarrierResponse response;
    ClientContext context;
    Status status = stub_->IBarrier(&context, request, &response);
    if (status.ok()) {
      return response.success();
    } else {
      std::cout << "IBarrier RPC failed." << std::endl;
      return false;
    }
  }

 private:
  std::unique_ptr<MPIService::Stub> stub_;
};

void RunClient(int process_rank) {
  MPIClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

  // Simulate an MPI_Barrier() operation
  bool barrier_success = client.Barrier(process_rank);
  if (barrier_success) {
    std::cout << "Process " << process_rank << " passed the barrier." << std::endl;
  } else {
    std::cout << "Failed to pass the barrier for process " << process_rank << "." << std::endl;
  }

  // Simulate an MPI_IBarrier() operation
  bool ibarrier_success = client.IBarrier(process_rank);
  if (ibarrier_success) {
    std::cout << "Process " << process_rank << " entered the IBarrier." << std::endl;
  } else {
    std::cout << "Failed to enter the IBarrier for process " << process_rank << "." << std::endl;
  }
}

int main(int argc, char** argv) {
  int comm_size = 4;
  std::vector<std::thread> threads;

  for (int i = 0; i < comm_size; ++i) {
    threads.push_back(std::thread(RunClient, i));
  }

  for (auto& t : threads) {
    t.join();
  }

  return 0;
}
