#ifndef LRUCACHE_H
#define LRUCACHE_H
#include <QtGlobal>
#include <QList>
#include <QHash>
#include <QDateTime>

/*
 * Almost the same interface as QCache
 * Enjoy!
 * only remove timed out key-value pair in method ``contains``
 * object method wouldnot remove any key-value pair
 */
template<typename Key, typename T>
class LRUCache
{
    static inline uint current_time() { return QDateTime::currentDateTime().toTime_t(); }
    int ttl_;
    struct Node {
        Node() : keyPtr(0) { timeout = 0; }
        Node(T *data, int ttl)
            : keyPtr(0), t(data), p(0), n(0) { timeout = current_time() + ttl; }
        const Key *keyPtr;
        T *t;
        uint timeout;
        Node *p, *n;
    };
    Node *f, *l;
    QHash<Key, Node> hash_;

    inline void unlink(Node &n) {
        if (n.p) n.p->n = n.n;
        if (n.n) n.n->p = n.p;
        if (l == &n) l = n.p;
        if (f == &n) f = n.n;
        T *obj = n.t;
        hash_.remove(*n.keyPtr);
        delete obj;
    }
    inline Node *relink(const Key &key) {
        typename QHash<Key, Node>::iterator i = hash_.find(key);
        if (typename QHash<Key, Node>::const_iterator(i) == hash_.constEnd())
            return 0;

        Node &n = *i;
        if (f != &n) {
            if (n.p) n.p->n = n.n;
            if (n.n) n.n->p = n.p;
            if (l == &n) l = n.p;
            n.p = 0;
            n.n = f;
            f->p = &n;
            f = &n;
        }
        return &n;
    }

    Q_DISABLE_COPY(LRUCache)
public:
    inline explicit LRUCache(int ttl = 120);
    ~LRUCache() { clear(); }

    int ttl() const { return ttl_; }
    void setTtl(int ttl) { ttl_ = ttl; }

    // please don't use these four functions until you know what they mean
    int size() const { return hash_.size(); }
    int count() const { return hash_.size(); }
    bool isEmpty() const { return hash_.isEmpty(); }
    QList<Key> keys() const { return hash_.keys(); }

    inline void clear();

    inline bool insert(const Key &key, T* object);
    // object is the same with operator[]
    inline T* object(const Key &key);
    inline bool contains(const Key &key);
    // STL-style at method, with a bounding check
    inline T* at(const Key &key);
    inline T* operator[](const Key &key);

    inline bool remove(const Key &key);
    inline T* take(const Key &key);
};

template<typename Key, typename T>
LRUCache<Key, T>::LRUCache(int ttl)
  :ttl_(ttl), f(0), l(0)
{
}

template<typename Key, typename T>
void LRUCache<Key, T>::clear()
{
    while (f) { delete f->t; f = f->n; }
    hash_.clear();
    l = 0;
}

template <typename Key, typename T>
bool LRUCache<Key, T>::contains(const Key &key)
{
    if (!hash_.contains(key))
        return false;
    Node *pn = &hash_[key];
    if (pn->timeout <= current_time()) {
        unlink(*pn);
        return false;
    }
    return true;
}

template <typename Key, typename T>
T *LRUCache<Key, T>::at(const Key &key)
{
    assert(contains(key));
    return object(key);
}

template <typename Key, typename T>
T *LRUCache<Key, T>::object(const Key &key)
{
    Node *pn = this->relink(key);
    return pn->t;
}

template <typename Key, typename T>
T *LRUCache<Key, T>::operator[](const Key &key)
{
    return object(key);
}

template <typename Key, typename T>
bool LRUCache<Key, T>::remove(const Key &key)
{
    typename QHash<Key, Node>::iterator i = hash_.find(key);
    if (typename QHash<Key, Node>::const_iterator(i) == hash_.constEnd()) {
        return false;
    } else {
        unlink(*i);
        return true;
    }
}

template <typename Key, typename T>
T *LRUCache<Key, T>::take(const Key &key)
{
    typename QHash<Key, Node>::iterator i = hash_.find(key);
    if (i == hash_.end())
        return 0;

    Node &n = *i;
    if (n.timeout <= current_time()) {
        unlink(n);
        return NULL;
    }
    T *t = n.t;
    n.t = 0;
    unlink(n);
    return t;
}

template <typename Key, typename T>
bool LRUCache<Key, T>::insert(const Key &akey, T *aobject)
{
    remove(akey);
    Node sn(aobject, ttl_);
    typename QHash<Key, Node>::iterator i = hash_.insert(akey, sn);
    Node *n = &i.value();
    n->keyPtr = &i.key();
    if (f) f->p = n;
    n->n = f;
    f = n;
    if (!l) l = f;
    return true;
}


#endif // LRUCACHE_H
