//
// Created by 孙逍 on 2022/4/15.
//

#include "nacos_health_check_service.h"
#include <iostream>

grpc::NacosHealthCheckService::~NacosHealthCheckService() {

}

void grpc::NacosHealthCheckService::SetServingStatus(const std::string &service_name, bool serving) {
    std::cout << service_name << ":" << serving << std::endl;
}

void grpc::NacosHealthCheckService::SetServingStatus(bool serving) {
    std::cout << serving << std::endl;
}

void grpc::NacosHealthCheckService::Shutdown() {
    std::cout << "Server shutdown" << std::endl;
    HealthCheckServiceInterface::Shutdown();
}
