#ifndef SEAFILE_CLIENT_STL_EXTRA_H_
#define SEAFILE_CLIENT_STL_EXTRA_H_

#ifdef __cplusplus

#include <cstddef>
#include <memory>

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#ifndef __has_extension
#define __has_extension(x) 0
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifndef SEAFILE_GCC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#  define SEAFILE_GCC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ >= \
     ((maj) << 20) + ((min) << 10) + (patch))
# elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define SEAFILE_GCC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) >= ((maj) << 20) + ((min) << 10))
# else
#  define SEAFILE_GCC_PREREQ(maj, min, patch) 0
# endif
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103L)
#define SEAFILE_CXX11_MODE
#endif

#if __has_feature(cxx_constexpr) || defined(SEAFILE_CXX11_MODE)
#define SEAFILE_CONSTEXPR constexpr
#else
#define SEAFILE_CONSTEXPR
#endif

#if __has_feature(cxx_deleted_functions) || defined(SEAFILE_CXX11_MODE)
#define SEAFILE_DELETED = delete
#else
#define SEAFILE_DELETED
#endif

/// Find the length of an array.
template <class T, std::size_t N>
SEAFILE_CONSTEXPR inline size_t arrayLengthOf(T (&)[N]) {
    return N;
}

#ifdef SEAFILE_CXX11_MODE

/// similar to std::make_unique in c++14

template <class T, class... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/// similar to std::make_unique in c++14 for array

template <class T>
typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0,
                        std::unique_ptr<T>>::type
make_unique(size_t n) {
    return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]());
}

/// This function isn't used and is only here to provide better compile errors.
template <class T, class... Args>
typename std::enable_if<std::extent<T>::value != 0>::type
make_unique(Args &&...) SEAFILE_DELETED;

#endif // SEAFILE_CXX11_MODE

#endif // __cplusplus

#endif // SEAFILE_CLIENT_STL_EXTRA_H_
