#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

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
	struct Node {
		value_type data;
		Node *prev, *next;      // list order (insertion order)
		Node *hnext;            // bucket chain
		Node(const value_type &v) : data(v), prev(nullptr), next(nullptr), hnext(nullptr) {}
	};

	Node *head, *tail;          // sentinels of doubly-linked list (data uninitialized)
	Node **buckets;
	size_t bucketCount;
	size_t elemCount;

	static const size_t INIT_BUCKETS = 16;

	size_t hashIndex(const Key &key, size_t bc) const {
		return Hash()(key) % bc;
	}

	Node *allocSentinel() {
		return static_cast<Node*>(operator new(sizeof(Node)));
	}

	void freeSentinel(Node *n) {
		operator delete(n);
	}

	void initList() {
		head = allocSentinel();
		tail = allocSentinel();
		head->prev = nullptr;
		head->next = tail;
		tail->prev = head;
		tail->next = nullptr;
	}

	void initBuckets(size_t bc) {
		bucketCount = bc;
		buckets = new Node*[bc];
		for (size_t i = 0; i < bc; ++i) buckets[i] = nullptr;
	}

	void linkBack(Node *n) {
		n->prev = tail->prev;
		n->next = tail;
		tail->prev->next = n;
		tail->prev = n;
	}

	void unlink(Node *n) {
		n->prev->next = n->next;
		n->next->prev = n->prev;
	}

	void bucketInsert(Node *n, size_t bc) {
		size_t idx = hashIndex(n->data.first, bc);
		n->hnext = buckets[idx];
		buckets[idx] = n;
	}

	void bucketRemove(Node *n) {
		size_t idx = hashIndex(n->data.first, bucketCount);
		Node *cur = buckets[idx];
		Node *prev = nullptr;
		while (cur) {
			if (cur == n) {
				if (prev) prev->hnext = cur->hnext;
				else buckets[idx] = cur->hnext;
				return;
			}
			prev = cur;
			cur = cur->hnext;
		}
	}

	Node *findNode(const Key &key) const {
		size_t idx = hashIndex(key, bucketCount);
		Node *cur = buckets[idx];
		while (cur) {
			if (Equal()(cur->data.first, key)) return cur;
			cur = cur->hnext;
		}
		return nullptr;
	}

	void rehash() {
		size_t newCount = bucketCount * 2;
		Node **newBuckets = new Node*[newCount];
		for (size_t i = 0; i < newCount; ++i) newBuckets[i] = nullptr;
		delete[] buckets;
		buckets = newBuckets;
		bucketCount = newCount;
		for (Node *cur = head->next; cur != tail; cur = cur->next) {
			bucketInsert(cur, bucketCount);
		}
	}

	void maybeRehash() {
		if (elemCount > bucketCount * 3 / 4) rehash();
	}

	void destroyAll() {
		Node *cur = head->next;
		while (cur != tail) {
			Node *nxt = cur->next;
			delete cur;
			cur = nxt;
		}
		head->next = tail;
		tail->prev = head;
		for (size_t i = 0; i < bucketCount; ++i) buckets[i] = nullptr;
		elemCount = 0;
	}

	void copyFrom(const linked_hashmap &other) {
		for (Node *cur = other.head->next; cur != other.tail; cur = cur->next) {
			Node *n = new Node(cur->data);
			linkBack(n);
			bucketInsert(n, bucketCount);
			++elemCount;
			maybeRehash();
		}
	}

public:
	class const_iterator;
	class iterator {
		friend class linked_hashmap;
		friend class const_iterator;
	private:
		Node *node;
		const linked_hashmap *container;
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;

		iterator() : node(nullptr), container(nullptr) {}
		iterator(Node *n, const linked_hashmap *c) : node(n), container(c) {}
		iterator(const iterator &other) : node(other.node), container(other.container) {}

		iterator operator++(int) {
			if (node == nullptr || node == container->tail) throw invalid_iterator();
			iterator tmp = *this;
			node = node->next;
			return tmp;
		}
		iterator & operator++() {
			if (node == nullptr || node == container->tail) throw invalid_iterator();
			node = node->next;
			return *this;
		}
		iterator operator--(int) {
			if (node == nullptr || node->prev == container->head) throw invalid_iterator();
			iterator tmp = *this;
			node = node->prev;
			return tmp;
		}
		iterator & operator--() {
			if (node == nullptr || node->prev == container->head) throw invalid_iterator();
			node = node->prev;
			return *this;
		}
		value_type & operator*() const {
			return node->data;
		}
		bool operator==(const iterator &rhs) const {
			return node == rhs.node && container == rhs.container;
		}
		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node && container == rhs.container;
		}
		bool operator!=(const iterator &rhs) const {
			return !(*this == rhs);
		}
		bool operator!=(const const_iterator &rhs) const {
			return !(*this == rhs);
		}
		value_type* operator->() const noexcept {
			return &(node->data);
		}
	};

	class const_iterator {
		friend class linked_hashmap;
		friend class iterator;
	private:
		Node *node;
		const linked_hashmap *container;
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;

		const_iterator() : node(nullptr), container(nullptr) {}
		const_iterator(Node *n, const linked_hashmap *c) : node(n), container(c) {}
		const_iterator(const const_iterator &other) : node(other.node), container(other.container) {}
		const_iterator(const iterator &other) : node(other.node), container(other.container) {}

		const_iterator operator++(int) {
			if (node == nullptr || node == container->tail) throw invalid_iterator();
			const_iterator tmp = *this;
			node = node->next;
			return tmp;
		}
		const_iterator & operator++() {
			if (node == nullptr || node == container->tail) throw invalid_iterator();
			node = node->next;
			return *this;
		}
		const_iterator operator--(int) {
			if (node == nullptr || node->prev == container->head) throw invalid_iterator();
			const_iterator tmp = *this;
			node = node->prev;
			return tmp;
		}
		const_iterator & operator--() {
			if (node == nullptr || node->prev == container->head) throw invalid_iterator();
			node = node->prev;
			return *this;
		}
		const value_type & operator*() const {
			return node->data;
		}
		bool operator==(const iterator &rhs) const {
			return node == rhs.node && container == rhs.container;
		}
		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node && container == rhs.container;
		}
		bool operator!=(const iterator &rhs) const {
			return !(*this == rhs);
		}
		bool operator!=(const const_iterator &rhs) const {
			return !(*this == rhs);
		}
		const value_type* operator->() const noexcept {
			return &(node->data);
		}
	};

	linked_hashmap() : elemCount(0) {
		initList();
		initBuckets(INIT_BUCKETS);
	}

	linked_hashmap(const linked_hashmap &other) : elemCount(0) {
		initList();
		initBuckets(INIT_BUCKETS);
		copyFrom(other);
	}

	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this == &other) return *this;
		destroyAll();
		copyFrom(other);
		return *this;
	}

	~linked_hashmap() {
		destroyAll();
		delete[] buckets;
		freeSentinel(head);
		freeSentinel(tail);
	}

	T & at(const Key &key) {
		Node *n = findNode(key);
		if (!n) throw index_out_of_bound();
		return n->data.second;
	}
	const T & at(const Key &key) const {
		Node *n = findNode(key);
		if (!n) throw index_out_of_bound();
		return n->data.second;
	}

	T & operator[](const Key &key) {
		Node *n = findNode(key);
		if (n) return n->data.second;
		Node *nn = new Node(value_type(key, T()));
		linkBack(nn);
		bucketInsert(nn, bucketCount);
		++elemCount;
		maybeRehash();
		return nn->data.second;
	}

	const T & operator[](const Key &key) const {
		Node *n = findNode(key);
		if (!n) throw index_out_of_bound();
		return n->data.second;
	}

	iterator begin() {
		return iterator(head->next, this);
	}
	const_iterator cbegin() const {
		return const_iterator(head->next, this);
	}

	iterator end() {
		return iterator(tail, this);
	}
	const_iterator cend() const {
		return const_iterator(tail, this);
	}

	bool empty() const {
		return elemCount == 0;
	}

	size_t size() const {
		return elemCount;
	}

	void clear() {
		destroyAll();
	}

	pair<iterator, bool> insert(const value_type &value) {
		Node *n = findNode(value.first);
		if (n) {
			return pair<iterator, bool>(iterator(n, this), false);
		}
		Node *nn = new Node(value);
		linkBack(nn);
		bucketInsert(nn, bucketCount);
		++elemCount;
		maybeRehash();
		return pair<iterator, bool>(iterator(nn, this), true);
	}

	void erase(iterator pos) {
		if (pos.container != this || pos.node == nullptr || pos.node == tail || pos.node == head) {
			throw invalid_iterator();
		}
		Node *n = pos.node;
		unlink(n);
		bucketRemove(n);
		delete n;
		--elemCount;
	}

	size_t count(const Key &key) const {
		return findNode(key) ? 1 : 0;
	}

	iterator find(const Key &key) {
		Node *n = findNode(key);
		if (!n) return end();
		return iterator(n, this);
	}
	const_iterator find(const Key &key) const {
		Node *n = findNode(key);
		if (!n) return cend();
		return const_iterator(n, this);
	}
};

}

#endif
