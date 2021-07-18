#pragma once

#include <functional>
#include <cstring>

// NOTE: This class exists instead of using std::any because we need the ability to memcpy the raw data to another place
class Any {
public:
    template<typename T>
    Any(const T &value) {
        data = new T(value);
        size = sizeof(T);
        destroy = [](void *ptr) {
            auto value = reinterpret_cast<T *>(ptr);
            delete value;
        };
        copy = [](void *srcPtr) {
            auto value = reinterpret_cast<T *>(srcPtr);
            auto copied = new T(*value);
            return copied;
        };
    }

    Any() = default;

    Any(const Any &other) {
        data = other.copy(other.data);
        size = other.size;

        destroy = other.destroy;
        copy = other.copy;
    }

    Any(const Any &&other) noexcept {
        data = other.data;
        size = other.size;

        destroy = other.destroy;
        copy = other.copy;
    };

    Any(Any &&other) noexcept {
        data = other.data;
        size = other.size;

        destroy = other.destroy;
        copy = other.copy;

        other.data = nullptr;
        other.size = 0;
        other.destroy = {};
        other.copy = {};
    };

    ~Any() {
        reset();
    }

    template<typename T>
    T &get() { return *reinterpret_cast<T *>(data); }

    template<typename T>
    const T &get() const { return *reinterpret_cast<T *>(data); }

    const void *getRaw() const { return data; }

    size_t getSize() const { return size; }

    bool empty() const { return data == nullptr; }

    Any &operator=(const Any &other) {
        reset();

        data = other.copy(other.data);
        size = other.size;

        destroy = other.destroy;
        copy = other.copy;

        return *this;
    }

    void reset() {
        if (data) {
            destroy(data);
        }

        data = nullptr;
        size = 0;
        destroy = {};
        copy = {};
    }

private:
    void *data { nullptr };
    size_t size { 0 };

    std::function<void(void *)> destroy;
    std::function<void *(void *)> copy;
};