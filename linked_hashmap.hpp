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
    size_t bucket_count;
    size_t element_count;
    float load_factor;
    
    ListNode *head; // dummy head for doubly-linked list
    ListNode *tail; // dummy tail for doubly-linked list
    
    Hash hash_fn;
    Equal equal_fn;
    
    size_t get_bucket_index(const Key &key) const {
        return hash_fn(key) % bucket_count;
    }
    
    HashNode *find_hash_node(const Key &key) const {
        size_t idx = get_bucket_index(key);
        HashNode *node = buckets[idx];
        while (node) {
            if (equal_fn(node->list_node->data->first, key)) {
                return node;
            }
            node = node->next;
        }
        return nullptr;
    }
    
    void resize() {
        size_t new_count = bucket_count * 2;
        HashNode **new_buckets = new HashNode*[new_count]();
        
        for (size_t i = 0; i < bucket_count; ++i) {
            HashNode *node = buckets[i];
            while (node) {
                HashNode *next = node->next;
                size_t new_idx = hash_fn(node->list_node->data->first) % new_count;
                node->next = new_buckets[new_idx];
                new_buckets[new_idx] = node;
                node = next;
            }
        }
        
        delete[] buckets;
        buckets = new_buckets;
        bucket_count = new_count;
    }
    
    void init() {
        bucket_count = 16;
        buckets = new HashNode*[bucket_count]();
        element_count = 0;
        load_factor = 0.75f;
        
        head = new ListNode();
        tail = new ListNode();
        head->next = tail;
        tail->prev = head;
    }
    
    void destroy_list() {
        ListNode *cur = head->next;
        while (cur != tail) {
            ListNode *next = cur->next;
            delete cur->data;
            delete cur;
            cur = next;
        }
        head->next = tail;
        tail->prev = head;
    }
    
    void destroy() {
        // Delete all hash nodes
        for (size_t i = 0; i < bucket_count; ++i) {
            HashNode *node = buckets[i];
            while (node) {
                HashNode *next = node->next;
                delete node;
                node = next;
            }
        }
        delete[] buckets;
        
        // Delete all list nodes and data
        destroy_list();
        delete head;
        delete tail;
    }
    
    void copy_from(const linked_hashmap &other) {
        bucket_count = other.bucket_count;
        buckets = new HashNode*[bucket_count]();
        element_count = 0;
        load_factor = other.load_factor;
        
        head = new ListNode();
        tail = new ListNode();
        head->next = tail;
        tail->prev = head;
        
        for (ListNode *cur = other.head->next; cur != other.tail; cur = cur->next) {
            insert(*cur->data);
        }
    }
    
public:
 
    class const_iterator;
    class iterator {
    private:
        linked_hashmap *map;
        ListNode *node;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::output_iterator_tag;

        iterator() : map(nullptr), node(nullptr) {}
        
        iterator(linked_hashmap *m, ListNode *n) : map(m), node(n) {}
        
        iterator(const iterator &other) : map(other.map), node(other.node) {}
        
        iterator operator++(int) {
            iterator tmp = *this;
            if (node == map->tail) {
                throw invalid_iterator();
            }
            node = node->next;
            return tmp;
        }
        
        iterator & operator++() {
            if (node == map->tail) {
                throw invalid_iterator();
            }
            node = node->next;
            return *this;
        }
        
        iterator operator--(int) {
            iterator tmp = *this;
            if (node == map->head->next) {
                throw invalid_iterator();
            }
            node = node->prev;
            return tmp;
        }
        
        iterator & operator--() {
            if (node == map->head->next) {
                throw invalid_iterator();
            }
            node = node->prev;
            return *this;
        }
        
        value_type & operator*() const {
            return *node->data;
        }
        
        value_type* operator->() const noexcept {
            return node->data;
        }
        
        bool operator==(const iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator!=(const iterator &rhs) const {
            return node != rhs.node;
        }
        
        bool operator!=(const const_iterator &rhs) const {
            return node != rhs.node;
        }
        
        friend class linked_hashmap;
        friend class const_iterator;
    };
 
    class const_iterator {
    private:
        const linked_hashmap *map;
        const ListNode *node;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::output_iterator_tag;
        
        const_iterator() : map(nullptr), node(nullptr) {}
        
        const_iterator(const linked_hashmap *m, const ListNode *n) : map(m), node(n) {}
        
        const_iterator(const const_iterator &other) : map(other.map), node(other.node) {}
        
        const_iterator(const iterator &other) : map(other.map), node(other.node) {}
        
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            if (node == map->tail) {
                throw invalid_iterator();
            }
            node = node->next;
            return tmp;
        }
        
        const_iterator & operator++() {
            if (node == map->tail) {
                throw invalid_iterator();
            }
            node = node->next;
            return *this;
        }
        
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            if (node == map->head->next) {
                throw invalid_iterator();
            }
            node = node->prev;
            return tmp;
        }
        
        const_iterator & operator--() {
            if (node == map->head->next) {
                throw invalid_iterator();
            }
            node = node->prev;
            return *this;
        }
        
        const value_type & operator*() const {
            return *node->data;
        }
        
        const value_type* operator->() const noexcept {
            return node->data;
        }
        
        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator==(const iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator!=(const const_iterator &rhs) const {
            return node != rhs.node;
        }
        
        bool operator!=(const iterator &rhs) const {
            return node != rhs.node;
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
        HashNode *hn = find_hash_node(key);
        if (hn) {
            return hn->list_node->data->second;
        }
        
        // Insert new element
        value_type *data = new value_type(key, T());
        
        ListNode *ln = new ListNode(data);
        ln->prev = tail->prev;
        ln->next = tail;
        tail->prev->next = ln;
        tail->prev = ln;
        
        size_t idx = get_bucket_index(key);
        HashNode *new_hn = new HashNode(ln);
        new_hn->next = buckets[idx];
        buckets[idx] = new_hn;
        
        element_count++;
        
        if ((float)element_count / bucket_count >= load_factor) {
            resize();
        }
        
        return ln->data->second;
    }
 
    const T & operator[](const Key &key) const {
        return at(key);
    }
 
    iterator begin() {
        return iterator(this, head->next);
    }
    
    const_iterator cbegin() const {
        return const_iterator(this, head->next);
    }
 
    iterator end() {
        return iterator(this, tail);
    }
    
    const_iterator cend() const {
        return const_iterator(this, tail);
    }
 
    bool empty() const {
        return element_count == 0;
    }
 
    size_t size() const {
        return element_count;
    }
 
    void clear() {
        for (size_t i = 0; i < bucket_count; ++i) {
            HashNode *node = buckets[i];
            while (node) {
                HashNode *next = node->next;
                delete node;
                node = next;
            }
            buckets[i] = nullptr;
        }
        destroy_list();
        element_count = 0;
    }
 
    pair<iterator, bool> insert(const value_type &value) {
        HashNode *hn = find_hash_node(value.first);
        if (hn) {
            return pair<iterator, bool>(iterator(this, hn->list_node), false);
        }
        
        // Create new element
        value_type *data = new value_type(value);
        
        ListNode *ln = new ListNode(data);
        ln->prev = tail->prev;
        ln->next = tail;
        tail->prev->next = ln;
        tail->prev = ln;
        
        size_t idx = get_bucket_index(value.first);
        HashNode *new_hn = new HashNode(ln);
        new_hn->next = buckets[idx];
        buckets[idx] = new_hn;
        
        element_count++;
        
        if ((float)element_count / bucket_count >= load_factor) {
            resize();
        }
        
        return pair<iterator, bool>(iterator(this, ln), true);
    }
 
    void erase(iterator pos) {
        if (pos.map != this || pos.node == tail || pos.node == head) {
            throw invalid_iterator();
        }
        
        ListNode *ln = pos.node;
        
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
        
        element_count--;
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
