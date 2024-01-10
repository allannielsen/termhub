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

#ifndef AGRPC_DETAIL_ALARM_HPP
#define AGRPC_DETAIL_ALARM_HPP

#include <agrpc/detail/config.hpp>
#include <agrpc/detail/forward.hpp>
#include <agrpc/detail/grpc_sender.hpp>
#include <agrpc/detail/sender_implementation.hpp>
#include <agrpc/detail/wait.hpp>

AGRPC_NAMESPACE_BEGIN()

namespace detail
{
template <class Executor>
struct MoveAlarmSenderImplementation
{
    static constexpr auto TYPE = detail::SenderImplementationType::GRPC_TAG;
    static constexpr bool NEEDS_ON_COMPLETE = true;

    using Alarm = agrpc::BasicAlarm<Executor>;
    using Signature = void(bool, Alarm);
    using StopFunction = detail::AlarmCancellationFunction;

    template <class OnComplete>
    void complete(OnComplete on_complete, bool ok)
    {
        on_complete(ok, static_cast<Alarm&&>(alarm_));
    }

    auto& grpc_alarm() { return alarm_.alarm_; }

    Alarm alarm_;
};

template <class Executor>
struct SenderMoveAlarmSenderImplementation : MoveAlarmSenderImplementation<Executor>
{
    using Alarm = agrpc::BasicAlarm<Executor>;
    using Signature = void(Alarm);

    template <class OnComplete>
    void complete(OnComplete on_complete, bool ok)
    {
        if (ok)
        {
            on_complete(static_cast<Alarm&&>(this->alarm_));
        }
        else
        {
            on_complete.done();
        }
    }
};

template <class Deadline>
struct MoveAlarmSenderInitiation
{
    template <class Executor>
    static auto& stop_function_arg(MoveAlarmSenderImplementation<Executor>& impl) noexcept
    {
        return impl.grpc_alarm();
    }

    template <class Executor>
    void initiate(agrpc::GrpcContext& grpc_context, MoveAlarmSenderImplementation<Executor>& impl, void* tag) const
    {
        detail::AlarmInitFunction{impl.grpc_alarm(), deadline_}(grpc_context, tag);
    }

    Deadline deadline_;
};

struct SenderAlarmSenderImplementation : detail::GrpcSenderImplementation<detail::AlarmCancellationFunction>
{
    static constexpr bool NEEDS_ON_COMPLETE = true;

    using Signature = void();

    template <class OnComplete>
    static void complete(OnComplete on_complete, bool ok)
    {
        if (ok)
        {
            on_complete();
        }
        else
        {
            on_complete.done();
        }
    }
};
}

AGRPC_NAMESPACE_END

#endif  // AGRPC_DETAIL_ALARM_HPP
