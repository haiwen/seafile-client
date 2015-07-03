#ifndef SEAFILE_CLIENT_UTILS_STL_H_
#define SEAFILE_CLIENT_UTILS_STL_H_

#include <utility>
#include <cassert>
#include <string>
#include <cstring>
#include <cwchar>
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103L)
#define UTILS_CXX11_MODE
#define UTILS_CXX11_DELETE = delete
#define UTILS_CXX11_NOEXCEPT noexcept
#else
#define UTILS_CXX11_DELETE
#define UTILS_CXX11_NOEXCEPT
#endif

namespace utils {
template <typename T = char> class BasicBufferArray {
    typedef T char_type;
    char_type *data_;
    size_t size_;
    size_t capacity_;

    BasicBufferArray(const BasicBufferArray &buffer) UTILS_CXX11_DELETE;
    BasicBufferArray &
    operator=(const BasicBufferArray &buffer) UTILS_CXX11_DELETE;

  public:
    void swap(BasicBufferArray &RHS) UTILS_CXX11_NOEXCEPT {
        std::swap(data_, RHS.data_);
        std::swap(size_, RHS.size_);
        std::swap(capacity_, RHS.capacity_);
    }
#ifdef UTILS_CXX11_MODE
    BasicBufferArray(BasicBufferArray &&RHS) UTILS_CXX11_NOEXCEPT
        : data_(RHS.data_),
          size_(RHS.size_),
          capacity_(RHS.capacity_) {
        RHS.data_ = NULL;
        RHS.size_ = 0;
        RHS.capacity_ = 0;
    }
    BasicBufferArray &operator=(BasicBufferArray &&RHS) UTILS_CXX11_NOEXCEPT {
        std::swap(data_, RHS.data_);
        std::swap(size_, RHS.size_);
        std::swap(capacity_, RHS.capacity_);
        return *this;
    }
#endif // UTILS_CXX11_MODE

    ~BasicBufferArray() {
        if (capacity_ && data_)
            delete[] data_;
    }
    BasicBufferArray() : data_(NULL), size_(0), capacity_(0){};
    BasicBufferArray(const char_type *buffer) {
        const char_type *end = buffer;
        while (*end++ != '\0')
            ;
        capacity_ = size_ = end - buffer;

        data_ = new char_type[size_];
        assert(data_ != NULL);
        memcpy(data_, buffer, size_ * sizeof(char_type));
    }
    BasicBufferArray(const char_type *buffer, size_t size) {
        data_ = new char_type[size];
        assert(data_ != NULL);
        capacity_ = size_ = size;
        memcpy(data_, buffer, size * sizeof(char_type));
    }
    BasicBufferArray(const std::basic_string<char_type> &string) {
        if (string.size() == 0) {
            data_ = NULL;
            capacity_ = size_ = 0;
            return;
        }
        capacity_ = size_ = string.size() + 1;
        data_ = new char_type[size_];
        assert(data_ != NULL);
        memcpy(data_, string.data(), size_ * sizeof(char_type));
        data_[size_ - 1] = '\0';
    }

    char_type *data() { return data_; }
    const char_type *data() const { return data_; }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    void shrink_to_fit() {
        if (size_ == capacity_)
            return;
        char_type *new_data = NULL;
        if (size_ != 0) {
            char_type *new_data = new char_type[size_];
            assert(new_data != NULL);
            memcpy(new_data, data_, size_ * sizeof(char_type));
        }
        delete[] data_;
        capacity_ = size_;
        data_ = new_data;
    }
    void reserve(size_t new_capacity) {
        if (new_capacity <= capacity_)
            return;
        char_type *new_data = new char_type[new_capacity];
        assert(new_data != NULL);
        if (data_) {
            memcpy(new_data, data_, size_ * sizeof(char_type));
            delete[] data_;
        }
        capacity_ = new_capacity;
        data_ = new_data;
    }
    void resize(size_t new_size) {
        if (new_size <= capacity_) {
            size_ = new_size;
            return;
        }
        reserve(new_size);
        size_ = new_size;
    };

    char_type &operator[](size_t pos) { return *(data() + pos); }

    const char_type &operator[](size_t pos) const { return *(data() + pos); }
};

template <typename T>
inline void swap(BasicBufferArray<T> &LHS,
                 BasicBufferArray<T> &RHS) UTILS_CXX11_NOEXCEPT {
    LHS.swap(RHS);
}

extern template class BasicBufferArray<char>;
extern template class BasicBufferArray<wchar_t>;
typedef BasicBufferArray<char> BufferArray;
typedef BasicBufferArray<wchar_t> WBufferArray;

} // namespace utils
#endif // SEAFILE_CLIENT_UTILS_STL_H_
