// Copyright 2023 Dennis Hezel
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef AGRPC_AGRPC_SERVER_RPC_PTR_HPP
#define AGRPC_AGRPC_SERVER_RPC_PTR_HPP

#include <agrpc/detail/config.hpp>
#include <agrpc/detail/forward.hpp>
#include <agrpc/detail/server_rpc_with_request.hpp>

#include <utility>

AGRPC_NAMESPACE_BEGIN()

/**
 * @brief (experimental) Allocated ServerRPC created by register_callback_rpc_handler
 *
 * @since 2.8.0
 */
template <class ServerRPCT>
class ServerRPCPtr
{
  public:
    /**
     * @brief The ServerRPC type
     */
    using ServerRPC = ServerRPCT;

    /**
     * @brief Default constructor
     *
     * The only valid operations after construction are move-assignment and destruction.
     */
    ServerRPCPtr() = default;

    ServerRPCPtr(const ServerRPCPtr& other) = delete;

    ServerRPCPtr(ServerRPCPtr&& other) noexcept
        : server_rpc_(std::exchange(other.server_rpc_, nullptr)), deleter_(std::exchange(other.deleter_, nullptr))
    {
    }

    ~ServerRPCPtr() noexcept { destruct(); }

    ServerRPCPtr& operator=(const ServerRPCPtr& other) = delete;

    ServerRPCPtr& operator=(ServerRPCPtr&& other) noexcept
    {
        destruct();
        server_rpc_ = std::exchange(other.server_rpc_, nullptr);
        deleter_ = std::exchange(other.deleter_, nullptr);
        return *this;
    }

    /**
     * @brief Get reference to underlying ServerRPC
     */
    ServerRPCT& operator*() noexcept { return server_rpc_->rpc_; }

    /**
     * @brief Get reference to underlying ServerRPC (const overload)
     */
    const ServerRPCT& operator*() const noexcept { return server_rpc_->rpc_; }

    /**
     * @brief Access underlying ServerRPC
     */
    ServerRPCT* operator->() noexcept { return &server_rpc_->rpc_; }

    /**
     * @brief Access underlying ServerRPC (const overload)
     */
    const ServerRPCT* operator->() const noexcept { return &server_rpc_->rpc_; }

    /**
     * @brief Get client's initial request message
     */
    decltype(auto) request() noexcept { return (server_rpc_->request_); }

    /**
     * @brief Get client's initial request message (const overload)
     */
    decltype(auto) request() const noexcept { return (server_rpc_->request_); }

  private:
    using Pointer = detail::ServerRPCWithRequest<ServerRPCT>*;
    using Deleter = void (*)(Pointer) noexcept;

    template <class, class, class>
    friend struct detail::RegisterCallbackRPCHandlerOperation;

    ServerRPCPtr(Pointer server_rpc, Deleter deleter) noexcept : server_rpc_(server_rpc), deleter_(deleter) {}

    auto* release() noexcept { return std::exchange(server_rpc_, nullptr); }

    void destruct() noexcept
    {
        if (server_rpc_)
        {
            deleter_(server_rpc_);
        }
    }

    Pointer server_rpc_{};
    Deleter deleter_{};
};

AGRPC_NAMESPACE_END

#endif  // AGRPC_AGRPC_SERVER_RPC_PTR_HPP
