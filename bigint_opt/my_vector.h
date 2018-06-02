#define _SCL_SECURE_NO_WARNINGS

#ifndef OPTS_MY_VECTOR_H
#define OPTS_MY_VECTOR_H
#include <vector>
#include <memory>
#include <string.h>
#include <iterator>
#include <algorithm>

using std::shared_ptr;

typedef shared_ptr<uint32_t> shp_type;

class my_vector {
public:
	using uint = std::uint32_t;
	using ll = std::int64_t;

private:
	static constexpr size_t SMALL_CAPACITY = 4;

	struct data_storage {
		shp_type big_ptr;
		size_t capacity;

		data_storage()
			: big_ptr(nullptr), capacity(0) {
		};

		~data_storage() = default;
	};

	size_t vector_size;

	union {
		data_storage big_object;
		uint small_object[SMALL_CAPACITY];
	};

	bool is_small;
	uint * cur_ptr;

	size_t estimate_capacity(size_t new_size);
	inline void ensure_capacity(size_t new_capacity);

public:

	my_vector();
	my_vector(size_t sz);
	my_vector(size_t sz, uint value);
	my_vector(my_vector const & other) noexcept;
	my_vector& operator=(my_vector const & other) noexcept;
	~my_vector();

	uint& operator[](size_t index);
	uint const & operator[](size_t index) const;

	void push_back(uint const x);
	void pop_back();

	uint& back() const;
	size_t size() const;
	size_t capacity() const;

	bool empty() const;
	void clear();

	void resize(size_t n);
	void resize(size_t n, uint value);

	void swap_small_big(my_vector &a, my_vector &b) noexcept;
	void swap(my_vector & other) noexcept;

	void make_unique_copy();
	void reverse();

	void remove_last_zeros();

	uint* begin() noexcept;
	uint const* begin() const noexcept;
	uint* end() noexcept;
	uint const* end() const noexcept;

	template<typename Iterator>
	std::reverse_iterator<Iterator> make_reverse_iterator(Iterator i) {
		return std::reverse_iterator<Iterator>(i);
	}

	template<typename Iterator>
	std::reverse_iterator<Iterator> make_reverse_iterator(Iterator i) const {
		return std::reverse_iterator<Iterator>(i);
	}

	std::reverse_iterator< uint* > rbegin() noexcept;
	std::reverse_iterator< const uint* > rbegin() const noexcept;
	std::reverse_iterator< uint* > rend() noexcept;
	std::reverse_iterator< const uint* > rend() const noexcept;
};

#endif //OPTS_MY_VECTOR_H