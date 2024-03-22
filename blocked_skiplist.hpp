#pragma once

#include "blocked_skiplist_node.hpp"
#include <random>
#include <memory>
#include <cassert>
#include <stdexcept>
#include <iostream>
#include <concepts>

#define CACHELINE_SIZE 64
#define NODE_LOWER_BOUND 0.45

// pre-declaration
template<typename K, typename V>
struct BlockedSkipListIterator;

template<typename K, typename V>
struct BlockedSkipList {
// Member variables
    Node<K, V> *head;
public:
    explicit BlockedSkipList();
    explicit BlockedSkipList(size_t block_size);
    ~BlockedSkipList();

    BlockedSkipList(const BlockedSkipList& other) requires(std::copyable<K> && std::copyable<V>);
    BlockedSkipList& operator=(const BlockedSkipList& other) requires(std::copyable<K> && std::copyable<V>);

    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;

    BlockedSkipListIterator<K, V> begin() const;
    BlockedSkipListIterator<K, V> end() const;
    BlockedSkipListIterator<K, V> rbegin() const;
    BlockedSkipListIterator<K, V> rend() const;

    BlockedSkipListIterator<K, V> find(K key) const;
    BlockedSkipListIterator<K, V> insert(Entry<K, V> entry);
    BlockedSkipListIterator<K, V> insert(K key, V value);
    BlockedSkipListIterator<K, V> update(Entry<K, V> entry);
    BlockedSkipListIterator<K, V> update(K key, V value);
    std::optional<std::pair<K, V>> erase(K key);
    void clear();

    void merge(BlockedSkipList<K, V>& other);
    std::pair<BlockedSkipList<K, V>, BlockedSkipList<K, V>> split(K key);
    std::pair<BlockedSkipList<K, V>, BlockedSkipList<K, V>> split(BlockedSkipListIterator<K, V> iter);

    void print() const;

    V& operator[](K key);

private:
    // Member functions
    Node<K, V> *find_node(Node<K, V> *cur_block, K key, Node<K, V> *level_lower_bound[SKIP_LIST_LEVELS]) const;
    void merge_node(Node<K, V>* node);
    void balance_block(Node<K, V> *node);
    [[nodiscard]] size_t get_random_level() const;
    [[nodiscard]] size_t get_node_lower_bound() const;

    size_t m_size;
    size_t block_size;
    const float p = 0.5;    // probability of a node having a level
    static std::mt19937 level_generator;
};

template<typename K, typename V>
std::mt19937 BlockedSkipList<K, V>::level_generator = std::mt19937(std::random_device{}());

template<typename K, typename V>
struct BlockedSkipListIterator {
    Node<K, V> *node;
    size_t index;
    bool forward = true;

    BlockedSkipListIterator() = default;
    BlockedSkipListIterator(Node<K, V> *node, size_t index): node(node), index(index) {}
    BlockedSkipListIterator(Node<K, V> *node, size_t index, bool forward): node(node), index(index), forward(forward) {}

    void change_direction() {
        forward = !forward;
    }

    bool operator==(const BlockedSkipListIterator &other) const {
        return node == other.node && index == other.index;
    }

    bool operator!=(const BlockedSkipListIterator &other) const {
        return !(*this == other);
    }

    BlockedSkipListIterator& operator++() {
        if (node != nullptr) {
            if(forward) {
                index += 1;
                if (index >= node->size) {
                    node = node->forward[0];
                    index = 0;
                }
            } else {
                if (index == 0) {
                    node = node->prev;
                    if (node != nullptr) {
                        index = node->size - 1;
                    }
                } else {
                    index -= 1;
                }
            }
        }
        return *this;
    }

    BlockedSkipListIterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    BlockedSkipListIterator& operator+(uint64_t n) {
        while (n > 0) {
            ++(*this);
            n--;
        }
        return *this;
    }

    Entry<K, V>& operator*() {
        return node->data[index];
    }

    Entry<K, V>* operator->() {
        return &node->data[index];
    }
};

// Template Class
template<typename K, typename V>
BlockedSkipList<K, V>::BlockedSkipList(): block_size(256) {
    // check if the block_size is the power of 2
    if ((block_size & (block_size - 1)) != 0) {
        throw std::runtime_error("Block m_size must be a power of 2");
    }
    head = new Node<K, V>(block_size);
}

template<typename K, typename V>
BlockedSkipList<K, V>::BlockedSkipList(size_t block_size): block_size(block_size) {
    // check if the block_size is the power of 2
    if ((block_size & (block_size - 1)) != 0) {
        throw std::runtime_error("Block m_size must be a power of 2");
    }
    head = new Node<K, V>(block_size);
}

template<typename K, typename V>
BlockedSkipList<K, V>::~BlockedSkipList() {
    auto cur = head;
    while (cur != nullptr) {
        auto next = cur->forward[0];
        delete cur;
        cur = next;
    }
}

template<typename K, typename V>
BlockedSkipList<K, V>::BlockedSkipList(const BlockedSkipList& other) requires(std::copyable<K> && std::copyable<V>) {
    block_size = other.block_size;
    head = new Node<K, V>(block_size);
    m_size = 0;
    for (auto it = other.begin(); it != other.end(); ++it) {
        insert(*it);
    }
}

template<typename K, typename V>
BlockedSkipList<K, V>& BlockedSkipList<K, V>::operator=(const BlockedSkipList& other) requires(std::copyable<K> && std::copyable<V>) {
    if (this != &other) {
        clear();
        block_size = other.block_size;
        head = new Node<K, V>(block_size);
        m_size = 0;
        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(*it);
        }
    }
    return *this;
}

template<typename K, typename V>
size_t BlockedSkipList<K, V>::size() const {
    return size;
}

template<typename K, typename V>
bool BlockedSkipList<K, V>::empty() const {
    return m_size == 0;
}

template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::begin() const {
    return BlockedSkipListIterator<K, V>(head, 0);
}

template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::end() const {
    return BlockedSkipListIterator<K, V>(nullptr, 0);
}

template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::rbegin() const {
    auto it = begin();
    while (it.node->forward[0] != nullptr) {
        it.node = it.node->forward[0];
    }
    it.index = it.node->size - 1;
    it.change_direction();
    return it;
}

template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::rend() const {
    return BlockedSkipListIterator<K, V>(nullptr, 0, false);
}

template<typename K, typename V>
size_t BlockedSkipList<K, V>::get_random_level() const {
    std::uniform_real_distribution<double> d(0.0, 1.0);
    auto level = 1;
    while (d(level_generator) < p && level < SKIP_LIST_LEVELS) {
        level += 1;
    }
    return level;
}

template<typename K, typename V>
V& BlockedSkipList<K, V>::operator[](K key) {
    auto entry = find(key);
    if (entry != end()) {
        return (*entry).val;
    } else {
        throw std::runtime_error("Key not found");
    }
}

/// @return the iterator to the inserted element
template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::insert(Entry<K, V> entry) {
    Node<K, V> *blocks_per_level[SKIP_LIST_LEVELS];
    auto target_node = find_node(head, entry.key, blocks_per_level);

    // The node is full
    if (target_node->size == block_size) {
        auto *new_node = new Node<K, V>();
        target_node->split_into(new_node);

        // Insert new target_node into the skip list at level 0.
        new_node->forward[0] = target_node->forward[0];
        new_node->prev = target_node;
        if (new_node->forward[0] != nullptr) {
            new_node->forward[0]->prev = new_node;
        }
        target_node->forward[0] = new_node;

        // Update skip list on all levels except 0.
        auto height = get_random_level();
        for (uint l = 1; l < SKIP_LIST_LEVELS; l++) {
            if (l < height) {
                if (blocks_per_level[l]->forward[l] != target_node) {
                    new_node->forward[l] = blocks_per_level[l]->forward[l];
                    blocks_per_level[l]->forward[l] = new_node;
                } else {
                    new_node->forward[l] = target_node->forward[l];
                    target_node->forward[l] = new_node;
                }
                blocks_per_level[l] = new_node;
            } else {
                new_node->forward[l] = nullptr;
            }
        }

        // Recall
        return insert(entry);
    } else {
        auto iter = target_node->insert(entry);
        balance_block(target_node);
        m_size += 1;
        return BlockedSkipListIterator<K, V>(target_node, iter - target_node->data);
    }
}

/// @return the iterator to the inserted element
template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::insert(K key, V value) {
    return insert(Entry<K, V>(key, value));
}

/// Update the value of the key if it exists, otherwise insert the key-value pair.
/// @return the iterator to the inserted element
template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::update(Entry<K, V> entry) {
    auto iter = find(entry.key);
    if (iter != end()) {
        (*iter).val = entry.val;
        return iter;
    } else {
        return insert(entry);
    }
}

/// Update the value of the key if it exists, otherwise insert the key-value pair.
/// @return the iterator to the inserted element
template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::update(K key, V value) {
    return update(Entry<K, V>(key, value));
}

/// @return the erased key-value pair, otherwise return end()
template<typename K, typename V>
std::optional<std::pair<K, V>> BlockedSkipList<K, V>::erase(K key) {
    Node<K, V> *blocks_per_level[SKIP_LIST_LEVELS];
    auto target_node = find_node(head, key, blocks_per_level);
    auto entry = target_node->erase(key);
    if (entry.has_value()) {
        balance_block(target_node);
        m_size -= 1;
    }
    return entry;
}

template<typename K, typename V>
void BlockedSkipList<K, V>::clear() {
    auto cur = head;
    while (cur != nullptr) {
        auto next = cur->forward[0];
        delete cur;
        cur = next;
    }
    head = nullptr;
    m_size = 0;
}

template<typename K, typename V>
void BlockedSkipList<K, V>::merge(BlockedSkipList<K, V>& other) {
    for (auto it = other.begin(); it != other.end(); ++it) {
        insert(*it);
    }
    other.clear();
}

template<typename K, typename V>
Node<K, V>* BlockedSkipList<K, V>::find_node(Node<K, V> *cur_block, K key, Node<K, V> *level_lower_bound[SKIP_LIST_LEVELS]) const {
    for (int l = SKIP_LIST_LEVELS - 1; 0 <= l; l--) {
        while (cur_block->forward[l] != nullptr && cur_block->forward[l]->max_key() < key &&
               cur_block->forward[l]->forward[0] != nullptr) {
            cur_block = cur_block->forward[l];
        }
        level_lower_bound[l] = cur_block;
    }
    return level_lower_bound[0]->forward[0] != nullptr && level_lower_bound[0]->max_key() < key ? level_lower_bound[0]->forward[0] : level_lower_bound[0];
}

template<typename K, typename V>
BlockedSkipListIterator<K, V> BlockedSkipList<K, V>::find(K key) const {
    if (head != nullptr) {
        Node<K, V> *blocks[SKIP_LIST_LEVELS];
        auto block = find_node(head, key, blocks);
        auto entry = block->find(key);
        if (entry != nullptr) {
            return BlockedSkipListIterator<K, V>(block, entry - block->data);
        }
    }
    return end();
}

template<typename K, typename V>
size_t BlockedSkipList<K, V>::get_node_lower_bound() const {
    return static_cast<size_t>(NODE_LOWER_BOUND * block_size);
}

template<typename K, typename V>
void BlockedSkipList<K, V>::balance_block(Node<K, V> *node) {
    // To few elements
    if (node->size < get_node_lower_bound()) {
        auto prev_node = node->prev;
        auto next_node = node->forward[0];

        if (prev_node == nullptr && next_node == nullptr) {
            return;
        }

        auto another = next_node;
        if (prev_node != nullptr && (next_node == nullptr || prev_node->size >= next_node->size)) {
            another = prev_node;
        }

        if (another->size + node->size <= block_size) {
            merge_node(node);
        } else {
            // Inference:
            // another->m_size + node->m_size > block_size
            // node->m_size < get_node_lower_bound() = 0.45 * block_size
            // another->m_size > node->m_size

            auto size_after_balance = (another->size + node->size) / 2;
            auto size_to_move = another->size - size_after_balance;

            if (another == next_node) {
                // 1. Copy the elements to move in `another` to `node`.
                std::copy(another->data, another->data + size_to_move, node->data + node->size);
                // 2. Move the rest of the elements in `another` to the beginning.
                std::move(another->data + size_to_move, another->data + another->size, another->data);
                // 3. Update the m_size of `node` and `another`.
                node->size += size_to_move;
                another->size -= size_to_move;
                // 4. Update the max_key
                node->m_max_key = node->data[node->size - 1].key;
            } else {
                // 1. Move the elements in `node` backward to make space for the elements in `another`.
                std::move_backward(node->data, node->data + node->size, node->data + node->size + size_to_move);
                // 2. Copy the elements in `another` to `node`.
                std::copy(another->data + another->size - size_to_move, another->data + another->size, node->data);
                // 3. Update the m_size of `node` and `another`.
                node->size += size_to_move;
                another->size -= size_to_move;
                // 4. Update the max_key
                another->m_max_key = another->data[another->size - 1].key;
            }
        }
    }
}


template<typename K, typename V>
void BlockedSkipList<K, V>::merge_node(Node<K, V> *node) {
    auto prev_node = node->prev;
    auto next_node = node->forward[0];

    if (node == head) {  // The key node
        if(next_node != nullptr) {
            merge_node(next_node);
        } else {
            return;
        }
    } else if (next_node == nullptr) {
        // When merging the last node, we instead merge the node prev it into the last node.
        if (prev_node == head) {
            // merge the last node into the head
            // 1. Move all elements from `node` to `prev_node`.
            std::move(node->data, node->data + node->size, prev_node->data + prev_node->size);
            // 3. Update the m_size of `prev_node`.
            prev_node->size += node->size;
            // 4. Update the max_key
            prev_node->m_max_key = prev_node->data[prev_node->size - 1].key;

            // clear the head
            for (auto l = 0; l < SKIP_LIST_LEVELS; l++) {
                head->forward[l] = nullptr;
            }

            delete node;
        }
        merge_node(prev_node);
    } else {  // We are dealing with a middle node, we merge it into the smaller neighbour.
        Node<K, V> *predecessors[SKIP_LIST_LEVELS];
        // Find predecessors, that needs to happen prev moving the elements
        find_node(head, node->min_key(), predecessors);

        // First, we move all elements out of the node in question.
        if (next_node->size < prev_node->size) {
            assert(next_node->size + node->size <= block_size && "The caller ensures this node can be merged.");
            // 1. Move all elements in `next_node` backward to make space for the elements in `node`.
            std::move_backward(next_node->data, next_node->data + next_node->size, next_node->data + node->size);
            // 2. Move all elements from `node` to `next_node`.
            std::move(node->data, node->data + node->size, next_node->data);
            // 3. Update the m_size of `next_node`.
            next_node->size += node->size;
        } else {
            assert(prev_node->size + node->size <= block_size && "The caller ensures this node can be merged.");
            // 1. Move all elements from `node` to `prev_node`.
            std::move(node->data, node->data + node->size, prev_node->data + prev_node->size);
            // 3. Update the m_size of `prev_node`.
            prev_node->size += node->size;
            // 4. Update the max_key
            prev_node->m_max_key = prev_node->data[prev_node->size - 1].key;
        }

        // Second, we remove it from the skiplist.
        for (auto l = 0; l < SKIP_LIST_LEVELS; l++) {
            if (predecessors[l]->forward[l] != node) {
                break;
            }
            predecessors[l]->forward[l] = node->forward[l];
        }
        next_node->prev = prev_node;

        delete node;
    }
}

template<typename K, typename V>
void BlockedSkipList<K, V>::print() const {
    auto cur = head;
    while (cur != nullptr) {
        for (int i = 0; i < cur->size; i++) {
            std::cout << "["<< (cur->data[i]).key << ", " << (cur->data[i]).val << "] ";
        }
        std::cout << std::endl;
        cur = cur->forward[0];
    }
}
