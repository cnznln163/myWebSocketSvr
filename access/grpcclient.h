#ifndef GRPC_CLIENT_H
#define GRPC_CLIENT_H

#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>

#include "helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class GRpcClient
{
public:
	GRpcClient();
	~GRpcClient();

	/* data */
};


#define GRpcClientManager CObjectManager<GRpcClient>

#endif
