#pragma once

#include <unordered_map>
#include <typeindex>
#include <cstdint>
#include <cstddef>

namespace eerie_leap::configuration::cbor {

struct CborTraitFunctions {
    void* encode;
    void* decode;
    void* get_size;
};

class CborTraitRegistry {
public:
    template<typename T>
    using CborEncodeFn = int (*)(uint8_t*, size_t, const T*, size_t*);
    template<typename T>
    using CborDecodeFn = int (*)(const uint8_t*, size_t, T*, size_t*);
    template<typename T>
    using CborGetSerializingSizeFn = size_t (*)(const T&);

    template<typename T>
    static void Register(
        CborEncodeFn<T> encode,
        CborDecodeFn<T> decode,
        CborGetSerializingSizeFn<T> get_size)
    {
        registry_[std::type_index(typeid(T))] = {
            reinterpret_cast<void*>(encode),
            reinterpret_cast<void*>(decode),
            reinterpret_cast<void*>(get_size)
        };
    }

    template<typename T>
    static auto Get() {
        struct Result {
            CborEncodeFn<T> encode;
            CborDecodeFn<T> decode;
            CborGetSerializingSizeFn<T> get_size;
        };

        auto& funcs = registry_.at(std::type_index(typeid(T)));
        return Result{
            reinterpret_cast<CborEncodeFn<T>>(funcs.encode),
            reinterpret_cast<CborDecodeFn<T>>(funcs.decode),
            reinterpret_cast<CborGetSerializingSizeFn<T>>(funcs.get_size)
        };
    }

private:
    static inline std::unordered_map<std::type_index, CborTraitFunctions> registry_;
};

} // namespace eerie_leap::configuration::cbor
