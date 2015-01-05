#ifndef SEAFILE_CLIENT_UTILS_SINGLETON_H
#define SEAFILE_CLIENT_UTILS_SINGLETON_H

/**
 * This macro helps conveniently define singleton classes. Usage:
 *
 * // foo.h
 * #include "utils/singleton.h"
 * class Foo {
 *     SINGLETON_DEFINE(Foo)
 * private:
 *     Foo()
 *     ...
 * }

 * // foo.cpp
 * #include "foo.h"
 * SINGLETON_IMPL(Foo)
*/

#define SINGLETON_DEFINE(CLASS) \
    public: \
    static CLASS *instance(); \
    private: \
    static CLASS *singleton_;                   \

#define SINGLETON_IMPL(CLASS) \
    CLASS* CLASS::singleton_; \
    CLASS* CLASS::instance() { \
        if (singleton_ == NULL) { \
            singleton_ = new CLASS; \
        } \
        return singleton_; \
    }

#endif // SEAFILE_CLIENT_UTILS_SINGLETON_H
