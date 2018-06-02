#include "big_integer.h"
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

#include <iostream>
using uint = std::uint32_t;

extern constexpr uint CHUNK_BIT_SIZE = sizeof(int32_t) << 3;
extern constexpr uint64_t NUM_SYS_BASE = (uint64_t)MAX_CHUNK_NUM + 1;

big_integer::big_integer()
	: signum(0) {
	data.clear();
	data.resize(1);
	data[0] = 0;
}

big_integer::big_integer(int a)
	: signum((a > 0) ? 1 : ((a == 0) ? 0 : -1)) {
	data.clear();
	data.resize(1);

	data[0] = absolute(a);
}

big_integer::big_integer(uint a)
	: signum((a == 0) ? 0 : 1) {
	data.clear();
	data.resize(1);

	signum = (a == 0) ? 0 : 1;
	data[0] = a;
}

big_integer::big_integer(std::string const &str) {
	if (str_to_bint(str, *this) != 0) {
		throw std::runtime_error("invalid string");
	}
}

big_integer &big_integer::operator+=(big_integer const &rhs) {
	if (rhs.is_zero()) {
		return *this;
	}
	else if (this->is_zero()) {
		return *this = rhs;
	}

	if (signum == rhs.signum) {
		shifted_summation(rhs.data, 0);
		remove_leading_0(data);
	}
	else {
		negate();
		*this -= rhs;
		negate();
	}
	return *this;
}

big_integer &big_integer::operator-=(big_integer const &rhs) {
	if (rhs.is_zero()) {
		return *this;
	}
	else if (this->is_zero()) {
		return *this = -rhs;
	}

	if (signum == rhs.signum) {
		int comp = compare_abs_numbers(*this, rhs);
		if (comp == 0) {
			return *this = big_integer();
		}
		const big_integer *first = this, *second = &rhs;
		bool invert = false;
		if (comp == -1) {
			std::swap(first, second);
			invert = true;
		}

		big_integer res = seqset_subtract(*first, *second);
		if (invert) {
			res.negate();
		}
		*this = res;
	}
	else {
		negate();
		*this += rhs;
		negate();
	}
	return *this;
}

big_integer &big_integer::operator*=(big_integer const &rhs) {
	if (rhs.is_zero()) {
		return *this = 0;
	}
	else if (this->is_zero()) {
		return *this;
	}
	else if (rhs.get_data_size() == 1) {
		mul_this_long_short(rhs.get_chunk(0));
		if (rhs.signum == -1) {
			this->negate();
		}
		return *this;
	}

	big_integer res;
	for (size_t i = 0; i != rhs.get_data_size(); ++i) {
		res.shifted_summation(mul_long_short(rhs.data[i]).get_data(), i);
		remove_leading_0(res.data);
	}

	res.signum = signum * rhs.signum;
	return *this = res;
}

big_integer &big_integer::operator/=(big_integer const &rhs) {
	if (rhs.is_zero()) {
		throw std::runtime_error("division by 0");
	}
	else if (this->is_zero()) {
		return *this;
	}
	else if (rhs.get_data_size() == 1) {
		div_long_short(rhs.get_chunk(0));
		this->signum *= rhs.signum;
		return *this;
	}

	int comp = compare_abs_numbers(*this, rhs);
	if (comp == -1)
		return *this = 0;
	else if (comp == 0)
		return *this = 1;

	big_integer res;
	big_integer remaind(*this), divis(rhs);

	const uint k = (uint)(NUM_SYS_BASE / ((ull)divis.data.back() + 1));
	remaind.mul_this_long_short(k); divis.mul_this_long_short(k);

	remaind.data.push_back(0);
	size_t prefix_size = divis.get_data_size() + 1, dividend_size = remaind.get_data_size();

	res.data.resize(dividend_size - prefix_size + 1);

	big_integer q_ch;
	uint quotient_chunk;

	size_t i = prefix_size, j = res.data.size() - 1;
	while (i++ <= dividend_size)
	{
		ull two_last_chunks = ((ull)(remaind.data[remaind.get_data_size() - 1]) << CHUNK_BIT_SIZE)
			| remaind.data[remaind.get_data_size() - 2];
		quotient_chunk = std::min((uint)(two_last_chunks / divis.data.back()), MAX_CHUNK_NUM);

		q_ch = divis.mul_long_short(quotient_chunk);

		while (compare_prefix(remaind, q_ch, prefix_size) == 0) {
			--quotient_chunk;
			q_ch = divis.mul_long_short(quotient_chunk);
		}
		res.data[j] = quotient_chunk;
		prefix_subtract(remaind, q_ch, prefix_size);

		if (remaind.data.back() == 0) remaind.data.pop_back();
		--j;
	}
	remove_leading_0(res.data);

	res.signum = signum * rhs.signum;
	return *this = res;
}

big_integer &big_integer::operator%=(big_integer const &rhs) {
	if (rhs.get_data_size() == 1) {
		int res_sign = this->signum;
		*this = div_long_short(rhs.get_chunk(0));

		this->signum *= res_sign;
		return *this;
	}
	return *this -= (*this / rhs) * rhs;
}

big_integer &big_integer::operator&=(big_integer const &rhs) {
	if (this->is_zero()) {
		return *this;
	}
	else if (rhs.is_zero()) {
		return *this = 0;
	}

	big_integer res = apply_logical_operation(
		this,
		rhs,
		[](uint a, uint b) { return a & b; }
	);
	return *this = res;
}

big_integer &big_integer::operator|=(big_integer const &rhs) {
	if (this->is_zero()) {
		return *this = rhs;
	}
	else if (rhs.is_zero()) {
		return *this;
	}

	big_integer res = apply_logical_operation(
		this,
		rhs,
		[](uint a, uint b) { return a | b; }
	);
	return *this = res;
}

big_integer &big_integer::operator^=(big_integer const &rhs) {
	if (this->is_zero()) {
		return *this = rhs;
	}
	else if (rhs.is_zero()) {
		return *this;
	}

	big_integer res = apply_logical_operation(
		this,
		rhs,
		[](uint a, uint b) { return a ^ b; }
	);
	return *this = res;
}

big_integer &big_integer::operator<<=(int shift) {
	if (this->is_zero()) {
		return *this;
	}
	else if (shift < 0) {
		return *this >>= -shift;
	}

	seqset copy_this(this->data);

	size_t shift_out = shift / CHUNK_BIT_SIZE;
	ull shift_in = shift % CHUNK_BIT_SIZE;
	seqset tmp(shift_out + copy_this.size(), 0);

	ull cur = 0;
	for (size_t i = 0; i != copy_this.size(); ++i) {
		ull buf = copy_this[i];
		cur |= (buf << shift_in);
		tmp[i + shift_out] = (cur & MAX_CHUNK_NUM);
		cur >>= CHUNK_BIT_SIZE;
	}
	while (cur) {
		tmp.push_back((uint)cur);
		cur >>= CHUNK_BIT_SIZE;
	}

	this->data = tmp;
	return *this;
}

big_integer &big_integer::operator>>=(int shift) {
	if (this->is_zero()) {
		return *this;
	}
	else if (shift < 0) {
		return *this <<= -shift;
	}

	seqset copy_this;
	copy_this = (signum == 1)
		? copy_this = this->data
		: copy_this = this->convert_to_2c().get_data();

	ull shift_in = shift % CHUNK_BIT_SIZE;
	size_t shift_out = shift / CHUNK_BIT_SIZE;
	seqset tmp(copy_this.size(), (signum == 1)
		? 0
		: (MAX_CHUNK_NUM << (CHUNK_BIT_SIZE - shift_in)));

	ull cur = 0;
	size_t i = copy_this.size();
	while (i-- && (i >= shift_out)) {
		ull buf = (ull)copy_this[i] << CHUNK_BIT_SIZE;
		cur |= (buf >> shift_in);
		tmp[i - shift_out] |= ((cur >> CHUNK_BIT_SIZE) & MAX_CHUNK_NUM);
		cur <<= CHUNK_BIT_SIZE;
	}
	remove_leading_0(tmp);

	this->data = tmp;
	if (get_data_size() == 1 && get_chunk(0) == 0)
		this->signum = 0;

	return (signum < 0)
		? *this = this->convert_to_2c()
		: *this;
}

big_integer big_integer::operator+() const {
	return *this;
}

big_integer big_integer::operator-() const {
	big_integer res(*this);
	res.negate();
	return res;
}

big_integer big_integer::operator~() const {
	big_integer res(*this);
	return res.invert();
}

big_integer &big_integer::operator++() {
	return (*this = *this + 1);
}

big_integer big_integer::operator++(int) {
	big_integer r(*this);
	++*this;
	return r;
}

big_integer &big_integer::operator--() {
	return (*this = *this - 1);
}

big_integer big_integer::operator--(int) {
	big_integer r(*this);
	--*this;
	return r;
}

big_integer operator+(big_integer a, big_integer const &b) { return a += b; }

big_integer operator-(big_integer a, big_integer const &b) { return a -= b; }

big_integer operator*(big_integer a, big_integer const &b) { return a *= b; }

big_integer operator/(big_integer a, big_integer const &b) { return a /= b; }

big_integer operator%(big_integer a, big_integer const &b) { return a %= b; }

big_integer operator&(big_integer a, big_integer const &b) { return a &= b; }

big_integer operator|(big_integer a, big_integer const &b) { return a |= b; }

big_integer operator^(big_integer a, big_integer const &b) { return a ^= b; }

big_integer operator<<(big_integer a, int b) { return a <<= b; }

big_integer operator>>(big_integer a, int b) { return a >>= b; }

bool operator==(big_integer const &first, big_integer const &second) {
	return (first.data == second.data) && (first.signum == second.signum);
}

bool operator!=(big_integer const &a, big_integer const &b) {
	return !(a == b);
}

bool operator<(big_integer const &a, big_integer const &b) {
	if (a.signum == 0)
		return b.signum > 0;
	else if (b.signum == 0)
		return a.signum < 0;
	else if (a.signum != b.signum)
		return a.signum < b.signum;
	else return compare_abs_numbers(a, b) * a.signum == -1;
}

bool operator>(big_integer const &a, big_integer const &b) {
	return (b < a);
}

bool operator<=(big_integer const &a, big_integer const &b) {
	return !(a > b);
}

bool operator>=(big_integer const &a, big_integer const &b) {
	return !(a < b);
}

std::string to_string(big_integer const &number)
{
	return to_string(number, '\0');
}

std::string to_string(big_integer const & number, char separator)
{
	string result = "";
	size_t sep_cnt = 0;
	big_integer copy_number(number);

	while (!copy_number.is_zero()) {
		result.push_back((char)copy_number.div_long_short(10) + '0');

		if (separator != '\0' && ++sep_cnt % 3 == 0) {
			result.push_back(separator);
			sep_cnt = 0;
		}
	}
	if (separator != '\0' && result.back() == separator)
		result.pop_back();

	if (number.signum == -1) {
		result.push_back('-');
	}
	std::reverse(result.begin(), result.end());
	if (result.empty()) {
		result = "0";
	}
	return result;
}

big_integer::uint big_integer::absolute(int a)
{
	uint res;
	if (a == INT32_MIN) {
		res = (uint)INT32_MAX + 1;
	}
	else {
		if (signum == -1) res = -a;
		else res = a;
	}
	return res;
}

void big_integer::remove_leading_0(seqset& number) {
	while (number.size() > 1 && number.back() == 0) {
		number.pop_back();
	}
}

big_integer &big_integer::negate() {
	signum = -signum;
	return *this;
}

big_integer &big_integer::invert() {
	negate();
	return --*this;
}

void big_integer::mul_seqset_short(seqset &seq, uint val)
{
	ull carry = 0;
	for (size_t i = 0; i != seq.size(); ++i) {
		carry += (ull)seq[i] * val;
		seq[i] = (uint)(carry & MAX_CHUNK_NUM);
		carry >>= CHUNK_BIT_SIZE;
	}
	if (carry != 0) {
		seq.push_back((uint)(carry & MAX_CHUNK_NUM));
		carry >>= CHUNK_BIT_SIZE;
	}
}

void big_integer::mul_this_long_short(uint val) {
	mul_seqset_short(data, val);
	remove_leading_0(data);
}

big_integer big_integer::mul_long_short(uint val) const {
	big_integer res(*this);
	res.mul_this_long_short(val);
	return res;
}

uint big_integer::div_long_short(uint value)
{
	const uint base_mod_val = (uint)(NUM_SYS_BASE % value);
	uint modulo = 0;
	ull carry = 0, tmp = 0;
	for (size_t i = data.size(); i--;)
	{
		modulo = (uint)((ull)modulo * base_mod_val + (ull)data[i]) % value;
		tmp = carry * NUM_SYS_BASE + (ull)data[i];
		data[i] = (uint)(tmp / value);
		carry = tmp % value;
	}
	remove_leading_0(data);
	if (data.back() == 0) signum = 0;
	return modulo;
}

void big_integer::shifted_summation(seqset const &second, size_t shift) {
	if (second.size() == 1 && second[0] == 0)
		return;

	ull carry = 0;
	seqset copy_second(second);

	size_t max_size = std::max((*this).data.size(), copy_second.size());
	(*this).data.resize(max_size + shift, 0);
	if (copy_second.size() < max_size) {
		copy_second.resize(max_size, 0);
	}

	for (size_t i = 0; i != max_size; ++i) {
		carry += (ull)(*this).data[i + shift] + (ull)copy_second[i];
		(*this).data[i + shift] = (uint)(carry & MAX_CHUNK_NUM);
		carry >>= CHUNK_BIT_SIZE;
	}
	if (carry != 0) {
		(*this).data.push_back((uint)(carry & MAX_CHUNK_NUM));
		carry >>= CHUNK_BIT_SIZE;
	}
}

big_integer big_integer::seqset_subtract(big_integer const &lhs, big_integer const &rhs)
{
	size_t max_size = std::max(lhs.get_data_size(), rhs.get_data_size());
	seqset res_data(max_size);

	ull carry = 0;
	for (size_t i = 0; i != rhs.get_data_size(); ++i) {
		if (rhs.get_chunk(i) <= (ll)lhs.get_chunk(i) - carry) {
			res_data[i] = (uint)((ull)lhs.get_chunk(i) - carry - rhs.get_chunk(i));
			carry = 0;
		}
		else {
			res_data[i] = (uint)((ull)lhs.get_chunk(i) + NUM_SYS_BASE - carry - rhs.get_chunk(i));
			carry = 1;
		}
	}
	for (size_t i = rhs.get_data_size(); i != max_size; ++i) {
		res_data[i] = (uint)(lhs.data[i] - carry);
		carry = 0;
	}

	remove_leading_0(res_data);

	big_integer res;
	res.data = res_data;
	res.signum = lhs.signum;
	return res;
}

void big_integer::prefix_subtract(big_integer &first, big_integer const &second, size_t const prefix_size) {
	size_t postfix_shift = first.get_data_size() - prefix_size;
	uint sec_chunk, tmp;
	int borrow = 0;
	for (size_t i = 0; i != prefix_size; ++i) {
		sec_chunk = (i < second.get_data_size())
			? second.data[i]
			: 0;
		tmp = first.data[i + postfix_shift] - sec_chunk - borrow;
		borrow = (int)((sec_chunk + borrow) > first.data[i + postfix_shift]);
		first.data[i + postfix_shift] = tmp;
	}
}

int big_integer::compare_prefix(big_integer const &first, big_integer const &second, size_t const pref_size) {
	int start = (int)(first.get_data_size() - pref_size);

	uint sec_chunk;
	for (int i = (int)first.get_data_size() - 1, j = (int)(pref_size - 1); i >= start; --i, --j) {
		sec_chunk = (j < (int)second.get_data_size())
			? second.data[j]
			: 0;
		if (first.data[i] != sec_chunk) {
			return (first.data[i] > sec_chunk)
				? 1
				: 0;
		}
	}
	return 1;
}

int big_integer::str_to_bint(const string &str, big_integer &number) {
	if (str.empty()) {
		throw std::runtime_error("empty string");
	}
	number.signum = 0;

	size_t i = 0;
	if (str[i] == '-' || str[i] == '+') {
		++i;
	}
	for (; i != str.size(); ++i) {
		if (!std::isdigit(str[i])) {
			throw std::runtime_error("invalid character: " + str[i]);
		}
		number.mul_this_long_short(10);
		number += (int)(str[i] - '0');
	}

	if (number.get_data_size() != 0) {
		number.signum = (str[0] == '-') ? -1 : 1;
	}
	else {
		number.data.push_back(0);
	}

	remove_leading_0(number.data);
	return 0;
}

big_integer big_integer::convert_to_2c() const {
	if (signum == 1 || signum == 0) {
		return *this;
	}
	big_integer res(*this);
	std::for_each(res.data.begin(), res.data.end(), [](uint &x) { x = ~x; });
	res--;
	return res;
}

int compare_abs_numbers(big_integer const &first, big_integer const &second)
{
	if (first.get_data_size() == second.get_data_size()) {
		auto mypair = std::mismatch(first.data.rbegin(), first.data.rend(), second.data.rbegin());
		if (mypair.first == first.data.rend()) {
			return 0;
		}
		else {
			return (*mypair.first > *mypair.second) ? 1 : -1;
		}
	}
	else return (first.get_data_size() > second.get_data_size()) ? 1 : -1;
}

std::ostream &operator<<(std::ostream &out, big_integer const &a)
{
	return out << to_string(a);
}

std::istream & operator>>(std::istream & in, big_integer &a)
{
	std::string tmp;
	in >> tmp;
	a = big_integer(tmp);
	return in;
}

template<typename FunctorT>
big_integer big_integer::apply_logical_operation(
	big_integer * lhs,
	big_integer const & rhs,
	FunctorT logical_oper)
{
	big_integer lhs_adding = lhs->convert_to_2c();
	big_integer rhs_adding = rhs.convert_to_2c();

	size_t len = std::max(lhs_adding.get_data_size(), rhs_adding.get_data_size());
	lhs_adding.data.resize(len);
	rhs_adding.data.resize(len);

	std::transform(lhs_adding.data.begin(), lhs_adding.data.end(),
		rhs_adding.data.begin(),
		lhs_adding.data.begin(),
		logical_oper);

	int res_sign = logical_oper(lhs_adding.signum, rhs_adding.signum) & 2;
	res_sign = -res_sign;

	lhs_adding.signum = (lhs_adding.get_chunk(0) == 0) ? 0 : (res_sign + 1);
	lhs_adding = lhs_adding.convert_to_2c();
	return lhs_adding;
}