//
// Created by 孙逍 on 2022/4/13.
//

#ifndef GRPC_SRN_SERVER_BUILDER_H
#define GRPC_SRN_SERVER_BUILDER_H

#include <grpcpp/server_builder.h>

namespace grpc {

// Server registry nacos add by sx
class SrnServerBuilder : public ServerBuilder {
public:
    SrnServerBuilder(std::string name_server_addr = "");
    ~SrnServerBuilder() override;

    /// Enlists an endpoint \a addr (port with an optional IP address) to
    /// bind the \a grpc::Server object to be created to.
    ///
    /// It can be invoked multiple times.
    ///
    /// \param port server listening port
    /// \param creds The credentials associated with the server.
    /// \param[out] selected_port If not `nullptr`, gets populated with the port
    /// number bound to the \a grpc::Server for the corresponding endpoint after
    /// it is successfully bound by BuildAndStart(), 0 otherwise. AddListeningPort
    /// does not modify this pointer.
    SrnServerBuilder& AddListeningPort(
            const int& port,
            std::shared_ptr<grpc::ServerCredentials> creds,
            int* selected_port = nullptr);

    /// Return a running server which is ready for processing calls.
    /// Before calling, one typically needs to ensure that:
    ///  1. a service is registered - so that the server knows what to serve
    ///     (via RegisterService, or RegisterAsyncGenericService)
    ///  2. a listening port has been added - so the server knows where to receive
    ///     traffic (via AddListeningPort)
    ///  3. [for async api only] completion queues have been added via
    ///     AddCompletionQueue
    ///
    ///  Will return a nullptr on errors.
    std::unique_ptr<grpc::Server> BuildAndStart() override;

protected:
    /// Experimental API, subject to change.
    ChannelArguments BuildChannelArgs() override;

private:
    std::pair<std::string, int> server_addr_;
    std::string name_server_addr_;
};

}
#endif //GRPC_SRN_SERVER_BUILDER_H
