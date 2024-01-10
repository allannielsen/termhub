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

#ifndef AGRPC_DETAIL_CANCEL_SAFE_HPP
#define AGRPC_DETAIL_CANCEL_SAFE_HPP

#include <agrpc/detail/asio_forward.hpp>
#include <agrpc/detail/config.hpp>
#include <agrpc/detail/tuple.hpp>
#include <agrpc/detail/utility.hpp>

AGRPC_NAMESPACE_BEGIN()

namespace detail
{
template <class Signature>
struct PrependErrorCodeToSignature;

template <class... Args>
struct PrependErrorCodeToSignature<void(detail::ErrorCode, Args...)>
{
    using Type = void(detail::ErrorCode, Args...);

    template <class Function>
    static void invoke_with_default_args(Function&& function, detail::ErrorCode&& ec)
    {
        static_cast<Function&&>(function)(static_cast<detail::ErrorCode&&>(ec), Args{}...);
    }
};

template <class... Args>
struct PrependErrorCodeToSignature<void(Args...)>
{
    using Type = void(detail::ErrorCode, Args...);

    template <class Function>
    static void invoke_with_default_args(Function&& function, detail::ErrorCode&& ec)
    {
        static_cast<Function&&>(function)(static_cast<detail::ErrorCode&&>(ec), Args{}...);
    }
};

template <class Signature>
using PrependErrorCodeToSignatureT = typename detail::PrependErrorCodeToSignature<Signature>::Type;

template <class CompletionHandler, class... Args>
void complete_successfully(CompletionHandler&& handler, detail::ErrorCode ec, Args&&... args)
{
    static_cast<CompletionHandler&&>(handler).complete(ec, static_cast<Args&&>(args)...);
}

template <class CompletionHandler, class... Args>
void complete_successfully(CompletionHandler&& handler, Args&&... args)
{
    static_cast<CompletionHandler&&>(handler).complete(detail::ErrorCode{}, static_cast<Args&&>(args)...);
}

template <class CompletionHandler, class... Args>
void invoke_successfully_from_tuple(CompletionHandler&& handler, detail::Tuple<detail::ErrorCode, Args...>&& args)
{
    detail::apply(static_cast<CompletionHandler&&>(handler),
                  static_cast<detail::Tuple<detail::ErrorCode, Args...>&&>(args));
}

template <class CompletionHandler, class... Args>
void invoke_successfully_from_tuple(CompletionHandler&& handler, detail::Tuple<Args...>&& args)
{
    detail::apply(static_cast<CompletionHandler&&>(handler),
                  detail::prepend_to_tuple(detail::ErrorCode{}, static_cast<detail::Tuple<Args...>&&>(args)));
}

#if defined(AGRPC_STANDALONE_ASIO) || defined(AGRPC_BOOST_ASIO)
template <class CompletionHandler, class... Args>
void complete_operation_aborted(CompletionHandler&& handler, detail::ErrorCode, Args&&... args)
{
    static_cast<CompletionHandler&&>(handler).complete(asio::error::operation_aborted, static_cast<Args&&>(args)...);
}

template <class CompletionHandler, class... Args>
void complete_operation_aborted(CompletionHandler&& handler, Args&&... args)
{
    static_cast<CompletionHandler&&>(handler).complete(asio::error::operation_aborted, static_cast<Args&&>(args)...);
}
#endif
}

AGRPC_NAMESPACE_END

#endif  // AGRPC_DETAIL_CANCEL_SAFE_HPP
