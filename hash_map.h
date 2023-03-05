#pragma once

#include <functional>
#include <vector>
#include <utility>
#include <iostream>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
	class iterator {
	public:
		explicit iterator(HashMap *hashmap_reference = nullptr, size_t bucket_id = 0) :
				hashmap_reference_(hashmap_reference), bucket_id_(bucket_id) {}

		std::pair<const KeyType, ValueType> &operator*() {
			return *(std::pair<const KeyType, ValueType> *) (&hashmap_reference_->data_[bucket_id_]);
		}

		std::pair<const KeyType, ValueType> *operator->() {
			return (std::pair<const KeyType, ValueType> *) (&hashmap_reference_->data_[bucket_id_]);
		}

		bool operator==(const iterator &other) const {
			return bucket_id_ == other.bucket_id_ && hashmap_reference_ == other.hashmap_reference_;
		}

		bool operator!=(const iterator &other) const {
			return !(*this == other);
		}

		iterator &operator++() {
			++bucket_id_;
			while (bucket_id_ < hashmap_reference_->capacity_ && !hashmap_reference_->is_busy_[bucket_id_]) {
				++bucket_id_;
			}
			return *this;
		}

		iterator operator++(int) {
			iterator prev_iter = *this;
			++(*this);
			return prev_iter;
		}

		iterator &operator--() {
			--bucket_id_;
			while (!hashmap_reference_->is_busy_[bucket_id_]) {
				--bucket_id_;
			}
			return *this;
		}

		iterator operator--(int) {
			iterator prev_iter = *this;
			--(*this);
			return prev_iter;
		}

	private:
		HashMap *hashmap_reference_;
		size_t bucket_id_;
	};

	class const_iterator {
	public:
		explicit const_iterator(const HashMap *hashmap_reference = nullptr, size_t bucket_id = 0) :
				hashmap_reference_(hashmap_reference), bucket_id_(bucket_id) {}

		const std::pair<const KeyType, ValueType> &operator*() {
			return *(const std::pair<const KeyType, ValueType> *) (&hashmap_reference_->data_[bucket_id_]);
		}

		const std::pair<const KeyType, ValueType> *operator->() {
			return (const std::pair<const KeyType, ValueType> *) (&hashmap_reference_->data_[bucket_id_]);
		}

		bool operator==(const const_iterator &other) const {
			return bucket_id_ == other.bucket_id_ && hashmap_reference_ == other.hashmap_reference_;
		}

		bool operator!=(const const_iterator &other) const {
			return !(*this == other);
		}

		const_iterator &operator++() {
			++bucket_id_;
			while (bucket_id_ < hashmap_reference_->capacity_ && !hashmap_reference_->is_busy_[bucket_id_]) {
				++bucket_id_;
			}
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator prev_iter = *this;
			++(*this);
			return prev_iter;
		}

		const_iterator &operator--() {
			--bucket_id_;
			while (!hashmap_reference_->is_busy_[bucket_id_]) {
				--bucket_id_;
			}
			return *this;
		}

		const_iterator operator--(int) {
			const_iterator prev_iter = *this;
			--(*this);
			return prev_iter;
		}

	private:
		const HashMap *hashmap_reference_;
		size_t bucket_id_;
	};

	explicit HashMap(const Hash &hasher = Hash()) : capacity_(BEGIN_CAPACITY), size_(0), hasher_(hasher) {
		is_busy_ = std::vector<bool>(capacity_, false);
		data_ = std::vector<std::pair <KeyType, ValueType>>(capacity_);
	}

	template<class Iterator>
	HashMap(Iterator first, Iterator last, const Hash &hasher = Hash()) : HashMap(hasher) {
		for (Iterator element = first; element != last; ++element) {
			insert(*element);
		}
	}

	HashMap(std::initializer_list<std::pair<KeyType, ValueType>> init_list,
			const Hash &hasher = Hash()) : HashMap(hasher) {
		for (const auto &element: init_list) {
			insert(element);
		}
	}

	bool empty() const {
		return size_ == 0;
	}

	size_t size() const {
		return size_;
	}

	const Hash &hash_function() const {
		return hasher_;
	}

	void insert(std::pair<KeyType, ValueType> element) {
		size_t index = hasher_(element.first) % capacity_;
		while (is_busy_[index]) {
			if (element.first == data_[index].first) {
				return;
			}
			size_t distance = DistanceToHash(element.first, index);
			if (distance > DistanceToHash(data_[index].first, index)) {
				std::swap(element, data_[index]);
			}
			index = (index + 1) % capacity_;
		}
		is_busy_[index] = true;
		data_[index] = element;
		++size_;
		CheckMemoryLimit();
	}

	void erase(KeyType key) {
		size_t index = hasher_(key) % capacity_;
		bool deleted = false;
		while (is_busy_[index]) {
			if (key == data_[index].first) {
				deleted = true;
				is_busy_[index] = false;
				break;
			}
			index = (index + 1) % capacity_;
		}
		if (deleted) {
			--size_;
			size_t next_index = (index + 1) % capacity_;
			while (is_busy_[next_index] && DistanceToHash(data_[next_index].first, next_index) > 0) {
				data_[index] = data_[next_index];
				is_busy_[index] = true;
				is_busy_[next_index] = false;
				index = next_index;
				next_index = (next_index + 1) % capacity_;
			}
		}
	}

	iterator begin() {
		size_t index = 0;
		while (index < capacity_ && !is_busy_[index]) {
			++index;
		}
		return iterator(this, index);
	}

	iterator end() {
		return iterator(this, capacity_);
	}

	const_iterator begin() const {
		size_t index = 0;
		while (index < capacity_ && !is_busy_[index]) {
			++index;
		}
		return const_iterator(this, index);
	}

	const_iterator end() const {
		return const_iterator(this, capacity_);
	}

	iterator find(KeyType key) {
		size_t index = hasher_(key) % capacity_;
		while (is_busy_[index]) {
			if (key == data_[index].first) {
				return iterator(this, index);
			}
			index = (index + 1) % capacity_;
		}
		return end();
	}

	const_iterator find(KeyType key) const {
		size_t index = hasher_(key) % capacity_;
		while (is_busy_[index]) {
			if (key == data_[index].first) {
				return const_iterator(this, index);
			}
			index = (index + 1) % capacity_;
		}
		return end();
	}

	ValueType &operator[](KeyType key) {
		auto iterator = find(key);
		if (iterator == end()) {
			insert({key, ValueType()});
		}
		return find(key)->second;
	}

	const ValueType &at(KeyType key) const {
		auto iterator = find(key);
		if (iterator == end()) {
			throw std::out_of_range("needed key not found in hash_map");
		}
		return iterator->second;
	}

	void clear() {
		size_ = 0;
		capacity_ = BEGIN_CAPACITY;
		data_ = std::vector<std::pair <KeyType, ValueType>>(capacity_);
		is_busy_ = std::vector<bool>(capacity_, false);
	}

	HashMap(const HashMap &other) {
		hasher_ = other.hasher_;
		capacity_ = other.capacity_;
		size_ = other.size_;
		data_ = std::vector<std::pair <KeyType, ValueType>>(capacity_);
		is_busy_ = std::vector<bool>(capacity_, false);
		for (size_t i = 0; i < capacity_; ++i) {
			if (other.is_busy_[i]) {
				is_busy_[i] = true;
				data_[i] = other.data_[i];
			}
		}
	}

	HashMap &operator=(const HashMap &other) {
		if (this == &other) {
			return *this;
		}
		hasher_ = other.hasher_;
		capacity_ = other.capacity_;
		size_ = other.size_;
		data_ = std::vector<std::pair <KeyType, ValueType>>(capacity_);
		is_busy_ = std::vector<bool>(capacity_, false);
		for (size_t i = 0; i < capacity_; ++i) {
			if (other.is_busy_[i]) {
				is_busy_[i] = true;
				data_[i] = other.data_[i];
			}
		}
		return *this;
	}

private:
	const size_t BEGIN_CAPACITY = 4;
	const double LOAD_FACTOR = 0.42;
	size_t capacity_ = 0;
	size_t size_ = 0;

	std::vector<bool> is_busy_;
	std::vector<std::pair <KeyType, ValueType>> data_;

	Hash hasher_;

	void CheckMemoryLimit() {
		if (size_ > capacity_ * LOAD_FACTOR) {
			Reallocation();
		}
	}

	void Reallocation() {
		std::vector<std::pair<KeyType, ValueType>> elements(size_);
		size_t last_index = 0;
		for (size_t i = 0; i < capacity_; ++i) {
			if (is_busy_[i]) {
				elements[last_index++] = data_[i];
			}
		}
		capacity_ *= 2;
		size_ = 0;

		data_ = std::vector<std::pair <KeyType, ValueType>>(capacity_);
		is_busy_ = std::vector<bool>(capacity_, false);
		for (const auto &element: elements) {
			insert(element);
		}
	}

	size_t DistanceToHash(KeyType key, size_t cur_index) {
		size_t hash_index = hasher_(key) % capacity_;
		if (cur_index >= hash_index) {
			return cur_index - hash_index;
		}
		return cur_index + capacity_ - hash_index;
	}
};
