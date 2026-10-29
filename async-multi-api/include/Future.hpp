#pragma once
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

    namespace detail {
        template <class T>
        struct State {
            std::mutex m;
            std::condition_variable cv;
            std::optional<T> value;
            std::exception_ptr error;
            bool done = false;
            std::vector<std::function<void()>> callbacks;
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
            lk.unlock();
            return value();
        }

        T value() const {
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

    private:
        std::shared_ptr<detail::State<T>> st_;
    };

    template <class T>
    class Promise {
    public:
        Promise() : st_(std::make_shared<detail::State<T>>()) {}
        Future<T> future() const {
            return Future<T>(st_);
        }
        void setValue(T v) const {
            complete([&] { st_->value.emplace(std::move(v)); });
        }
        void setError(std::exception_ptr e) const {
            complete([&] { st_->error = e; });
        }

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
    Future<std::decay_t<T>> ready(T&& v) {
        Promise<std::decay_t<T>> p;
        p.setValue(std::forward<T>(v));
        return p.future();
    }

    template <class T, class F>
    auto then(Future<T> prev, F f) {
        using R = std::invoke_result_t<F&, T>;
        if constexpr (is_future<R>::value) {
            using U = typename R::value_type;
            Promise<U> p;
            auto out = p.future();
            prev.subscribe([prev, p, f]() mutable {
                try {
                    R inner = f(prev.value());
                    inner.subscribe([inner, p] {
                        try {
                            p.setValue(inner.value());
                        }catch (...) {
                            p.setError(std::current_exception());
                        }
                    });
                } catch (...) {
                    p.setError(std::current_exception());
                }
            });
            return out;
        } else {
            Promise<R> p;
            auto out = p.future();
            prev.subscribe([prev, p, f]() mutable {
                try {
                    p.setValue(f(prev.value()));
                }catch (...) {
                    p.setError(std::current_exception());
                }
            });
            return out;
        }
    }

    template <class T, class F>
    Future<T> recover(Future<T> prev, F handler) {
        Promise<T> p;
        auto out = p.future();
        prev.subscribe([prev, p, handler]() mutable {
            try {
                p.setValue(prev.value());
            } catch (const std::exception& e) {
                try {
                    p.setValue(handler(e));
                }catch (...) {
                    p.setError(std::current_exception());
                }
            } catch (...) {
                p.setError(std::current_exception());
            }
        });
        return out;
    }

}