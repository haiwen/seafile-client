#include "stl.h"

namespace utils {

template class BasicBufferArray<char>;
template class BasicBufferArray<wchar_t>;

template <>
inline void swap(BasicBufferArray<char> &LHS,
                 BasicBufferArray<char> &RHS) UTILS_CXX11_NOEXCEPT;

template <>
inline void swap(BasicBufferArray<wchar_t> &LHS,
                 BasicBufferArray<wchar_t> &RHS) UTILS_CXX11_NOEXCEPT;

} // namespace utils
