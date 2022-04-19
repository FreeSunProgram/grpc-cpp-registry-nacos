//
// Created by 孙逍 on 2022/4/15.
//

#ifndef GRPC_NACOS_HEALTH_CHECK_SERVICE_H
#define GRPC_NACOS_HEALTH_CHECK_SERVICE_H

#include <grpcpp/health_check_service_interface.h>

namespace grpc {

class NacosHealthCheckService final : public HealthCheckServiceInterface {
public:
    ~NacosHealthCheckService() override;

    void SetServingStatus(const string &service_name, bool serving) override;

    void SetServingStatus(bool serving) override;

    void Shutdown() override;

};

}   // namespace grpc

#endif //GRPC_NACOS_HEALTH_CHECK_SERVICE_H
