//
// Created by 孙逍 on 2022/4/15.
//

#ifndef GRPC_SRN_SERVER_H
#define GRPC_SRN_SERVER_H

#include <grpcpp/server.h>

namespace nacos {
    class NamingService;
    class Instance;
}

namespace grpc {

using grpc::internal::RpcServiceMethod;
using grpc::internal::RpcMethod;

class SrnServer final : public Server {
public:
    ~SrnServer()  override;
    void Wait() override;

protected:
    bool RegisterService(const std::string *addr, Service *service) override;

    int AddListeningPort(const string &addr, ServerCredentials *creds) override;

    SrnServer(ChannelArguments* args,
              std::shared_ptr<std::vector<std::unique_ptr<ServerCompletionQueue>>>
              sync_server_cqs,
              int min_pollers, int max_pollers, int sync_cq_timeout_msec,
              std::vector<std::shared_ptr<internal::ExternalConnectionAcceptorImpl>>
              acceptors,
              grpc_server_config_fetcher* server_config_fetcher = nullptr,
              grpc_resource_quota* server_rq = nullptr,
              std::vector<
                      std::unique_ptr<experimental::ServerInterceptorFactoryInterface>>
              interceptor_creators = std::vector<std::unique_ptr<
                      experimental::ServerInterceptorFactoryInterface>>());

    void Start(ServerCompletionQueue **cqs, size_t num_cqs) override;

    grpc_server *server() override;

private:
    friend class ServerBuilder;
    friend class SrnServerBuilder;
    friend class ServerInitializer;

    ///
    void RegisterServiceToNacos(const std::string& name_server_addr, const std::pair<std::string, int>& server_addr);
    /// 获取Rpc方法类型
    std::string GetRpcMethodType(const std::unique_ptr<RpcServiceMethod>& method);
    /// 命名服务地址
    std::string naming_server_addr_;
    /// 命名服务
    std::unique_ptr<nacos::NamingService> naming_server_;
    /// 实例
    std::map<std::string, nacos::Instance> service_instances_;
};

}   // namespace grpc

#endif //GRPC_SRN_SERVER_H
