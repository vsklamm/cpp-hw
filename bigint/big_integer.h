#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <cstring>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <functional>

#define MAX_CHUNK_NUM UINT32_MAX

using namespace std;

struct big_integer {
private:
	using uint = std::uint32_t;
	using ll = std::int64_t;
	using ull = std::uint64_t;
	using seqset = std::vector<uint>;

	seqset data;

	uint absolute(int a);
	void remove_leading_0(seqset &number);

	big_integer& negate();
	big_integer& invert();

	// multiplication
	void mul_seqset_short(seqset &seq, uint val);
	void mul_this_long_short(uint val);
	big_integer mul_long_short(uint val) const;

	// summation & subtract
	void shifted_summation(seqset const &second, size_t shift);
	big_integer seqset_subtract(big_integer const &lhs, big_integer const &rhs);

	// division
	uint div_long_short(uint val);
	int compare_prefix(big_integer const &first, big_integer const &second, size_t const prefix_size);
	void prefix_subtract(big_integer &first, big_integer const &second, size_t const prefix_size);

	int str_to_bint(const string &str, big_integer &number);

	bool is_zero() const {
		return (signum == 0);
	}

	template<typename FunctorT>
	big_integer apply_logical_operation(
		big_integer *lhs,
		big_integer const &rhs,
		FunctorT logic_oper);

public:
	int signum;

	big_integer();
	big_integer(int a);
	big_integer(uint a);
	big_integer(big_integer const &other) = default;
	explicit big_integer(std::string const &str);
	~big_integer() = default;

	big_integer& operator=(big_integer const& other) = default;

	big_integer& operator+=(big_integer const& rhs);
	big_integer& operator-=(big_integer const& rhs);

	big_integer& operator*=(big_integer const& rhs);
	big_integer& operator/=(big_integer const& rhs);
	big_integer& operator%=(big_integer const& rhs);

	big_integer& operator&=(big_integer const& rhs);
	big_integer& operator|=(big_integer const& rhs);
	big_integer& operator^=(big_integer const& rhs);

	big_integer& operator<<=(int rhs);
	big_integer& operator>>=(int rhs);

	big_integer operator+() const;
	big_integer operator-() const;
	big_integer operator~() const;

	big_integer& operator++();
	big_integer operator++(int);

	big_integer& operator--();
	big_integer operator--(int);

	friend bool operator==(big_integer const& a, big_integer const& b);
	friend bool operator!=(big_integer const& a, big_integer const& b);
	friend bool operator<(big_integer const& a, big_integer const& b);
	friend bool operator>(big_integer const& a, big_integer const& b);
	friend bool operator<=(big_integer const& a, big_integer const& b);
	friend bool operator>=(big_integer const& a, big_integer const& b);

	size_t get_data_size() const {
		return data.size();
	}

	seqset get_data() const {
		return data;
	}

	uint get_chunk(size_t i) const {
		return data[i];
	}

	big_integer convert_to_2c() const;

	friend int compare_abs_numbers(big_integer const &first, big_integer const &second);

	friend std::string to_string(big_integer const& a);
	friend std::string to_string(big_integer const& a, char separator);
};

big_integer operator+(big_integer a, big_integer const& b);
big_integer operator-(big_integer a, big_integer const& b);
big_integer operator*(big_integer a, big_integer const& b);
big_integer operator/(big_integer a, big_integer const& b);
big_integer operator%(big_integer a, big_integer const& b);

big_integer operator&(big_integer a, big_integer const& b);
big_integer operator|(big_integer a, big_integer const& b);
big_integer operator^(big_integer a, big_integer const& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(big_integer const& a, big_integer const& b);
bool operator!=(big_integer const& a, big_integer const& b);
bool operator<(big_integer const& a, big_integer const& b);
bool operator>(big_integer const& a, big_integer const& b);
bool operator<=(big_integer const& a, big_integer const& b);
bool operator>=(big_integer const& a, big_integer const& b);

std::ostream& operator<<(std::ostream& out, big_integer const& a);
std::istream& operator>>(std::istream& in, big_integer& a);

int compare_abs_numbers(big_integer const &first, big_integer const &second);

#endif // BIG_INTEGER_H