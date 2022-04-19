//
// Created by sx on 2022/4/5.
//

#include "src/core/lib/resolver/resolver_registry.h"
#include "src/core/lib/config/core_configuration.h"
#include "src/core/lib/address_utils/parse_address.h"
#include "naming/selectors/HealthInstanceSelector.h"
#include "Nacos.h"

namespace grpc_core {

/// 调试用
TraceFlag grpc_nacos_resolver_trace(false, "nacos_resolver");

class NacosResolver : public Resolver {
public:
    explicit NacosResolver(ResolverArgs args);

    ~NacosResolver() override;

    void StartLocked() override;

    void RequestReresolutionLocked() override;

    void ResetBackoffLocked() override;

    void ShutdownLocked() override;

private:
    /// 创建Nacos命名服务
    void CreateNamingServer();
    /// nacos 服务地址
    std::string nacos_server_;
    /// 服务名
    std::string name_to_resolve_;
    /// 通道参数
    grpc_channel_args* channel_args_;
    /// 回调任务执行序列,将任务通过Run丢进执行队列执行
    std::shared_ptr<WorkSerializer> work_serializer_;
    /// 结果句柄，通过它返回resolve结果
    std::unique_ptr<ResultHandler> result_handler_;
    /// 不详 pollset_set to drive the name resolution process
    grpc_pollset_set* interested_parties_;
    /// 命名服务
    std::unique_ptr<nacos::NamingService> naming_server_;
};

NacosResolver::NacosResolver(ResolverArgs args)
    : nacos_server_(args.uri.authority()),
      name_to_resolve_(absl::StripPrefix(args.uri.path(), "/")),
      channel_args_(grpc_channel_args_copy(args.args)),
      work_serializer_(std::move(args.work_serializer)),
      result_handler_(std::move(args.result_handler)),
      interested_parties_(args.pollset_set) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_nacos_resolver_trace)) {
        gpr_log(GPR_INFO, "[nacos_resolver %p] created for server name %s", this,
                name_to_resolve_.c_str());
    }
    CreateNamingServer();
}

NacosResolver::~NacosResolver() {
    grpc_channel_args_destroy(channel_args_);
    if (GRPC_TRACE_FLAG_ENABLED(grpc_nacos_resolver_trace)) {
        gpr_log(GPR_INFO, "[nacos_resolver %p] destroyed", this);
    }
}

void NacosResolver::StartLocked() {
    if (!naming_server_) {
        Result result;
        result.service_config = absl::UnavailableError("Resolver transient failure");
        result_handler_->ReportResult(result);
        return;
    }
    try {
        nacos::naming::selectors::HealthInstanceSelector selector;
        auto instances =
                naming_server_->getInstanceWithPredicate(name_to_resolve_, &selector);

        Result result;
        grpc_arg new_args[] = {};
        result.args = grpc_channel_args_copy_and_add(channel_args_, new_args,
                                                     GPR_ARRAY_SIZE(new_args));
        grpc_resolved_address addr{};
        for (const auto& instance : instances) {
            auto uri = URI::Create("ipv4", {},
                                 instance.ip + ":" + nacos::NacosStringOps::valueOf(instance.port),
                                 {}, {});
            if (uri.ok() and grpc_parse_uri(uri.value(), &addr)) {
                result.addresses.value().emplace_back(addr, nullptr);
            }
        }
        result_handler_->ReportResult(result);
    }
    catch (nacos::NacosException &e) {
        gpr_log(GPR_INFO, "[nacos_resolver %p] %s", this, e.what());
        Result result;
        result.service_config = absl::UnavailableError(e.what());
        result_handler_->ReportResult(result);
    }
}

void NacosResolver::RequestReresolutionLocked() {
    Resolver::RequestReresolutionLocked();
}

void NacosResolver::ResetBackoffLocked() {
    // TODO
}

void NacosResolver::ShutdownLocked() {
    // TODO
}

void NacosResolver::CreateNamingServer() {
    try {
        // TODO nacos_server_ 空时从配置读取
        nacos::Properties nacos_props;
        nacos_props[nacos::PropertyKeyConst::SERVER_ADDR] = nacos_server_;
        std::unique_ptr<nacos::INacosServiceFactory> factory(
                nacos::NacosFactoryFactory::getNacosFactory(nacos_props));
        naming_server_ = std::unique_ptr<nacos::NamingService>(factory->CreateNamingService());
    }
    catch (nacos::NacosException &e) {
        gpr_log(GPR_INFO, "[nacos_resolver %p] %s", this, e.what());
    }
}

class NacosResolverFactory : public ResolverFactory {
public:
    bool IsValidUri(const URI &uri) const override {
        // 符合：nacos:///service
        if (absl::StripPrefix(uri.path(), "/").empty()) {
            gpr_log(GPR_ERROR, "no server name supplied in nacos URI");
            return false;
        }
        return true;
    }

    OrphanablePtr<Resolver> CreateResolver(ResolverArgs args) const override {
        if (!IsValidUri(args.uri)) return nullptr;
        return MakeOrphanable<NacosResolver>(std::move(args));
    }

    absl::string_view scheme() const override {
        return "nacos";
    }
};

void RegisterNacosResolver(CoreConfiguration::Builder* builder) {
  builder->resolver_registry()->RegisterResolverFactory(
      absl::make_unique<grpc_core::NacosResolverFactory>());
}

}   // namespace grpc_core

void grpc_resolver_nacos_init() {

}

void grpc_resolver_nacos_shutdown() {

}