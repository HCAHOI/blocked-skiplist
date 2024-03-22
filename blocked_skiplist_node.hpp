#pragma once


#include <cstdint>
#include <algorithm>
#include <optional>

#define SKIP_LIST_LEVELS 6
#define CACHELINE_SIZE 64

template<typename K, typename V>
struct Entry{
    K key;
    V val;

    Entry(): key{}, val{} {}
    Entry(K key, V value): key(key), val(value) {}

    Entry& operator=(const Entry& other) {
        key = other.key;
        val = other.val;
        return *this;
    }

    bool operator<(K other_key) const {
        return key < other_key;
    }

    bool operator<(const Entry &other) const {
        return key < other.key;
    }

    bool operator==(K other_key) const {
        return key == other_key;
    }

    bool operator==(const Entry &other) const {
        return key == other.key;    // Do not support multi-value for the same key.
    }
};

template<typename K, typename V>
struct Node {
    // Header zone
    K m_max_key;
    uint16_t size;  // Number of elements stored in this block.
    uint16_t capacity;  // Number of elements that can be stored in this block.
    Node *forward[SKIP_LIST_LEVELS];  // A fixed number of pointers for all levels.
    Node *prev;

    // Data zone
    Entry<K, V> *data;

    // Member functions
    explicit Node(uint64_t block_size = 256);
    ~Node();

    std::pair<K, V> min() const;
    std::pair<K, V> max() const;
    K min_key() const;
    K max_key() const;

    Entry<K, V>* insert(K key, V value);
    Entry<K, V>* insert(Entry<K, V> entry);
    std::optional<std::pair<K, V>> erase(K key);
    Entry<K, V>* find(K key) const;
    void split_into(Node *other);
};

template<typename K, typename V>
Entry<K, V>* Node<K, V>::find(K key) const {
    auto pos = std::lower_bound(data, data + size, key);
    if (pos == data + size || pos->key != key) {
        return nullptr;
    }
    return pos;
}

template<typename K, typename V>
Node<K, V>::Node(uint64_t block_size): m_max_key{}, size(0), capacity(block_size), prev(nullptr) {
    // align data to cache line
    data = (Entry<K, V> *) aligned_alloc(CACHELINE_SIZE, block_size * sizeof(Entry<K, V>));
    std::fill(data, data + capacity, Entry<K, V>());
    std::fill(forward, forward + SKIP_LIST_LEVELS, nullptr);
}

template<typename K, typename V>
Node<K, V>::~Node() {
    free(data);
}

template<typename K, typename V>
std::pair<K, V> Node<K, V>::min() const {
    return std::make_pair(data[0].key, data[0].val);
}

template<typename K, typename V>
std::pair<K, V> Node<K, V>::max() const {
    return std::make_pair(data[size - 1].key, data[size - 1].val);
}

template<typename K, typename V>
K Node<K, V>::min_key() const {
    return data[0].key;
}

template<typename K, typename V>
K Node<K, V>::max_key() const {
    return m_max_key;
}

template<typename K, typename V>
Entry<K, V>* Node<K, V>::insert(K key, V value) {
    // find the position to insert
    auto pos = std::upper_bound(data, data + size, Entry<K, V>(key, value));
    std::move_backward(pos, data + size, data + size + 1);
    *pos = Entry<K, V>(key, value);
    size++;
    m_max_key = std::max(m_max_key, key);

    return pos;
}

template<typename K, typename V>
std::optional<std::pair<K, V>> Node<K, V>::erase(K key) {
    auto pos = find(key);
    if (pos == nullptr) {
        return std::nullopt;
    }
    auto res = std::make_pair(pos->key, pos->val);
    std::move(pos + 1, data + size, pos);
    size--;
    m_max_key = data[size - 1].key;
    return res;
}

template<typename K, typename V>
Entry<K, V>* Node<K, V>::insert(Entry<K, V> entry) {
    // find the position to insert
    auto pos = std::upper_bound(data, data + size, entry);
    std::move_backward(pos, data + size, data + size + 1);
    *pos = entry;
    size++;
    m_max_key = std::max(m_max_key, pos->key);

    return pos;
}

// Move half of the elements from this block to the other block.
template<typename K, typename V>
void Node<K, V>::split_into(Node *other) {
    // move
    uint16_t half = size / 2;
    std::move(data + half, data + size, other->data);

    // update
    size -= half;
    other->size += half;
    other->m_max_key = other->data[other->size - 1].key;
    m_max_key = data[size - 1].key;
}

