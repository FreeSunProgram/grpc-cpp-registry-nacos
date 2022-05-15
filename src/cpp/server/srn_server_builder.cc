//
// Created by 孙逍 on 2022/4/13.
//

#include <grpcpp/impl/service_type.h>
#include <grpcpp/srn_server_builder.h>

#include <utility>
#include "health/nacos_health_check_service.h"
#include "srn_server.h"

#include "third_party/nacos-sdk-cpp/src/utils/NetUtils.h"

namespace grpc {

SrnServerBuilder::SrnServerBuilder(std::string name_server_uri)
        : name_server_addr_(std::move(name_server_uri)),
          ServerBuilder() {
    auto create_server
        = [] (
            grpc::ChannelArguments* args,
            std::shared_ptr<std::vector<std::unique_ptr<grpc::ServerCompletionQueue>>>
            sync_server_cqs,
            int min_pollers, int max_pollers, int sync_cq_timeout_msec,
            std::vector<std::shared_ptr<grpc::internal::ExternalConnectionAcceptorImpl>>
            acceptors,
            grpc_server_config_fetcher* server_config_fetcher,
            grpc_resource_quota* server_rq,
            std::vector<
                    std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
            interceptor_creators) {
        return (Server*) new SrnServer(
                args, sync_server_cqs, min_pollers,
                max_pollers, sync_cq_timeout_msec,
                std::move(acceptors), server_config_fetcher, server_rq,
                std::move(interceptor_creators));
    };
    InternalAddServerFactory(create_server);
}

SrnServerBuilder& SrnServerBuilder::AddListeningPort(const int& port,
                                                     std::shared_ptr<grpc::ServerCredentials> creds, int* selected_port) {
    server_addr_.first = "0.0.0.0";
    try {
        server_addr_.first = nacos::NetUtils::getHostIp();
    }
    catch (nacos::NacosException &e) {
        gpr_log(GPR_ERROR, "[srn_server %p] %s", this, e.what());
    }
    auto addrStr = server_addr_.first + ":" + std::to_string(port);
    server_addr_.second = port;
    return (SrnServerBuilder&)ServerBuilder::AddListeningPort(addrStr, std::move(creds), selected_port);
}

SrnServerBuilder::~SrnServerBuilder() = default;

std::unique_ptr<grpc::Server> SrnServerBuilder::BuildAndStart() {
    auto server = ServerBuilder::BuildAndStart();
    auto srn_server = (SrnServer*)server.get();
    if (!name_server_addr_.empty()) {
        srn_server->RegisterServiceToNacos(name_server_addr_, server_addr_);
    }
    return server;
}

ChannelArguments SrnServerBuilder::BuildChannelArgs() {
    auto args = ServerBuilder::BuildChannelArgs();
//    args.SetPointer(grpc::kHealthCheckServiceInterfaceArg, new NacosHealthCheckService);
    return args;
}

};