#pragma once

#define IMPL_CONSTRUCT(...) \
  { \
    static_assert(sizeof(impl_storage) >= sizeof(Impl), "impl_storage is too small"); \
    impl = std::construct_at(reinterpret_cast<Impl *>(impl_storage), __VA_ARGS__); \
  }

#define IMPL_COPY(other) \
  { \
    static_assert(sizeof(impl_storage) >= sizeof(Impl), "impl_storage is too small"); \
    impl = std::construct_at(reinterpret_cast<Impl *>(impl_storage), other); \
  }
