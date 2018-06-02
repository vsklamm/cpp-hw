#define _SCL_SECURE_NO_WARNINGS

#include "my_vector.h"
#include <cassert>
#include <memory>
#include <string.h>
#include <algorithm>
#include <iterator>

using uint = std::uint32_t;

constexpr size_t UINT_SIZE = sizeof(uint);

my_vector::my_vector()
	: vector_size(0), big_object(), is_small(true), cur_ptr(small_object)
{
	std::fill(cur_ptr, cur_ptr + SMALL_CAPACITY, 0);
}

my_vector::my_vector(size_t required_size)
	: vector_size(required_size), big_object(), is_small(required_size <= SMALL_CAPACITY)
{
	if (!is_small) {
		big_object.capacity = required_size;
		new(&big_object.big_ptr) shp_type(new uint[required_size], std::default_delete<uint[]>());
		cur_ptr = big_object.big_ptr.get();
	}
	else {
		cur_ptr = small_object;
	}
	std::fill(cur_ptr, cur_ptr + capacity(), 0);
}

my_vector::my_vector(size_t required_size, uint value)
	: vector_size(required_size), big_object(), is_small(required_size <= SMALL_CAPACITY)
{
	if (!is_small) {
		big_object.capacity = required_size;
		new(&big_object.big_ptr) shp_type(new uint[required_size], std::default_delete<uint[]>());
		cur_ptr = big_object.big_ptr.get();
	}
	else {
		cur_ptr = small_object;
	}
	std::fill(cur_ptr + required_size, cur_ptr + capacity(), 0);
	std::fill(begin(), end(), value);
}

my_vector::my_vector(my_vector const & other) noexcept
	: vector_size(other.size()), is_small(other.is_small)
{
	if (!other.is_small) {
		big_object.capacity = other.big_object.capacity;
		new (&big_object.big_ptr) shp_type(other.big_object.big_ptr);
		cur_ptr = big_object.big_ptr.get();
	}
	else {
		cur_ptr = small_object;
		std::copy(other.small_object, other.small_object + SMALL_CAPACITY, small_object);
	}
}

my_vector & my_vector::operator=(my_vector const & other) noexcept
{
	if (!is_small)
		big_object.~data_storage();

	vector_size = other.vector_size;
	is_small = other.is_small;
	if (!other.is_small) {
		big_object.capacity = other.big_object.capacity;
		new (&big_object.big_ptr) shp_type(other.big_object.big_ptr);
		cur_ptr = big_object.big_ptr.get();
	}
	else {
		cur_ptr = small_object;
		std::copy(other.small_object, other.small_object + SMALL_CAPACITY, small_object);
	}
	return *this;
}

my_vector::~my_vector() {
	if (!is_small)
		big_object.~data_storage();
}

uint & my_vector::operator[](size_t index)
{
	assert(index < vector_size && "vector subscript out of range");

	make_unique_copy();
	return cur_ptr[index];
}

uint const & my_vector::operator[](size_t index) const
{
	assert(index < vector_size && "vector subscript out of range");

	return cur_ptr[index];
}

void my_vector::push_back(uint const value)
{
	assert(is_small || big_object.big_ptr.unique());

	ensure_capacity(estimate_capacity(vector_size + 1));

	cur_ptr[vector_size] = value;
	++vector_size;
}

void my_vector::pop_back()
{
	assert(vector_size != 0);
	assert(is_small || big_object.big_ptr.unique());

	ensure_capacity(estimate_capacity(vector_size - 1));
	--vector_size;
}

uint & my_vector::back() const
{
	assert(vector_size != 0);

	return cur_ptr[vector_size - 1];
}

size_t my_vector::size() const
{
	return vector_size;
}

size_t my_vector::capacity() const
{
	if (is_small)
		return SMALL_CAPACITY;
	else
		return big_object.capacity;
}

bool my_vector::empty() const
{
	return vector_size == 0;
}

void my_vector::clear()
{
	vector_size = 0;
	big_object.big_ptr = nullptr;
}

void my_vector::resize(size_t n)
{
	resize(n, 0);
}

void my_vector::resize(size_t n, uint value)
{
	size_t old_size = vector_size;
	ensure_capacity(n);

	if (n > old_size) {
		std::fill(cur_ptr + old_size, cur_ptr + n, value);
	}
	vector_size = n;
}

void my_vector::swap_small_big(my_vector & to_big, my_vector & to_small) noexcept
{
	uint temp[SMALL_CAPACITY];
	std::copy(to_big.begin(), to_big.end(), temp);

	new (&to_big.big_object.big_ptr) shp_type(to_small.big_object.big_ptr);

	to_small.big_object.big_ptr = nullptr;
	std::copy(temp, temp + SMALL_CAPACITY, to_small.small_object);
}

void my_vector::swap(my_vector & other) noexcept
{
	using std::swap;
	if (is_small == other.is_small) {
		if (!is_small) {
			swap(big_object, other.big_object);
			swap(other.cur_ptr, cur_ptr);
		}
		else {
			for (size_t i = 0; i != SMALL_CAPACITY; ++i) {
				swap(small_object[i], other.small_object[i]);
			}
		}
	}
	else {
		swap(is_small, other.is_small);
		if (is_small) {
			swap_small_big(other, *this);
			cur_ptr = small_object;
			big_object.big_ptr = nullptr;
			other.cur_ptr = other.big_object.big_ptr.get();
		}
		else {
			swap_small_big(*this, other);
			other.cur_ptr = other.small_object;
			cur_ptr = big_object.big_ptr.get();
		}
	}
	swap(vector_size, other.vector_size);
}

void my_vector::make_unique_copy()
{
	if (!is_small && !big_object.big_ptr.unique()) {
		my_vector tmp(big_object.capacity);
		std::copy(
			big_object.big_ptr.get(),
			big_object.big_ptr.get() + big_object.capacity,
			tmp.big_object.big_ptr.get()
		);
		*this = tmp;
		cur_ptr = big_object.big_ptr.get();
	}
}

void my_vector::reverse()
{
	make_unique_copy();
	size_t half_size = (vector_size >> 1);
	for (size_t i = 0; i != half_size; ++i) {
		std::swap(cur_ptr[i], cur_ptr[vector_size - i - 1]);
	}
}

size_t my_vector::estimate_capacity(size_t new_size)
{
	if (capacity() >= new_size)
		return (new_size == 0)
		? SMALL_CAPACITY
		: (capacity() > (new_size << 2))
		? new_size << 1
		: new_size;
	else
		return new_size << 1;
}

inline void my_vector::ensure_capacity(size_t new_capacity)
{
	size_t old_capacity = capacity();
	size_t old_size = size();

	if (new_capacity <= old_capacity)
		return;

	my_vector tmp(new_capacity);
	std::copy(cur_ptr, cur_ptr + old_capacity, tmp.big_object.big_ptr.get());
	if (old_capacity == SMALL_CAPACITY) {
		is_small = false;
		std::fill(small_object, small_object + SMALL_CAPACITY, 0);
	}
	*this = tmp;
	vector_size = old_size;
	cur_ptr = tmp.big_object.big_ptr.get();
}

void my_vector::remove_last_zeros()
{
	if (vector_size > 1 && back() == 0) {
		auto it = find_if(rbegin(), rend(), [&](const uint& element) { return element != 0; });
		size_t last_zero_pos = it - rbegin();
		last_zero_pos = vector_size - last_zero_pos;
		last_zero_pos += (last_zero_pos == 0 && ((*this)[0] == 0)) ? 1 : 0;

		resize(last_zero_pos);
	}
}

uint* my_vector::begin() noexcept
{
	return cur_ptr;
}

uint const* my_vector::begin() const noexcept
{
	return cur_ptr;
}

uint* my_vector::end() noexcept
{
	return cur_ptr + vector_size;
}

uint const* my_vector::end() const noexcept
{
	return cur_ptr + vector_size;
}

std::reverse_iterator<uint*> my_vector::rbegin() noexcept
{
	return make_reverse_iterator(end());
}

std::reverse_iterator<const uint*> my_vector::rbegin() const noexcept
{
	return make_reverse_iterator(end());
}

std::reverse_iterator<uint*> my_vector::rend() noexcept
{
	return make_reverse_iterator(begin());
}

std::reverse_iterator<const uint*> my_vector::rend() const noexcept
{
	return make_reverse_iterator(begin());
}