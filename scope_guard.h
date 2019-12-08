/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 * Modifications copyright (c) 2019 John Drouhard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace jd {
namespace detail {

class ScopeGuardBase
{
public:
    void dismiss() noexcept {
        dismissed_ = true;
    }

protected:
    bool dismissed_ = false;
};

template <typename FunctionType, bool InvokeNoexcept>
class ScopeGuardImpl : public ScopeGuardBase
{
public:
    explicit ScopeGuardImpl(FunctionType & fn) noexcept(std::is_nothrow_copy_constructible_v<FunctionType>)
        : ScopeGuardImpl(std::as_const(fn), safeConstruct(std::is_nothrow_copy_constructible<FunctionType>{}, &fn))
    {
    }

    explicit ScopeGuardImpl(FunctionType const & fn) noexcept(std::is_nothrow_copy_constructible_v<FunctionType>)
        : ScopeGuardImpl(fn, safeConstruct(std::is_nothrow_copy_constructible<FunctionType>{}, &fn))
    {
    }

    explicit ScopeGuardImpl(FunctionType && fn) noexcept(std::is_nothrow_move_constructible_v<FunctionType>)
        : ScopeGuardImpl(std::move_if_noexcept(fn), safeConstruct(std::is_nothrow_move_constructible<FunctionType>{}, &fn))
    {
    }

    ScopeGuardImpl(ScopeGuardImpl && other) noexcept(std::is_nothrow_move_constructible_v<FunctionType>)
        : function_(std::move_if_noexcept(other.function_))
    {
        dismissed_ = std::exchange(other.dismissed_, true);
    }

    ~ScopeGuardImpl() noexcept(InvokeNoexcept) {
        if (!dismissed_) {
            execute();
        }
    }

private:
    template <typename Fn>
    explicit ScopeGuardImpl(Fn && f, ScopeGuardBase && failsafe)
        : ScopeGuardBase{}
        , function_(std::forward<Fn>(f))
    {
        failsafe.dismiss();
    }

    ScopeGuardBase safeConstruct(std::true_type, const void *) noexcept {
        return ScopeGuardBase{};
    }

    ScopeGuardImpl<std::reference_wrapper<FunctionType>, InvokeNoexcept> safeConstruct(std::false_type, FunctionType * f) noexcept {
        return ScopeGuardImpl<std::reference_wrapper<FunctionType>, InvokeNoexcept>(std::ref(*f));
    }

    void execute() noexcept(InvokeNoexcept) {
        if constexpr (InvokeNoexcept) {
            try {
                function_();
            }
            catch(...) {
                //TODO: warn();
                std::terminate();
            }
        }
        else {
            function_();
        }
    }

    void * operator new(std::size_t) = delete;

    FunctionType function_;
};

template <typename FunctionType, bool ExecuteOnException>
class ScopeGuardConditional
{
public:
    explicit ScopeGuardConditional(FunctionType const & fn) noexcept(std::is_nothrow_copy_constructible_v<FunctionType>)
        : guard_(fn)
    {
    }

    explicit ScopeGuardConditional(FunctionType && fn) noexcept(std::is_nothrow_move_constructible_v<FunctionType>)
        : guard_(std::move(fn))
    {
    }

    ScopeGuardConditional(ScopeGuardConditional &&) = default;

    ~ScopeGuardConditional() noexcept(ExecuteOnException) {
        if (ExecuteOnException != (exceptionCount_ < std::uncaught_exceptions())) {
            guard_.dismiss();
        }
    }

private:
    void * operator new(std::size_t) = delete;

    ScopeGuardImpl<FunctionType, ExecuteOnException> guard_;
    int const exceptionCount_ = std::uncaught_exceptions();
};

enum class ScopeGuardExit {};
template <typename F>
ScopeGuardImpl<std::decay_t<F>, true> operator+(ScopeGuardExit, F && f) noexcept(
        noexcept(ScopeGuardImpl<std::decay_t<F>, true>(std::forward<F>(f))))
{
    return ScopeGuardImpl<std::decay_t<F>, true>(std::forward<F>(f));
}

enum class ScopeGuardSuccess {};
template <typename F>
ScopeGuardConditional<std::decay_t<F>, false> operator+(ScopeGuardSuccess, F && f) noexcept(
        noexcept(ScopeGuardConditional<std::decay_t<F>, false>(std::forward<F>(f))))
{
    return ScopeGuardConditional<std::decay_t<F>, false>(std::forward<F>(f));
}

enum class ScopeGuardFail {};
template <typename F>
ScopeGuardConditional<std::decay_t<F>, true> operator+(ScopeGuardFail, F && f) noexcept(
        noexcept(ScopeGuardConditional<std::decay_t<F>, true>(std::forward<F>(f))))
{
    return ScopeGuardConditional<std::decay_t<F>, true>(std::forward<F>(f));
}
} // namespace detail

template <typename F>
detail::ScopeGuardImpl<std::decay_t<F>, true> makeGuard(F && f) {
    return detail::ScopeGuardImpl<std::decay_t<F>, true>(std::forward<F>(f));
}

} // namespace jd

#define CONCAT(_1, _2) _1##_2
#ifdef __COUNTER__
#define ANON_VARIABLE(str) CONCAT(str, __COUNTER__)
#else
#define ANON_VARIABLE(str) CONCAT(str, __LINE__)
#endif

#define SCOPE_EXIT                                \
    auto ANON_VARIABLE(SCOPE_EXIT_STATE) =        \
        jd::detail::ScopeGuardExit() + [&]()

#define SCOPE_SUCCESS                             \
    auto ANON_VARIABLE(SCOPE_SUCCESS_STATE) =     \
        jd::detail::ScopeGuardSuccess() + [&]()

#define SCOPE_FAIL                                \
    auto ANON_VARIABLE(SCOPE_FAIL_STATE) =        \
        jd::detail::ScopeGuardFail() + [&]()
