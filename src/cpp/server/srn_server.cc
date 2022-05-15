//
// Created by 孙逍 on 2022/4/15.
//

#include "srn_server.h"
#include <cstdlib>
#include <sstream>
#include <type_traits>
#include <utility>

#include "absl/memory/memory.h"
#include "map"

#include <grpc/grpc.h>
#include <grpc/impl/codegen/grpc_types.h>

#include <grpcpp/completion_queue.h>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/byte_buffer.h>

#include <grpcpp/impl/codegen/server_interceptor.h>

#include <grpcpp/impl/server_initializer.h>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>

#include "src/core/lib/surface/server.h"
#include "src/cpp/server/external_connection_acceptor_impl.h"
#include "src/cpp/server/health/default_health_check_service.h"
#include "src/cpp/thread_manager/thread_manager.h"

#include "Nacos.h"

#include <iostream>

namespace grpc {

SrnServer::~SrnServer() {
    if (naming_server_) {
        try {
            auto iter = service_instances_.begin();
            while (iter != service_instances_.end()) {
              naming_server_->deregisterInstance(iter->second.serviceName,
                                                   iter->second.groupName,
                                                   iter->second);
                iter++;
            }
        }
        catch (nacos::NacosException &e) {
            gpr_log(GPR_ERROR, "[srn_server %p] %s", this, e.what());
        }
    }
}

void SrnServer::Wait() {
    Server::Wait();
}

bool SrnServer::RegisterService(const std::string *addr, grpc::Service *service) {
    const char *method_name = nullptr;
    std::string serviceName;
    std::map <NacosString, NacosString> metaData;
    for (const auto &method: *service->methods()) {
        method_name = method->name();
        metaData.insert({method_name, GetRpcMethodType(method)});
    }
    if (method_name != nullptr) {
        std::stringstream ss(method_name);
        if (std::getline(ss, serviceName, '/') &&
            std::getline(ss, serviceName, '/')) {
        }
        if (std::string::npos == serviceName.find("grpc") &&
            std::string::npos == serviceName.find("v1")) {
            auto &instance = service_instances_[serviceName];
            instance.serviceName = serviceName;
            instance.groupName = "DEFAULT_GROUP";
            instance.metadata = metaData;
        }
    }
    return Server::RegisterService(addr, service);
}

int SrnServer::AddListeningPort(const std::string &addr, grpc::ServerCredentials *creds) {
    return Server::AddListeningPort(addr, creds);
}

void SrnServer::Start(grpc::ServerCompletionQueue **cqs, size_t num_cqs) {
    Server::Start(cqs, num_cqs);
}

grpc_server *SrnServer::server() {
    return Server::server();
}

SrnServer::SrnServer(
        grpc::ChannelArguments *args,
        std::shared_ptr<std::vector<std::unique_ptr<ServerCompletionQueue>>> sync_server_cqs,
        int min_pollers, int max_pollers, int sync_cq_timeout_msec,
        std::vector<std::shared_ptr<internal::ExternalConnectionAcceptorImpl>> acceptors,
        grpc_server_config_fetcher *server_config_fetcher, grpc_resource_quota *server_rq,
        std::vector<std::unique_ptr<experimental::ServerInterceptorFactoryInterface>> interceptor_creators)
        : Server(args, std::move(sync_server_cqs), min_pollers, max_pollers, sync_cq_timeout_msec, acceptors,
                 server_config_fetcher, server_rq, std::move(interceptor_creators)) {
}

void SrnServer::RegisterServiceToNacos(const std::string &name_server_addr,
                                       const std::pair<std::string, int> &server_addr) {
    nacos::Properties nacos_props;
    nacos_props[nacos::PropertyKeyConst::SERVER_ADDR] = name_server_addr;
    nacos_props[nacos::PropertyKeyConst::UDP_RECEIVER_PORT] = "";
    try {
        std::unique_ptr<nacos::INacosServiceFactory> factory(
                nacos::NacosFactoryFactory::getNacosFactory(nacos_props));
        naming_server_ = std::unique_ptr<nacos::NamingService>(factory->CreateNamingService());
        if (naming_server_) {
            auto iter = service_instances_.begin();
            while (iter != service_instances_.end()) {
                iter->second.ip = server_addr.first;
                iter->second.port = server_addr.second;
                naming_server_->registerInstance(iter->first, iter->second);
                iter++;
            }
        }
    }
    catch (nacos::NacosException &e) {
        gpr_log(GPR_ERROR, "[srn_server %p] %s", this, e.what());
    }
}

std::string SrnServer::GetRpcMethodType(const std::unique_ptr<RpcServiceMethod>& method) {
    static std::map<RpcServiceMethod::ApiType, std::string> APIDICT = {
            {RpcServiceMethod::ApiType::SYNC, "SYNC"},
            {RpcServiceMethod::ApiType::ASYNC, "ASYNC"},
            {RpcServiceMethod::ApiType::RAW, "RAW"},
            {RpcServiceMethod::ApiType::CALL_BACK, "CALL_BACK"},
            {RpcServiceMethod::ApiType::RAW_CALL_BACK, "RAW_CALL_BACK"}
    };
    static std::map<RpcMethod::RpcType, std::string> RPCDICT = {
            {RpcMethod::RpcType::NORMAL_RPC, "NORMAL_RPC"},
            {RpcMethod::RpcType::CLIENT_STREAMING, "CLIENT_STREAMING"},
            {RpcMethod::RpcType::SERVER_STREAMING, "SERVER_STREAMING"},
            {RpcMethod::RpcType::BIDI_STREAMING, "BIDI_STREAMING"}
    };
    return APIDICT[method->api_type()] + "&" + RPCDICT[method->method_type()];
}

}