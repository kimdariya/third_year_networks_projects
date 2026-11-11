#pragma once
#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace fut {

    class CancellationToken {
    public:
        CancellationToken() : cancelled_(std::make_shared<std::atomic<bool>>(false)) {}
        void cancel() { cancelled_->store(true, std::memory_order_release); }
        bool isCancelled() const { return cancelled_->load(std::memory_order_acquire); }
    private:
        std::shared_ptr<std::atomic<bool>> cancelled_;
    };

    namespace detail {
        template <class T>
        struct State {
            std::mutex m;
            std::condition_variable cv;
            std::optional<T> value;
            std::exception_ptr error;
            bool done = false;
            std::vector<std::function<void()>> callbacks;
            CancellationToken token;
        };
    }

    template <class T>
    class Future {
    public:
        using value_type = T;
        explicit Future(std::shared_ptr<detail::State<T>> st) : st_(std::move(st)) {}

        T get() const {
            std::unique_lock<std::mutex> lk(st_->m);
            st_->cv.wait(lk, [&] { return st_->done; });
            if (st_->error) {
                std::rethrow_exception(st_->error);
            }
            return *st_->value;
        }

        T value() const {
            std::lock_guard<std::mutex> lk(st_->m);
            if (st_->error) {
                std::rethrow_exception(st_->error);
            }
            return *st_->value;
        }

        void subscribe(std::function<void()> cb) const {
            {
                std::lock_guard<std::mutex> lk(st_->m);
                if (!st_->done) {
                    st_->callbacks.push_back(std::move(cb));
                    return;
                }
            }
            cb();
        }

        CancellationToken token() const { return st_->token; }

        void cancel() const { st_->token.cancel(); }

    private:
        std::shared_ptr<detail::State<T>> st_;
    };

    template <class T>
    class Promise {
    public:
        Promise() : st_(std::make_shared<detail::State<T>>()) {}
        explicit Promise(CancellationToken token) : st_(std::make_shared<detail::State<T>>()) {
            st_->token = std::move(token);
        }

        Future<T> future() const {
            return Future<T>(st_);
        }

        CancellationToken token() const { return st_->token; }

        void setValue(T v) const {
            complete([&] { st_->value.emplace(std::move(v)); });
        }
        void setError(std::exception_ptr e) const {
            complete([&] { st_->error = e; });
        }

        bool isCancelled() const { return st_->token.isCancelled(); }

    private:
        template <class F>
        void complete(F fill) const {
            std::vector<std::function<void()>> cbs;
            {
                std::lock_guard<std::mutex> lk(st_->m);
                if (st_->done) return;
                fill();
                st_->done = true;
                cbs.swap(st_->callbacks);
            }
            st_->cv.notify_all();
            for (auto& cb : cbs) {
                cb();
            }
        }
        std::shared_ptr<detail::State<T>> st_;
    };

    template <class> struct is_future : std::false_type {};
    template <class T> struct is_future<Future<T>> : std::true_type {};

    template <class T>
    Future<std::decay_t<T>> ready(T&& v, CancellationToken token = {}) {
        Promise<std::decay_t<T>> p(std::move(token));
        p.setValue(std::forward<T>(v));
        return p.future();
    }

    template <class T, class F>
    auto then(Future<T> prev, F f) {
        using R = std::invoke_result_t<F&, T>;
        if constexpr (is_future<R>::value) {
            using U = typename R::value_type;
            Promise<U> p(prev.token());
            auto out = p.future();
            prev.subscribe([prev, p, f]() mutable {
                if (prev.token().isCancelled()) {
                    p.setError(std::make_exception_ptr(std::runtime_error("operation cancelled")));
                    return;
                }
                try {
                    R inner = f(prev.value());
                    inner.subscribe([inner, p] {
                        try {
                            if (p.isCancelled()) {
                                p.setError(std::make_exception_ptr(std::runtime_error("operation cancelled")));
                                return;
                            }
                            p.setValue(inner.value());
                        } catch (...) {
                            p.setError(std::current_exception());
                        }
                    });
                } catch (...) {
                    p.setError(std::current_exception());
                }
            });
            return out;
        } else {
            Promise<R> p(prev.token());
            auto out = p.future();
            prev.subscribe([prev, p, f]() mutable {
                if (prev.token().isCancelled()) {
                    p.setError(std::make_exception_ptr(std::runtime_error("operation cancelled")));
                    return;
                }
                try {
                    p.setValue(f(prev.value()));
                } catch (...) {
                    p.setError(std::current_exception());
                }
            });
            return out;
        }
    }

    template <class T, class F>
    Future<T> recover(Future<T> prev, F handler) {
        Promise<T> p(prev.token());
        auto out = p.future();
        prev.subscribe([prev, p, handler]() mutable {
            if (prev.token().isCancelled()) {
                p.setError(std::make_exception_ptr(std::runtime_error("operation cancelled")));
                return;
            }
            try {
                p.setValue(prev.value());
            } catch (const std::exception& e) {
                try {
                    p.setValue(handler(e));
                } catch (...) {
                    p.setError(std::current_exception());
                }
            } catch (...) {
                p.setError(std::current_exception());
            }
        });
        return out;
    }

}