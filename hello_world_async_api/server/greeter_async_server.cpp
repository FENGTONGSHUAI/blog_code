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
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/strings/str_format.h"

#include "hello_world.grpc.pb.h"

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class ServerImpl final {
 public:
  ~ServerImpl() {
    server_->Shutdown();
    // 总是在服务器之后关闭完成队列。
    cq_->Shutdown();
  }

  // 此代码中没有关闭处理逻辑。
  void Run(uint16_t port) {
    std::string server_address = absl::StrFormat("0.0.0.0:%d", port);

    ServerBuilder builder;
    // 在给定地址上监听，不使用任何认证机制。
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 注册"service_"作为与客户端通信的实例。
    // 这里对应的是一个*异步*服务。
    builder.RegisterService(&service_);
    // 获取用于与gRPC运行时进行异步通信的完成队列。
    cq_ = builder.AddCompletionQueue();
    // 最后组装服务器。
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // 进入服务器主循环。
    HandleRpcs();
  }

 private:
  // 包含处理请求所需的状态和逻辑的类。
  class CallData {
   public:
    // 接收"service"实例（这里代表异步服务器）和用于与gRPC运行时
    // 进行异步通信的完成队列"cq"。
    CallData(Greeter::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // 立即调用服务逻辑。
      std::cout << "CallData init, CallData = " << this << std::endl;
      Proceed();
    }

    void Proceed() {
      if (status_ == CREATE) {
        // 使此实例进入PROCESS状态。
        status_ = PROCESS;

        // 作为初始CREATE状态的一部分，我们*请求*系统开始处理SayHello请求。
        // 在此请求中，"this"作为唯一标识请求的标签（使不同CallData实例可以并发处理不同请求），
        // 这里是此CallData实例的内存地址。
        service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
        std::cout << "CallData entering PROCESS state, CallData =" << this << std::endl;
      } else if (status_ == PROCESS) {
        // 在处理当前CallData的请求时，生成一个新的CallData实例来服务新客户端。
        // 该实例将在其FINISH状态时自行释放内存。
        new CallData(service_, cq_);

        // 实际处理过程。
        std::string prefix("Hello ");
        reply_.set_message(prefix + request_.name());

        // 处理完成！让gRPC运行时知道我们已经完成，
        // 使用此实例的内存地址作为事件的唯一标识标签。
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
        std::cout << "CallData entering FINISH state, CallData =" << this << std::endl;
      } else {
        CHECK_EQ(status_, FINISH);
        // 进入FINISH状态后，自行释放内存（CallData）。
        delete this;
        std::cout << "CallData delete, CallData =" << this << std::endl;
      }
    }

   private:
    // 与gRPC运行时进行异步服务器通信的方式。
    Greeter::AsyncService* service_;
    // 用于异步服务器通知的生产者-消费者队列。
    ServerCompletionQueue* cq_;
    // RPC的上下文，允许调整其各方面设置，如使用压缩、认证，以及向客户端发送元数据。
    ServerContext ctx_;

    // 从客户端接收的内容。
    HelloRequest request_;
    // 发送回客户端的内容。
    HelloReply reply_;

    // 用于响应客户端的方式。
    ServerAsyncResponseWriter<HelloReply> responder_;

    // 用一个微型状态机实现以下状态。
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // 当前服务状态。
  };

  // 如果需要，可以在多个线程中运行。
  void HandleRpcs() {
    // 生成一个新的CallData实例来服务新客户端。
    new CallData(&service_, cq_.get());
    void* tag;  // 唯一标识一个请求。
    bool ok;
    while (true) {
      // 阻塞等待从完成队列中读取下一个事件。
      // 事件由其标签唯一标识，这里是一个CallData实例的内存地址。
      // 应始终检查Next的返回值，该值告诉我们是否有任何事件或cq_是否正在关闭。
      CHECK(cq_->Next(&tag, &ok));
      CHECK(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  Greeter::AsyncService service_;
  std::unique_ptr<Server> server_;
};

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  ServerImpl server;
  server.Run(absl::GetFlag(FLAGS_port));

  return 0;
}
