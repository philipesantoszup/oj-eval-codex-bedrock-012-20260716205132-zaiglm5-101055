/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
    class Key,
    class T,
    class Hash = std::hash<Key>, 
    class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
    typedef pair<const Key, T> value_type;
    
private:
    struct ListNode {
        ListNode *prev;
        ListNode *next;
        value_type *data;
        
        ListNode() : prev(nullptr), next(nullptr), data(nullptr) {}
        ListNode(value_type *d) : prev(nullptr), next(nullptr), data(d) {}
    };
    
    struct HashNode {
        ListNode *list_node;
        HashNode *next;
        
        HashNode(ListNode *ln) : list_node(ln), next(nullptr) {}
    };
    
    // Data members
    HashNode **buckets;
    size_t bucket_count_;
    size_t element_count_;
    float load_factor_;
    
    ListNode *head_; // dummy head for doubly-linked list
    ListNode *tail_; // dummy tail for doubly-linked list
    
    Hash hash_fn_;
    Equal equal_fn_;
    
    inline size_t get_bucket_index(const Key &key) const {
        return hash_fn_(key) % bucket_count_;
    }
    
    HashNode *find_hash_node(const Key &key) const {
        size_t idx = get_bucket_index(key);
        HashNode *node = buckets[idx];
        while (node) {
            if (equal_fn_(node->list_node->data->first, key)) {
                return node;
            }
            node = node->next;
        }
        return nullptr;
    }
    
    void resize() {
        size_t new_count = bucket_count_ * 2;
        HashNode **new_buckets = new HashNode*[new_count]();
        
        for (size_t i = 0; i < bucket_count_; ++i) {
            HashNode *node = buckets[i];
            while (node) {
                HashNode *next = node->next;
                size_t new_idx = hash_fn_(node->list_node->data->first) % new_count;
                node->next = new_buckets[new_idx];
                new_buckets[new_idx] = node;
                node = next;
            }
        }
        
        delete[] buckets;
        buckets = new_buckets;
        bucket_count_ = new_count;
    }
    
    void init() {
        bucket_count_ = 16;
        buckets = new HashNode*[bucket_count_]();
        element_count_ = 0;
        load_factor_ = 0.75f;
        
        head_ = new ListNode();
        tail_ = new ListNode();
        head_->next = tail_;
        tail_->prev = head_;
    }
    
    void destroy_contents() {
        // Delete all hash nodes
        for (size_t i = 0; i < bucket_count_; ++i) {
            HashNode *node = buckets[i];
            while (node) {
                HashNode *next = node->next;
                delete node;
                node = next;
            }
            buckets[i] = nullptr;
        }
        
        // Delete all list nodes and data
        ListNode *cur = head_->next;
        while (cur != tail_) {
            ListNode *next = cur->next;
            delete cur->data;
            delete cur;
            cur = next;
        }
        head_->next = tail_;
        tail_->prev = head_;
        element_count_ = 0;
    }
    
    void destroy() {
        destroy_contents();
        delete[] buckets;
        delete head_;
        delete tail_;
    }
    
    void copy_from(const linked_hashmap &other) {
        bucket_count_ = other.bucket_count_;
        buckets = new HashNode*[bucket_count_]();
        element_count_ = 0;
        load_factor_ = other.load_factor_;
        
        head_ = new ListNode();
        tail_ = new ListNode();
        head_->next = tail_;
        tail_->prev = head_;
        
        hash_fn_ = other.hash_fn_;
        equal_fn_ = other.equal_fn_;
        
        for (ListNode *cur = other.head_->next; cur != other.tail_; cur = cur->next) {
            insert(*cur->data);
        }
    }
    
public:
 
    class const_iterator;
    class iterator {
    private:
        linked_hashmap *map_;
        ListNode *node_;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::output_iterator_tag;

        iterator() : map_(nullptr), node_(nullptr) {}
        
        iterator(linked_hashmap *m, ListNode *n) : map_(m), node_(n) {}
        
        iterator(const iterator &other) : map_(other.map_), node_(other.node_) {}
        
        iterator operator++(int) {
            iterator tmp = *this;
            if (node_ == map_->tail_) {
                throw invalid_iterator();
            }
            node_ = node_->next;
            return tmp;
        }
        
        iterator & operator++() {
            if (node_ == map_->tail_) {
                throw invalid_iterator();
            }
            node_ = node_->next;
            return *this;
        }
        
        iterator operator--(int) {
            iterator tmp = *this;
            if (node_ == map_->head_->next) {
                throw invalid_iterator();
            }
            node_ = node_->prev;
            return tmp;
        }
        
        iterator & operator--() {
            if (node_ == map_->head_->next) {
                throw invalid_iterator();
            }
            node_ = node_->prev;
            return *this;
        }
        
        value_type & operator*() const {
            return *node_->data;
        }
        
        value_type* operator->() const noexcept {
            return node_->data;
        }
        
        bool operator==(const iterator &rhs) const {
            return node_ == rhs.node_;
        }
        
        bool operator==(const const_iterator &rhs) const {
            return node_ == rhs.node_;
        }
        
        bool operator!=(const iterator &rhs) const {
            return node_ != rhs.node_;
        }
        
        bool operator!=(const const_iterator &rhs) const {
            return node_ != rhs.node_;
        }
        
        friend class linked_hashmap;
        friend class const_iterator;
    };
 
    class const_iterator {
    private:
        const linked_hashmap *map_;
        const ListNode *node_;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::output_iterator_tag;
        
        const_iterator() : map_(nullptr), node_(nullptr) {}
        
        const_iterator(const linked_hashmap *m, const ListNode *n) : map_(m), node_(n) {}
        
        const_iterator(const const_iterator &other) : map_(other.map_), node_(other.node_) {}
        
        const_iterator(const iterator &other) : map_(other.map_), node_(other.node_) {}
        
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            if (node_ == map_->tail_) {
                throw invalid_iterator();
            }
            node_ = node_->next;
            return tmp;
        }
        
        const_iterator & operator++() {
            if (node_ == map_->tail_) {
                throw invalid_iterator();
            }
            node_ = node_->next;
            return *this;
        }
        
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            if (node_ == map_->head_->next) {
                throw invalid_iterator();
            }
            node_ = node_->prev;
            return tmp;
        }
        
        const_iterator & operator--() {
            if (node_ == map_->head_->next) {
                throw invalid_iterator();
            }
            node_ = node_->prev;
            return *this;
        }
        
        const value_type & operator*() const {
            return *node_->data;
        }
        
        const value_type* operator->() const noexcept {
            return node_->data;
        }
        
        bool operator==(const const_iterator &rhs) const {
            return node_ == rhs.node_;
        }
        
        bool operator==(const iterator &rhs) const {
            return node_ == rhs.node_;
        }
        
        bool operator!=(const const_iterator &rhs) const {
            return node_ != rhs.node_;
        }
        
        bool operator!=(const iterator &rhs) const {
            return node_ != rhs.node_;
        }
        
        friend class linked_hashmap;
        friend class iterator;
    };

    linked_hashmap() {
        init();
    }
    
    linked_hashmap(const linked_hashmap &other) {
        copy_from(other);
    }
 
    linked_hashmap & operator=(const linked_hashmap &other) {
        if (this != &other) {
            destroy();
            copy_from(other);
        }
        return *this;
    }
 
    ~linked_hashmap() {
        destroy();
    }
 
    T & at(const Key &key) {
        HashNode *hn = find_hash_node(key);
        if (!hn) {
            throw index_out_of_bound();
        }
        return hn->list_node->data->second;
    }
    
    const T & at(const Key &key) const {
        HashNode *hn = find_hash_node(key);
        if (!hn) {
            throw index_out_of_bound();
        }
        return hn->list_node->data->second;
    }
 
    T & operator[](const Key &key) {
        size_t idx = get_bucket_index(key);
        HashNode *node = buckets[idx];
        while (node) {
            if (equal_fn_(node->list_node->data->first, key)) {
                return node->list_node->data->second;
            }
            node = node->next;
        }
        
        // Insert new element
        value_type *data = new value_type(key, T());
        
        ListNode *ln = new ListNode(data);
        ln->prev = tail_->prev;
        ln->next = tail_;
        tail_->prev->next = ln;
        tail_->prev = ln;
        
        HashNode *new_hn = new HashNode(ln);
        new_hn->next = buckets[idx];
        buckets[idx] = new_hn;
        
        element_count_++;
        
        if ((float)element_count_ / bucket_count_ >= load_factor_) {
            resize();
        }
        
        return ln->data->second;
    }
 
    const T & operator[](const Key &key) const {
        return at(key);
    }
 
    iterator begin() {
        return iterator(this, head_->next);
    }
    
    const_iterator cbegin() const {
        return const_iterator(this, head_->next);
    }
 
    iterator end() {
        return iterator(this, tail_);
    }
    
    const_iterator cend() const {
        return const_iterator(this, tail_);
    }
 
    bool empty() const {
        return element_count_ == 0;
    }
 
    size_t size() const {
        return element_count_;
    }
 
    void clear() {
        destroy_contents();
    }
 
    pair<iterator, bool> insert(const value_type &value) {
        size_t idx = get_bucket_index(value.first);
        HashNode *node = buckets[idx];
        while (node) {
            if (equal_fn_(node->list_node->data->first, value.first)) {
                return pair<iterator, bool>(iterator(this, node->list_node), false);
            }
            node = node->next;
        }
        
        // Create new element
        value_type *data = new value_type(value);
        
        ListNode *ln = new ListNode(data);
        ln->prev = tail_->prev;
        ln->next = tail_;
        tail_->prev->next = ln;
        tail_->prev = ln;
        
        HashNode *new_hn = new HashNode(ln);
        new_hn->next = buckets[idx];
        buckets[idx] = new_hn;
        
        element_count_++;
        
        if ((float)element_count_ / bucket_count_ >= load_factor_) {
            resize();
        }
        
        return pair<iterator, bool>(iterator(this, ln), true);
    }
 
    void erase(iterator pos) {
        if (pos.map_ != this || pos.node_ == tail_ || pos.node_ == head_) {
            throw invalid_iterator();
        }
        
        ListNode *ln = pos.node_;
        
        // Remove from hash table
        size_t idx = get_bucket_index(ln->data->first);
        HashNode *hn = buckets[idx];
        HashNode *prev = nullptr;
        while (hn) {
            if (hn->list_node == ln) {
                if (prev) {
                    prev->next = hn->next;
                } else {
                    buckets[idx] = hn->next;
                }
                delete hn;
                break;
            }
            prev = hn;
            hn = hn->next;
        }
        
        // Remove from linked list
        ln->prev->next = ln->next;
        ln->next->prev = ln->prev;
        
        delete ln->data;
        delete ln;
        
        element_count_--;
    }
 
    size_t count(const Key &key) const {
        return find_hash_node(key) ? 1 : 0;
    }
 
    iterator find(const Key &key) {
        HashNode *hn = find_hash_node(key);
        if (!hn) {
            return end();
        }
        return iterator(this, hn->list_node);
    }
    
    const_iterator find(const Key &key) const {
        HashNode *hn = find_hash_node(key);
        if (!hn) {
            return cend();
        }
        return const_iterator(this, hn->list_node);
    }
};

}

#endif
