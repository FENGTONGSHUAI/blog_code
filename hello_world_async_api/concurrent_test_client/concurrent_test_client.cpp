/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <mutex>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"

#include "hello_world.grpc.pb.h"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(int, num_requests, 100, "Number of concurrent requests to send");

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class AsyncGreeterClient {
 public:
  explicit AsyncGreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // 发送多个并发异步请求
  void SendConcurrentRequests(const std::string& name_prefix, int num_requests) {
    std::cout << "Sending " << num_requests << " concurrent requests..." << std::endl;
    
    // 用于跟踪请求的完成状态
    std::atomic<int> completed_count{0};
    std::atomic<int> success_count{0};
    
    auto start_time = std::chrono::steady_clock::now();
    
    // 第一步：发起所有异步请求
    for (int i = 0; i < num_requests; i++) {
      // 为每个请求创建唯一的上下文和回复对象
      auto* context = new ClientContext();
      auto* reply = new HelloReply();
      auto* status = new Status();
      
      // 准备请求
      HelloRequest request;
      request.set_name(name_prefix + "_" + std::to_string(i));
      
      AsyncCallData* call_data = new AsyncCallData{
        i, completed_count, success_count, 
        std::chrono::steady_clock::now(),
        context, reply, status
      };
      
      // 发起异步调用
      std::unique_ptr<ClientAsyncResponseReader<HelloReply>> rpc(
          stub_->AsyncSayHello(context, request, &cq_));
      
      // 请求在RPC完成时回调
      rpc->Finish(reply, status, (void*)call_data);
    }
    
    std::cout << "All " << num_requests << " requests initiated. Waiting for responses..." << std::endl;
    
    // 第二步：处理所有响应
    ProcessResponses(num_requests);
    
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    
    // 显示统计信息
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "Total time: " << total_duration.count() << "ms" << std::endl;
    std::cout << "Total requests: " << num_requests << std::endl;
    std::cout << "Success rate: " << (success_count_ * 100.0 / num_requests) << "%" << std::endl;
    
    if (success_count_ > 0) {
      std::cout << "Average latency: " << (total_latency_ms_ / success_count_) << "ms" << std::endl;
      std::cout << "Requests per second: " 
                << (success_count_ * 1000.0 / total_duration.count()) << std::endl;
    }
    
    if (success_count_ < num_requests) {
      std::cout << "Failed requests: " << (num_requests - success_count_) << std::endl;
    }
  }

 private:
  // 创建用于跟踪请求的tag
  struct AsyncCallData {
    int request_id;
    std::atomic<int>& completed_count;
    std::atomic<int>& success_count;
    std::chrono::steady_clock::time_point start_time;
    ClientContext* context;
    HelloReply* reply;
    Status* status;
  };
  // 处理完成队列中的所有响应
  void ProcessResponses(int expected_responses) {
    int received_count = 0;
    
    while (received_count < expected_responses) {
      void* got_tag;
      bool ok = false;
      
      // 阻塞直到下一个结果可用
      if (!cq_.Next(&got_tag, &ok)) {
        std::cerr << "Completion queue shutdown unexpectedly" << std::endl;
        break;
      }
      
      received_count++;
      
      // 获取回调数据
      AsyncCallData* call_data = static_cast<AsyncCallData*>(got_tag);
      
      // 计算延迟
      auto end_time = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
          end_time - call_data->start_time);
      
      // 根据状态处理响应
      if (ok && call_data->status->ok()) {
        success_count_++;
        total_latency_ms_ += duration.count();
      } else {
        std::cout << "Request " << call_data->request_id << " failed: ";
        if (!ok) {
          std::cout << "operation not ok";
        } else {
          std::cout << call_data->status->error_code() 
                   << " - " << call_data->status->error_message();
        }
        std::cout << " (took " << duration.count() << "ms)" << std::endl;
      }
      
      // 清理资源
      delete call_data->context;
      delete call_data->reply;
      delete call_data->status;
      delete call_data;
    }
  }

  std::unique_ptr<Greeter::Stub> stub_;
  CompletionQueue cq_;
  std::atomic<int> success_count_{0};
  std::atomic<long long> total_latency_ms_{0};
};

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  
  std::string target_str = absl::GetFlag(FLAGS_target);
  int num_requests = absl::GetFlag(FLAGS_num_requests);
  
  std::cout << "=== gRPC Async Concurrency Test ===" << std::endl;
  std::cout << "Target server: " << target_str << std::endl;
  std::cout << "Number of concurrent requests: " << num_requests << std::endl;
  std::cout << "===============================" << std::endl;
  
  // 创建通道
  auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
  
  // 创建异步客户端
  AsyncGreeterClient client(channel);
  
  // 运行测试
  client.SendConcurrentRequests("async_user", num_requests);
  
  return 0;
}