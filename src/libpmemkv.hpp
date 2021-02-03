// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_HPP
#define LIBPMEMKV_HPP

#include <cassert>
#include <functional>
#include <iostream>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/string_view.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "libpmemkv.h"
#include <libpmemobj/pool_base.h>

/*! \file libpmemkv.hpp
	\brief Main C++ pmemkv public header.

	It contains all pmemkv public types, enums, classes with their functions and
	members.
*/

/*! \namespace pmem
	\brief Persistent memory namespace.

	It is a common namespace for all persistent memory C++ libraries
	For more information about pmem goto: https://pmem.io
*/
namespace pmem
{
/*! \namespace pmem::kv
	\brief Main pmemkv namespace.

	It contains all pmemkv public types, enums, classes with their functions and
   members. It is located within pmem namespace.
*/
namespace kv
{
using string_view = obj::string_view;

/**
 * The C++ idiomatic function type to use for callback using key-value pair.
 *
 * @param[in] key returned by callback item's key
 * @param[in] value returned by callback item's data
 */
typedef int get_kv_function(string_view key, string_view value);
/**
 * The C++ idiomatic function type to use for callback using only the value.
 * It is used only by non-range get() calls.
 *
 * @param[in] value returned by callback item's data
 */
typedef void get_v_function(string_view value);

typedef int comparator_function(string_view key1, string_view key2);

/**
 * Key-value pair callback, C-style.
 */
using get_kv_callback = pmemkv_get_kv_callback;
/**
 * Value-only callback, C-style.
 */
using get_v_callback = pmemkv_get_v_callback;

/*! \enum status
	\brief Status returned by pmemkv functions.

	Each function, except for db::close() and pmem::kv::errormsg(), returns one of the
	following status codes.
*/
enum class status {
	OK = PMEMKV_STATUS_OK,			     /**< no error */
	UNKNOWN_ERROR = PMEMKV_STATUS_UNKNOWN_ERROR, /**< unknown error */
	NOT_FOUND = PMEMKV_STATUS_NOT_FOUND, /**< record (or config item) not found */
	NOT_SUPPORTED = PMEMKV_STATUS_NOT_SUPPORTED, /**< function is not implemented by
							current engine */
	INVALID_ARGUMENT = PMEMKV_STATUS_INVALID_ARGUMENT, /**< argument to function has
							wrong value */
	CONFIG_PARSING_ERROR =
		PMEMKV_STATUS_CONFIG_PARSING_ERROR, /**< parsing data to config failed */
	CONFIG_TYPE_ERROR =
		PMEMKV_STATUS_CONFIG_TYPE_ERROR, /**< config item has different type than
							expected */
	STOPPED_BY_CB = PMEMKV_STATUS_STOPPED_BY_CB, /**< iteration was stopped by user's
							callback */
	OUT_OF_MEMORY =
		PMEMKV_STATUS_OUT_OF_MEMORY, /**< operation failed because there is not
						enough memory (or space on the device) */
	WRONG_ENGINE_NAME =
		PMEMKV_STATUS_WRONG_ENGINE_NAME, /**< engine name does not match any
							available engine */
	TRANSACTION_SCOPE_ERROR =
		PMEMKV_STATUS_TRANSACTION_SCOPE_ERROR, /**< an error with the scope of the
							libpmemobj transaction */
	DEFRAG_ERROR = PMEMKV_STATUS_DEFRAG_ERROR, /**< the defragmentation process failed
						      (possibly in the middle of a run) */
	COMPARATOR_MISMATCH =
		PMEMKV_STATUS_COMPARATOR_MISMATCH, /**< db was created with a different
						      comparator */
};

inline std::ostream &operator<<(std::ostream &os, const status &s)
{
	static const std::string statuses[] = {"OK",
					       "UNKNOWN_ERROR",
					       "NOT_FOUND",
					       "NOT_SUPPORTED",
					       "INVALID_ARGUMENT",
					       "CONFIG_PARSING_ERROR",
					       "CONFIG_TYPE_ERROR",
					       "STOPPED_BY_CB",
					       "OUT_OF_MEMORY",
					       "WRONG_ENGINE_NAME",
					       "TRANSACTION_SCOPE_ERROR",
					       "DEFRAG_ERROR",
					       "COMPARATOR_MISMATCH"};

	int status_no = static_cast<int>(s);
	os << statuses[status_no] << " (" << status_no << ")";

	return os;
}

/*! \exception bad_result_access
	\brief Defines a type of object to be thrown by result::get_value() when
   result doesn't contain value.
 */
class bad_result_access : public std::runtime_error {
public:
	bad_result_access(const char *what_arg) : std::runtime_error(what_arg)
	{
	}

	const char *what() const noexcept final
	{
		return std::runtime_error::what();
	}

private:
};

/*! \class result
	\brief Stores result of an operation. It always contains status and optionally can
   contain value.

	If result contains value: is_ok() returns true, get_value() returns value,
   get_status() returns status::OK.

	If result contains error: is_ok() returns false, get_value() throws
   bad_result_access, get_status() returns status other than status::OK.
 */
template <typename T>
class result {
	union {
		T value;
	};

	status s;

public:
	result(const T &val) noexcept(noexcept(T(std::declval<T>())));
	result(const status &err) noexcept;
	result(const result &other) noexcept(noexcept(T(std::declval<T>())));
	result(result &&other) noexcept(noexcept(T(std::declval<T>())));
	result(T &&val) noexcept(noexcept(T(std::declval<T>())));

	~result();

	result &
	operator=(const result &other) noexcept(noexcept(std::declval<T>().~T()) &&
						noexcept(T(std::declval<T>())));
	result &operator=(result &&other) noexcept(noexcept(std::declval<T>().~T()) &&
						   noexcept(T(std::declval<T>())));

	bool is_ok() const noexcept;

	const T &get_value() const &;
	T &get_value() &;

	T &&get_value() &&;

	status get_status() const noexcept;
};

/**
 * Creates result with value (status is automatically initialized to status::OK).
 *
 * @param[in] val value.
 */
template <typename T>
result<T>::result(const T &val) noexcept(noexcept(T(std::declval<T>())))
    : value(val), s(status::OK)
{
}

/**
 * Creates result which contains only status.
 *
 * @param[in] status status other than status::OK.
 */
template <typename T>
result<T>::result(const status &status) noexcept : s(status)
{
	assert(s != status::OK);
}

/**
 * Default copy constructor.
 *
 * @param[in] other result to copy.
 */
template <typename T>
result<T>::result(const result &other) noexcept(noexcept(T(std::declval<T>())))
    : s(other.s)
{
	if (s == status::OK)
		new (&value) T(other.value);
}

/**
 * Default move constructor.
 *
 * @param[in] other result to move.
 */
template <typename T>
result<T>::result(result &&other) noexcept(noexcept(T(std::declval<T>()))) : s(other.s)
{
	if (s == status::OK)
		new (&value) T(std::move(other.value));

	other.s = status::UNKNOWN_ERROR;
}

/**
 * Explicit destructor
 */
template <typename T>
result<T>::~result()
{
	if (s == status::OK)
		value.~T();
}

/**
 * Default copy assignment operator.
 *
 * @param[in] other result to copy.
 */
template <typename T>
result<T> &
result<T>::operator=(const result &other) noexcept(noexcept(std::declval<T>().~T()) &&
						   noexcept(T(std::declval<T>())))
{
	if (s == status::OK && other.is_ok())
		value = other.value;
	else if (other.is_ok())
		new (&value) T(other.value);
	else if (s == status::OK)
		value.~T();

	s = other.s;

	return *this;
}

/**
 * Default move assignment operator.
 *
 * @param[in] other result to move.
 */
template <typename T>
result<T> &
result<T>::operator=(result &&other) noexcept(noexcept(std::declval<T>().~T()) &&
					      noexcept(T(std::declval<T>())))
{
	if (s == status::OK && other.is_ok())
		value = std::move(other.value);
	else if (other.is_ok())
		new (&value) T(std::move(other.value));
	else if (s == status::OK)
		value.~T();

	s = other.s;
	other.s = status::UNKNOWN_ERROR;

	return *this;
}

/**
 * Constructor with rvalue reference to T.
 *
 * @param[in] val rvalue reference to T
 */
template <typename T>
result<T>::result(T &&val) noexcept(noexcept(T(std::declval<T>())))
    : value(std::move(val)), s(status::OK)
{
}

/**
 * Checks if the result contains value (status == status::OK).
 *
 * @return bool
 */
template <typename T>
bool result<T>::is_ok() const noexcept
{
	return s == status::OK;
}

/**
 * Returns const reference to value from the result.
 *
 * If result doesn't contain value, throws bad_result_access.
 *
 * @throw bad_result_access
 *
 * @return const reference to value from the result.
 */
template <typename T>
const T &result<T>::get_value() const &
{
	if (s == status::OK)
		return value;
	else
		throw bad_result_access("bad_result_access: value doesn't exist");
}

/**
 * Returns reference to value from the result.
 *
 * If result doesn't contain value, throws bad_result_access.
 *
 * @throw bad_result_access
 *
 * @return reference to value from the result
 */
template <typename T>
T &result<T>::get_value() &
{
	if (s == status::OK)
		return value;
	else
		throw bad_result_access("bad_result_access: value doesn't exist");
}

/**
 * Returns rvalue reference to value from the result.
 *
 * If result doesn't contain value, throws bad_result_access.
 *
 * @throw bad_result_access
 *
 * @return rvalue reference to value from the result
 */
template <typename T>
T &&result<T>::get_value() &&
{
	if (s == status::OK) {
		s = status::UNKNOWN_ERROR;
		return std::move(value);
	} else
		throw bad_result_access("bad_result_access: value doesn't exist");
}

/**
 * Returns status from the result.
 *
 * It returns status::OK if there is a value, and other status (with the appropriate
 * 'error') if there isn't any value.
 *
 * @return status
 */
template <typename T>
status result<T>::get_status() const noexcept
{
	return s;
}

template <typename T>
bool operator==(const result<T> &lhs, const status &rhs)
{
	return lhs.get_status() == rhs;
}

template <typename T>
bool operator==(const status &lhs, const result<T> &rhs)
{
	return lhs == rhs.get_status();
}

template <typename T>
bool operator!=(const result<T> &lhs, const status &rhs)
{
	return lhs.get_status() != rhs;
}

template <typename T>
bool operator!=(const status &lhs, const result<T> &rhs)
{
	return lhs != rhs.get_status();
}

/*! \class config
	\brief Holds configuration parameters for engines.

	It stores mappings of keys (strings) to values. A value can be:
		* uint64_t,
		* int64_t,
		* string,
		* binary data,
		* pointer to an object (with accompanying deleter function).

	It also delivers methods to store and read configuration items provided by
	a user. Once the configuration object is set (with all required
	parameters),pmemkv_open it can be passed to db::open() method.

	List of options which are required by pmemkv database is specific to an engine.
	Every engine has documented all supported config parameters (please see
	[libpmemkv(7)](https://pmem.io/pmemkv/master/manpages/libpmemkv.7.html) for
	details).
*/
class config {
public:
	config() noexcept;
	explicit config(pmemkv_config *cfg) noexcept;

	template <typename T>
	status put_data(const std::string &key, const T *value,
			const std::size_t number = 1) noexcept;

	template <typename T>
	status put_object(const std::string &key, T *value,
			  void (*deleter)(void *)) noexcept;
	template <typename T, typename D>
	status put_object(const std::string &key, std::unique_ptr<T, D> object) noexcept;
	status put_uint64(const std::string &key, std::uint64_t value) noexcept;
	status put_int64(const std::string &key, std::int64_t value) noexcept;
	status put_string(const std::string &key, const std::string &value) noexcept;

	status put_size(std::uint64_t size) noexcept;
	status put_path(const std::string &path) noexcept;
	status put_force_create(bool value) noexcept;
	status put_oid(PMEMoid *oid) noexcept;
	template <typename Comparator>
	status put_comparator(Comparator &&comparator);
	status reserve(std::uint64_t reserve_count) noexcept;

	template <typename T>
	status get_data(const std::string &key, T *&value, std::size_t &number) const
		noexcept;
	template <typename T>
	status get_object(const std::string &key, T *&value) const noexcept;

	status get_uint64(const std::string &key, std::uint64_t &value) const noexcept;
	status get_int64(const std::string &key, std::int64_t &value) const noexcept;
	status get_string(const std::string &key, std::string &value) const noexcept;

	pmemkv_config *release() noexcept;

private:
	int init() noexcept;

	std::unique_ptr<pmemkv_config, decltype(&pmemkv_config_delete)> config_;
};

/*! \class tx
	\brief Pmemkv transaction handle.

	This API is EXPERIMENTAL and might change.

	The tx class allows grouping put and remove operations into a single atomic action
	(with respect to persistence and concurrency). Concurrent engines provide
	transactions with ACID (atomicity, consistency, isolation, durability) properties.
	Transactions for single threaded engines provide atomicity, consistency and
	durability. Actions in a transaction are executed in the order in which they were
	called.

	@snippet examples/pmemkv_transaction_cpp/pmemkv_transaction.cpp transaction
*/
class tx {
public:
	tx(pmemkv_tx *tx_) noexcept;

	status put(string_view key, string_view value) noexcept;
	status remove(string_view key) noexcept;
	status commit() noexcept;
	void abort() noexcept;

private:
	std::unique_ptr<pmemkv_tx, decltype(&pmemkv_tx_end)> tx_;
};

/*! \class db
	\brief Main pmemkv class, it provides functions to operate on data in database.

	Database class for creating, opening and closing pmemkv's data file.
	It provides functions to write, read & remove data, count elements stored
	and check for existence of an element based on its key.

	Note: It does not explicitly provide upper_bound/lower_bound functions.
	If you want to obtain an element(s) above or below the selected key,
	you can use pmem::kv::get_above() or pmem::kv::get_below().
	See descriptions of these functions for details.
*/
class db {
	template <bool IsConst>
	class iterator;

public:
	using read_iterator = iterator<true>;
	using write_iterator = iterator<false>;

	db() noexcept;

	status open(const std::string &engine_name, config &&cfg = config{}) noexcept;

	void close() noexcept;

	status count_all(std::size_t &cnt) noexcept;
	status count_above(string_view key, std::size_t &cnt) noexcept;
	status count_equal_above(string_view key, std::size_t &cnt) noexcept;
	status count_equal_below(string_view key, std::size_t &cnt) noexcept;
	status count_below(string_view key, std::size_t &cnt) noexcept;
	status count_between(string_view key1, string_view key2,
			     std::size_t &cnt) noexcept;

	status get_all(get_kv_callback *callback, void *arg) noexcept;
	status get_all(std::function<get_kv_function> f) noexcept;

	status get_above(string_view key, get_kv_callback *callback, void *arg) noexcept;
	status get_above(string_view key, std::function<get_kv_function> f) noexcept;

	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) noexcept;
	status get_equal_above(string_view key,
			       std::function<get_kv_function> f) noexcept;

	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) noexcept;
	status get_equal_below(string_view key,
			       std::function<get_kv_function> f) noexcept;

	status get_below(string_view key, get_kv_callback *callback, void *arg) noexcept;
	status get_below(string_view key, std::function<get_kv_function> f) noexcept;

	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) noexcept;
	status get_between(string_view key1, string_view key2,
			   std::function<get_kv_function> f) noexcept;

	status exists(string_view key) noexcept;

	status get(string_view key, get_v_callback *callback, void *arg) noexcept;
	status get(string_view key, std::function<get_v_function> f) noexcept;
	status get(string_view key, std::string *value) noexcept;

	status put(string_view key, string_view value) noexcept;
	status remove(string_view key) noexcept;
	status defrag(double start_percent = 0, double amount_percent = 100);

	result<tx> tx_begin() noexcept;

	result<read_iterator> new_read_iterator();
	result<write_iterator> new_write_iterator();

	std::string errormsg();

private:
	std::unique_ptr<pmemkv_db, decltype(&pmemkv_close)> db_;
};

/*! \class db::iterator
	\brief Iterator provides methods to iterate over records in db.

	This API is EXPERIMENTAL and might change.

	It can be only created by methods in db (db::new_read_iterator() - for a read
	iterator, and db::new_write_iterator() for a write iterator).

	Both iterator types (write_iterator and read_iterator) allow reading record's
	key and value. A write_iterator additionally can modify record's value
	transactionally.

	Holding simultaneously in the same thread more than one iterator is undefined
	behavior.

	Example usage of iterators with single-threaded engines:
	@snippet examples/pmemkv_iterator_cpp/pmemkv_iterator.cpp Single-threaded

	Example usage of iterators with concurrent engines:
	@snippet examples/pmemkv_iterator_cpp/pmemkv_iterator.cpp Concurrent
*/
template <bool IsConst>
class db::iterator {
	using iterator_type = typename std::conditional<IsConst, pmemkv_iterator,
							pmemkv_write_iterator>::type;

	template <typename T>
	class OutputIterator;

public:
	iterator(iterator_type *it);

	status seek(string_view key) noexcept;
	status seek_lower(string_view key) noexcept;
	status seek_lower_eq(string_view key) noexcept;
	status seek_higher(string_view key) noexcept;
	status seek_higher_eq(string_view key) noexcept;

	status seek_to_first() noexcept;
	status seek_to_last() noexcept;

	status is_next() noexcept;
	status next() noexcept;
	status prev() noexcept;

	result<string_view> key() noexcept;

	result<string_view>
	read_range(size_t pos = 0,
		   size_t n = std::numeric_limits<size_t>::max()) noexcept;

	template <bool IC = IsConst>
	typename std::enable_if<!IC, result<pmem::obj::slice<OutputIterator<char>>>>::type
	write_range(size_t pos = 0,
		    size_t n = std::numeric_limits<size_t>::max()) noexcept;

	template <bool IC = IsConst>
	typename std::enable_if<!IC, status>::type commit() noexcept;
	template <bool IC = IsConst>
	typename std::enable_if<!IC>::type abort() noexcept;

private:
	std::unique_ptr<
		iterator_type,
		typename std::conditional<IsConst, decltype(&pmemkv_iterator_delete),
					  decltype(&pmemkv_write_iterator_delete)>::type>
		it_;

	pmemkv_iterator *get_raw_it();
};

/*! \class db::iterator::OutputIterator
	\brief OutputIterator provides iteration through elements without a possibility of
	reading them. It is only allowed to modify them.
*/
template <bool IsConst>
template <typename T>
class db::iterator<IsConst>::OutputIterator {
	struct assign_only;

public:
	using reference = assign_only &;
	using pointer = void;
	using difference_type = std::ptrdiff_t;
	using value_type = void;
	using iterator_category = std::output_iterator_tag;

	OutputIterator(T *x);

	reference operator*();

	OutputIterator &operator++();
	OutputIterator operator++(int);

	OutputIterator &operator--();
	OutputIterator operator--(int);

	assign_only operator[](difference_type pos);

	difference_type operator-(const OutputIterator &other) const;

	bool operator!=(const OutputIterator &other) const;

private:
	struct assign_only {
		friend OutputIterator<T>;

		assign_only(T *x);

		assign_only &operator=(const T &x);

	private:
		T *c;
	};

	assign_only ao;
};

template <bool IsConst>
template <typename T>
db::iterator<IsConst>::OutputIterator<T>::OutputIterator(T *x) : ao(x)
{
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T>::reference
	db::iterator<IsConst>::OutputIterator<T>::operator*()
{
	return ao;
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T> &
db::iterator<IsConst>::OutputIterator<T>::operator++()
{
	ao.c += sizeof(T);
	return *this;
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T>
db::iterator<IsConst>::OutputIterator<T>::operator++(int)
{
	auto tmp = *this;
	++(*this);
	return tmp;
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T> &
db::iterator<IsConst>::OutputIterator<T>::operator--()
{
	ao.c -= sizeof(T);
	return *this;
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T>
db::iterator<IsConst>::OutputIterator<T>::operator--(int)
{
	auto tmp = *this;
	--(*this);
	return tmp;
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T>::assign_only
	db::iterator<IsConst>::OutputIterator<T>::operator[](difference_type pos)
{
	return assign_only(ao.c + pos);
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T>::difference_type
db::iterator<IsConst>::OutputIterator<T>::operator-(const OutputIterator &other) const
{
	return this->ao.c - other.ao.c;
}

template <bool IsConst>
template <typename T>
bool db::iterator<IsConst>::OutputIterator<T>::operator!=(
	const OutputIterator &other) const
{
	return this->ao.c != other.ao.c;
}

template <bool IsConst>
template <typename T>
db::iterator<IsConst>::OutputIterator<T>::assign_only::assign_only(T *x) : c(x)
{
}

template <bool IsConst>
template <typename T>
typename db::iterator<IsConst>::template OutputIterator<T>::assign_only &
db::iterator<IsConst>::OutputIterator<T>::assign_only::operator=(const T &x)
{
	*c = x;
	return *this;
}

template <>
inline db::iterator<true>::iterator(iterator_type *it) : it_(it, &pmemkv_iterator_delete)
{
}

template <>
inline db::iterator<false>::iterator(iterator_type *it)
    : it_(it, &pmemkv_write_iterator_delete)
{
}

/**
 * Changes iterator position to the record with given *key*.
 * If the record is present and no errors occurred, returns pmem::kv::status::OK. If the
 * record does not exist, pmem::kv::status::NOT_FOUND is returned and the iterator
 * position is undefined. Other possible return values are described in pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * @param[in] key key that will be equal to the key of the record on the new iterator
 * position
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::seek(string_view key) noexcept
{
	return static_cast<status>(
		pmemkv_iterator_seek(this->get_raw_it(), key.data(), key.size()));
}

/**
 * Changes iterator position to the record with key lower than given *key*.
 * If the record is present and no errors occurred, returns pmem::kv::status::OK. If the
 * record does not exist, pmem::kv::status::NOT_FOUND is returned and the iterator
 * position is undefined. Other possible return values are described in pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * @param[in] key key that will be higher than the key of the record on the new iterator
 * position
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::seek_lower(string_view key) noexcept
{
	return static_cast<status>(
		pmemkv_iterator_seek_lower(this->get_raw_it(), key.data(), key.size()));
}

/**
 * Changes iterator position to the record with key equal or lower than given *key*.
 * If the record is present and no errors occurred, returns pmem::kv::status::OK. If the
 * record does not exist, pmem::kv::status::NOT_FOUND is returned and the iterator
 * position is undefined. Other possible return values are described in pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * @param[in] key key that will be equal or higher than the key of the record on the new
 * iterator position
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::seek_lower_eq(string_view key) noexcept
{
	return static_cast<status>(pmemkv_iterator_seek_lower_eq(this->get_raw_it(),
								 key.data(), key.size()));
}

/**
 * Changes iterator position to the record with key higher than given *key*.
 * If the record is present and no errors occurred, returns pmem::kv::status::OK. If the
 * record does not exist, pmem::kv::status::NOT_FOUND is returned and the iterator
 * position is undefined. Other possible return values are described in pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * @param[in] key key that will be lower than the key of the record on the new iterator
 * position
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::seek_higher(string_view key) noexcept
{
	return static_cast<status>(
		pmemkv_iterator_seek_higher(this->get_raw_it(), key.data(), key.size()));
}

/**
 * Changes iterator position to the record with key equal or higher than given *key*.
 * If the record is present and no errors occurred, returns pmem::kv::status::OK. If the
 * record does not exist, pmem::kv::status::NOT_FOUND is returned and the iterator
 * position is undefined. Other possible return values are described in pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * @param[in] key key that will be equal or lower than the key of the record on the new
 * iterator position
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::seek_higher_eq(string_view key) noexcept
{
	return static_cast<status>(pmemkv_iterator_seek_higher_eq(
		this->get_raw_it(), key.data(), key.size()));
}

/**
 * Changes iterator position to the first record.
 * If db isn't empty, and no errors occurred, returns
 * pmem::kv::status::OK. If db is empty, pmem::kv::status::NOT_FOUND is returned
 * and the iterator position is undefined. Other possible return values are described in
 * pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::seek_to_first() noexcept
{
	return static_cast<status>(pmemkv_iterator_seek_to_first(this->get_raw_it()));
}

/**
 * Changes iterator position to the last record.
 * If db isn't empty, and no errors occurred, returns
 * pmem::kv::status::OK. If db is empty, pmem::kv::status::NOT_FOUND is returned
 * and the iterator position is undefined. Other possible return values are described in
 * pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::seek_to_last() noexcept
{
	return static_cast<status>(pmemkv_iterator_seek_to_last(this->get_raw_it()));
}

/**
 * Checks if there is a next record available. If true is returned, it is guaranteed that
 * iterator.next() will return status::OK, otherwise iterator is already on the last
 * element and iterator.next() will return status::NOT_FOUND.
 *
 * If the iterator is on an undefined position, calling this method is undefined
 * behaviour.
 *
 * @return bool
 */
template <bool IsConst>
inline status db::iterator<IsConst>::is_next() noexcept
{
	return static_cast<status>(pmemkv_iterator_is_next(this->get_raw_it()));
}

/**
 * Changes iterator position to the next record.
 * If the next record exists, returns pmem::kv::status::OK, otherwise
 * pmem::kv::status::NOT_FOUND is returned and the iterator position is undefined. Other
 * possible return values are described in pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * If the iterator is on an undefined position, calling this method is undefined
 * behaviour.
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::next() noexcept
{
	return static_cast<status>(pmemkv_iterator_next(this->get_raw_it()));
}

/**
 * Changes iterator position to the previous record.
 * If the previous record exists, returns pmem::kv::status::OK, otherwise
 * pmem::kv::status::NOT_FOUND is returned and the iterator position is undefined. Other
 * possible return values are described in pmem::kv::status.
 *
 * It internally aborts all changes made to an element previously pointed by the iterator.
 *
 * If the iterator is on an undefined position, calling this method is undefined
 * behaviour.
 *
 * @return pmem::kv::status
 */
template <bool IsConst>
inline status db::iterator<IsConst>::prev() noexcept
{
	return static_cast<status>(pmemkv_iterator_prev(this->get_raw_it()));
}

/**
 * Returns record's key (pmem::kv::string_view), in
 * pmem::kv::result<pmem::kv::string_view>.
 *
 * If the iterator is on an undefined position, calling this method is undefined
 * behaviour.
 *
 * @return pmem::kv::result<pmem::kv::string_view>
 */
template <bool IsConst>
inline result<string_view> db::iterator<IsConst>::key() noexcept
{
	const char *c;
	size_t size;
	auto s = static_cast<status>(pmemkv_iterator_key(this->get_raw_it(), &c, &size));

	if (s == status::OK)
		return {string_view{c, size}};
	else
		return {s};
}

/**
 * Returns value's range (pmem::kv::string_view) to read, in pmem::kv::result.
 *
 * It is only used to read a value. If you want to modify the value, use
 * db::iterator::write_range instead.
 *
 * If the iterator is on an undefined position, calling this method is undefined
 * behaviour.
 *
 * @param[in] pos position of the element in a value which will be the first element in
 * the returned range (default = 0)
 * @param[in] n number of elements in range (default = std::numeric_limits<size_t>::max(),
 * if n is bigger than the length of a value it's automatically shrinked)
 *
 * @return pmem::kv::result<pmem::kv::string_view>
 */
template <bool IsConst>
inline result<string_view> db::iterator<IsConst>::read_range(size_t pos,
							     size_t n) noexcept
{
	const char *data;
	size_t size;
	auto s = static_cast<status>(
		pmemkv_iterator_read_range(this->get_raw_it(), pos, n, &data, &size));

	if (s == status::OK)
		return {string_view{data, size}};
	else
		return {s};
}

/**
 * Returns value's range (pmem::obj::slice<db::iterator::OutputIterator<char>>) to modify,
 * in pmem::kv::result.
 *
 * It is only used to modify a value. If you want to read the value, use
 * db::iterator::read_range instead.
 *
 * Changes made on a requested range are not persistent until db::iterator::commit is
 * called.
 *
 * If iterator is on an undefined position, calling this method is undefined behaviour.
 *
 * @param[in] pos position of the element in a value which will be the first element in
 * the returned range (default = 0)
 * @param[in] n number of elements in range (default = std::numeric_limits<size_t>::max(),
 * if n is bigger than the length of a value it's automatically shrinked)
 *
 * @return pmem::kv::result<pmem::obj::slice<db::iterator::OutputIterator<char>>>
 */
template <>
template <>
inline result<pmem::obj::slice<db::iterator<false>::OutputIterator<char>>>
db::iterator<false>::write_range(size_t pos, size_t n) noexcept
{
	char *data;
	size_t size;
	auto s = static_cast<status>(
		pmemkv_write_iterator_write_range(this->it_.get(), pos, n, &data, &size));

	if (s == status::OK) {
		try {
			return {{data, data + size}};
		} catch (std::out_of_range &e) {
			return {status::INVALID_ARGUMENT};
		}
	} else
		return {s};
}

/**
 * Commits modifications made on the current record.
 *
 * Calling this method is the only way to save modifications made by the iterator on the
 * current record. You need to call this method before changing the iterator position,
 * otherwise modifications will be automatically aborted.
 *
 * @return pmem::kv::status
 */
template <>
template <>
inline status db::iterator<false>::commit() noexcept
{
	auto s = static_cast<status>(pmemkv_write_iterator_commit(this->it_.get()));
	return s;
}

/**
 * Aborts uncommitted modifications made on the current record.
 */
template <>
template <>
inline void db::iterator<false>::abort() noexcept
{
	pmemkv_write_iterator_abort(this->it_.get());
}

template <>
inline pmemkv_iterator *db::iterator<true>::get_raw_it()
{
	return it_.get();
}

template <>
inline pmemkv_iterator *db::iterator<false>::get_raw_it()
{
	return it_.get()->iter;
}

/*! \namespace pmem::kv::internal
	\brief internal pmemkv classes for C++ API

	Nothing from this namespace should be used by the users.
	It holds pmemkv internal classes which might be changed or
	removed in future.
*/
namespace internal
{

/*
 * Abstracts unique_ptr - exposes only void *get() method and a destructor.
 * This class is needed for C callbacks which cannot be templated
 * (type of object and deleter must be abstracted away).
 */
struct unique_ptr_wrapper_base {
	virtual ~unique_ptr_wrapper_base()
	{
	}

	virtual void *get() = 0;
};

template <typename T, typename D>
struct unique_ptr_wrapper : public unique_ptr_wrapper_base {
	unique_ptr_wrapper(std::unique_ptr<T, D> ptr) : ptr(std::move(ptr))
	{
	}

	void *get() override
	{
		return ptr.get();
	}

	std::unique_ptr<T, D> ptr;
};

class comparator_base {
public:
	virtual ~comparator_base()
	{
	}
	virtual int compare(string_view key1, string_view key2) = 0;
};

template <typename Comparator>
struct comparator_wrapper : public comparator_base {
	comparator_wrapper(const Comparator &cmp) : cmp(cmp)
	{
	}

	comparator_wrapper(Comparator &&cmp) : cmp(std::move(cmp))
	{
	}

	int compare(string_view key1, string_view key2) override
	{
		return cmp.compare(key1, key2);
	}

	Comparator cmp;
};

struct comparator_config_entry : public unique_ptr_wrapper_base {
	comparator_config_entry(
		std::unique_ptr<comparator_base> ptr,
		std::unique_ptr<pmemkv_comparator, decltype(pmemkv_comparator_delete) *>
			c_cmp)
	    : ptr(std::move(ptr)), c_cmp(std::move(c_cmp))
	{
	}

	void *get() override
	{
		return c_cmp.get();
	}

	std::unique_ptr<comparator_base> ptr;
	std::unique_ptr<pmemkv_comparator, decltype(pmemkv_comparator_delete) *> c_cmp;
};

/*
 * All functions which will be called by C code must be declared as extern "C"
 * to ensure they have C linkage. It is needed because it is possible that
 * C and C++ functions use different calling conventions.
 */
extern "C" {
static inline void call_up_destructor(void *object)
{
	auto *ptr = static_cast<unique_ptr_wrapper_base *>(object);
	delete ptr;
}

static inline void *call_up_get(void *object)
{
	auto *ptr = static_cast<unique_ptr_wrapper_base *>(object);
	return ptr->get();
}

static inline int call_comparator_function(const char *k1, size_t kb1, const char *k2,
					   size_t kb2, void *arg)
{
	auto *cmp = static_cast<comparator_base *>(arg);
	return cmp->compare(string_view(k1, kb1), string_view(k2, kb2));
}
} /* extern "C" */
} /* namespace internal */

/**
 * Default constructor with uninitialized config.
 */
inline config::config() noexcept : config_(nullptr, &pmemkv_config_delete)
{
}

/**
 * Creates config from pointer to pmemkv_config.
 * Ownership is transferred to config class.
 */
inline config::config(pmemkv_config *cfg) noexcept : config_(cfg, &pmemkv_config_delete)
{
}

/**
 * Initialization function for config.
 * It's lazy initialized and called within all put functions.
 *
 * @return int initialization result; 0 on success
 */
inline int config::init() noexcept
{
	if (this->config_.get() == nullptr) {
		this->config_ = {pmemkv_config_new(), &pmemkv_config_delete};

		if (this->config_.get() == nullptr)
			return 1;
	}

	return 0;
}

/**
 * Puts binary data pointed by value, of type T, with count of elements
 * to a config. Count parameter is useful for putting arrays of data.
 *
 * @param[in] key The string representing config item's name.
 * @param[in] value The pointer to data.
 * @param[in] count The count of elements stored under reference.
 *
 * @return pmem::kv::status
 */
template <typename T>
inline status config::put_data(const std::string &key, const T *value,
			       const std::size_t count) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	return static_cast<status>(pmemkv_config_put_data(
		this->config_.get(), key.data(), (void *)value, count * sizeof(T)));
}

/**
 * Puts object pointed by value, of type T, with given destructor
 * to a config.
 *
 * @param[in] key The string representing config item's name.
 * @param[in] value The pointer to object.
 * @param[in] deleter The object's destructor function.
 *
 * @return pmem::kv::status
 */
template <typename T>
inline status config::put_object(const std::string &key, T *value,
				 void (*deleter)(void *)) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	return static_cast<status>(pmemkv_config_put_object(
		this->config_.get(), key.data(), (void *)value, deleter));
}

/**
 * Puts unique_ptr (to an object) to a config.
 *
 * @param[in] key The string representing config item's name.
 * @param[in] object unique_ptr to an object.
 *
 * @return pmem::kv::status
 */
template <typename T, typename D>
inline status config::put_object(const std::string &key,
				 std::unique_ptr<T, D> object) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	internal::unique_ptr_wrapper_base *wrapper;

	try {
		wrapper = new internal::unique_ptr_wrapper<T, D>(std::move(object));
	} catch (std::bad_alloc &e) {
		return status::OUT_OF_MEMORY;
	} catch (...) {
		return status::UNKNOWN_ERROR;
	}

	return static_cast<status>(pmemkv_config_put_object_cb(
		this->config_.get(), key.data(), (void *)wrapper, internal::call_up_get,
		internal::call_up_destructor));
}

/**
 * Puts comparator object to a config.
 *
 * Comparator must:
 * - implement `int compare(pmem::kv::string_view, pmem::kv::string_view)`
 * - implement `std::string name()`
 * - be copy or move constructible
 * - be thread safe
 *
 * @param[in] comparator forwarding reference to a comparator
 *
 * @return pmem::kv::status
 *
 * Example of custom comparator:
 * @snippet examples/pmemkv_comparator_cpp/pmemkv_comparator.cpp Custom comparator
 */
template <typename Comparator>
inline status config::put_comparator(Comparator &&comparator)
{
	static_assert(
		std::is_same<decltype(std::declval<Comparator>().compare(
				     std::declval<string_view>(),
				     std::declval<string_view>())),
			     int>::value,
		"Comparator should implement `int compare(pmem::kv::string_view, pmem::kv::string_view)` method");
	static_assert(std::is_convertible<decltype(std::declval<Comparator>().name()),
					  std::string>::value,
		      "Comparator should implement `std::string name()` method");

	std::unique_ptr<internal::comparator_base> wrapper;

	try {
		wrapper = std::unique_ptr<internal::comparator_base>(
			new internal::comparator_wrapper<Comparator>(
				std::forward<Comparator>(comparator)));
	} catch (std::bad_alloc &e) {
		return status::OUT_OF_MEMORY;
	} catch (...) {
		return status::UNKNOWN_ERROR;
	}

	auto cmp =
		std::unique_ptr<pmemkv_comparator, decltype(pmemkv_comparator_delete) *>(
			pmemkv_comparator_new(&internal::call_comparator_function,
					      std::string(comparator.name()).c_str(),
					      wrapper.get()),
			&pmemkv_comparator_delete);
	if (cmp == nullptr)
		return status::UNKNOWN_ERROR;

	internal::unique_ptr_wrapper_base *entry;

	try {
		entry = new internal::comparator_config_entry(std::move(wrapper),
							      std::move(cmp));
	} catch (std::bad_alloc &e) {
		return status::OUT_OF_MEMORY;
	} catch (...) {
		return status::UNKNOWN_ERROR;
	}

	return static_cast<status>(pmemkv_config_put_object_cb(
		this->config_.get(), "comparator", (void *)entry, internal::call_up_get,
		internal::call_up_destructor));
}

/**
 * Puts std::uint64_t value to a config.
 *
 * @param[in] key The string representing config item's name.
 * @param[in] value The std::uint64_t value.
 *
 * @return pmem::kv::status
 */
inline status config::put_uint64(const std::string &key, std::uint64_t value) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	return static_cast<status>(
		pmemkv_config_put_uint64(this->config_.get(), key.data(), value));
}

/**
 * Puts std::int64_t value to a config.
 *
 * @param[in] key The string representing config item's name.
 * @param[in] value The std::int64_t value.
 *
 * @return pmem::kv::status
 */
inline status config::put_int64(const std::string &key, std::int64_t value) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	return static_cast<status>(
		pmemkv_config_put_int64(this->config_.get(), key.data(), value));
}

/**
 * Puts string value to a config.
 *
 * @param[in] key The string representing config item's name.
 * @param[in] value The string value.
 *
 * @return pmem::kv::status
 */
inline status config::put_string(const std::string &key,
				 const std::string &value) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	return static_cast<status>(
		pmemkv_config_put_string(this->config_.get(), key.data(), value.data()));
}

/**
 * Puts size to a config
 *
 * @param[in] size of the database in bytes.
 *
 * @return pmem::kv::status
 */
inline status config::put_size(std::uint64_t size) noexcept
{
	return put_uint64("size", size);
}

/**
 * Puts path to a config.
 *
 * @param[in] path to a database file or to a poolset file (see **poolset**(5) for
 * details). Note that when using poolset file, size should be 0.
 *
 * @return pmem::kv::status
 */
inline status config::put_path(const std::string &path) noexcept
{
	return put_string("path", path);
}

/**
 * Puts force_create parameter to a config, For supporting engines If false,
 * pmemkv opens file specified by 'path', otherwise it creates it. False by
 * default.
 *
 * @return pmem::kv::status
 */
inline status config::put_force_create(bool value) noexcept
{
	return put_uint64("force_create", value ? 1 : 0);
}

/**
 * Puts PMEMoid object to a config
 *
 * @param[in] oid pointer (for details see **libpmemobj**(7)) which points to the engine
 * data. If oid is null engine will allocate new data, otherwise it will use existing
 * one.
 *
 * @return pmem::kv::status
 */
inline status config::put_oid(PMEMoid *oid) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	return static_cast<status>(pmemkv_config_put_oid(this->config_.get(), oid));
}

/**
 * Puts **reserve** value to a config. It specifies demanded count of elements to reserve
 * space/memory in the database at pmem::kv::open() call. 0 means no reserving, any other
 * number should increase container's capacity (or leave it as is if the value is less or
 * equal to the current capacity). If reservation exceeds (database's) available storage,
 * pmem::kv::open() will return pmem::kv::OUT_OF_MEMORY status.
 *
 * @param[in] reserve_count The count of elements to reserve space/memory for, in the
 * database.
 *
 * @return pmem::kv::status
 */
inline status config::reserve(std::uint64_t reserve_count) noexcept
{
	if (init() != 0)
		return status::UNKNOWN_ERROR;

	return put_uint64("reserve", reserve_count);
}

/**
 * Gets object from a config item with key name and copies it
 * into T object value.
 *
 * @param[in] key The string representing config item's name.
 * @param[out] value The pointer to data.
 * @param[out] count The count of elements stored under reference.
 *
 * @return pmem::kv::status
 */
template <typename T>
inline status config::get_data(const std::string &key, T *&value,
			       std::size_t &count) const noexcept
{
	if (this->config_.get() == nullptr)
		return status::NOT_FOUND;

	std::size_t size;
	auto s = static_cast<status>(pmemkv_config_get_data(
		this->config_.get(), key.data(), (const void **)&value, &size));

	if (s != status::OK)
		return s;

	count = size / sizeof(T);

	return status::OK;
}

/**
 * Gets binary data from a config item with key name and
 * assigns pointer to T object value.
 *
 * @param[in] key The string representing config item's name.
 * @param[out] value The pointer to object.
 *
 * @return pmem::kv::status
 */
template <typename T>
inline status config::get_object(const std::string &key, T *&value) const noexcept
{
	if (this->config_.get() == nullptr)
		return status::NOT_FOUND;

	auto s = static_cast<status>(pmemkv_config_get_object(
		this->config_.get(), key.data(), (void **)&value));

	return s;
}

/**
 * Gets std::uint64_t value from a config item with key name.
 *
 * @param[in] key The string representing config item's name.
 * @param[out] value The std::uint64_t value.
 *
 * @return pmem::kv::status
 */
inline status config::get_uint64(const std::string &key, std::uint64_t &value) const
	noexcept
{
	if (this->config_.get() == nullptr)
		return status::NOT_FOUND;

	return static_cast<status>(
		pmemkv_config_get_uint64(this->config_.get(), key.data(), &value));
}

/**
 * Gets std::int64_t value from a config item with key name.
 *
 * @param[in] key The string representing config item's name.
 * @param[out] value The std::int64_t value.
 *
 * @return pmem::kv::status
 */
inline status config::get_int64(const std::string &key, std::int64_t &value) const
	noexcept
{
	if (this->config_.get() == nullptr)
		return status::NOT_FOUND;

	return static_cast<status>(
		pmemkv_config_get_int64(this->config_.get(), key.data(), &value));
}

/**
 * Gets string value from a config item with key name.
 *
 * @param[in] key The string representing config item's name.
 * @param[out] value The string value.
 *
 * @return pmem::kv::status
 */
inline status config::get_string(const std::string &key, std::string &value) const
	noexcept
{
	if (this->config_.get() == nullptr)
		return status::NOT_FOUND;

	const char *data;

	auto s = static_cast<status>(
		pmemkv_config_get_string(this->config_.get(), key.data(), &data));

	if (s != status::OK)
		return s;

	value = data;

	return status::OK;
}

/**
 * Similarly to std::unique_ptr::release it passes the ownership
 * of underlying pmemkv_config variable and sets it to nullptr.
 *
 * @return handle to pmemkv_config
 */
inline pmemkv_config *config::release() noexcept
{
	return this->config_.release();
}

/**
 * Constructs C++ tx object from a C pmemkv_tx pointer
 */
inline tx::tx(pmemkv_tx *tx_) noexcept : tx_(tx_, &pmemkv_tx_end)
{
}

/**
 * Removes from database record with given *key*. The removed element is still
 * visible until commit. This function will succeed even if there is no element in the
 * database.
 *
 * @param[in] key record's key to query for, to be removed
 *
 * @return pmem::kv::status
 */
inline status tx::remove(string_view key) noexcept
{
	return static_cast<status>(pmemkv_tx_remove(tx_.get(), key.data(), key.size()));
}

/**
 * Inserts a key-value pair into pmemkv database. The inserted elements are not
 * visible (not even in the same thread) until commit.
 *
 * @param[in] key record's key; record will be put into database under its name
 * @param[in] value data to be inserted into this new database record
 *
 * @return pmem::kv::status
 */
inline status tx::put(string_view key, string_view value) noexcept
{
	return static_cast<status>(pmemkv_tx_put(tx_.get(), key.data(), key.size(),
						 value.data(), value.size()));
}

/**
 * Commits the transaction. All operations of this transaction are applied as
 * a single power fail-safe atomic action. The tx object can be safely used after
 * commit.
 *
 * @return pmem::kv::status
 */
inline status tx::commit() noexcept
{
	return static_cast<status>(pmemkv_tx_commit(tx_.get()));
}

/**
 * Aborts the transaction. The tx object can be safely used after
 * abort.
 */
inline void tx::abort() noexcept
{
	pmemkv_tx_abort(tx_.get());
}

/*
 * All functions which will be called by C code must be declared as extern "C"
 * to ensure they have C linkage. It is needed because it is possible that
 * C and C++ functions use different calling conventions.
 */
extern "C" {
static inline int call_get_kv_function(const char *key, size_t keybytes,
				       const char *value, size_t valuebytes, void *arg)
{
	return (*reinterpret_cast<std::function<get_kv_function> *>(arg))(
		string_view(key, keybytes), string_view(value, valuebytes));
}

static inline void call_get_v_function(const char *value, size_t valuebytes, void *arg)
{
	(*reinterpret_cast<std::function<get_v_function> *>(arg))(
		string_view(value, valuebytes));
}

static inline void call_get_copy(const char *v, size_t vb, void *arg)
{
	auto c = reinterpret_cast<std::string *>(arg);
	c->assign(v, vb);
}
}

/**
 * Default constructor with uninitialized database.
 */
inline db::db() noexcept : db_(nullptr, &pmemkv_close)
{
}

/**
 * Opens the pmemkv database with specified config.
 *
 * @param[in] engine_name name of the engine to work with
 * @param[in] cfg pmem::kv::config with parameters specified for the engine
 *
 * @return pmem::kv::status
 */
inline status db::open(const std::string &engine_name, config &&cfg) noexcept
{
	pmemkv_db *db;
	auto s =
		static_cast<status>(pmemkv_open(engine_name.c_str(), cfg.release(), &db));
	if (s == pmem::kv::status::OK)
		this->db_.reset(db);
	return s;
}

/**
 * Closes pmemkv database.
 */
inline void db::close() noexcept
{
	this->db_.reset(nullptr);
}

/**
 * It returns number of currently stored elements in pmem::kv::db.
 *
 * @param[out] cnt number of records stored in pmem::kv::db.
 *
 * @return pmem::kv::status
 */
inline status db::count_all(std::size_t &cnt) noexcept
{
	return static_cast<status>(pmemkv_count_all(this->db_.get(), &cnt));
}

/**
 * It returns number of currently stored elements in pmem::kv::db, whose keys
 * are greater than the given *key*.
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the lower bound of counting
 * @param[out] cnt number of records in pmem::kv::db matching query
 *
 * @return pmem::kv::status
 */
inline status db::count_above(string_view key, std::size_t &cnt) noexcept
{
	return static_cast<status>(
		pmemkv_count_above(this->db_.get(), key.data(), key.size(), &cnt));
}

/**
 * It returns number of currently stored elements in pmem::kv::db, whose keys
 * are greater than or equal to the given *key*.
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the lower bound of counting
 * @param[out] cnt number of records in pmem::kv::db matching query
 *
 * @return pmem::kv::status
 */
inline status db::count_equal_above(string_view key, std::size_t &cnt) noexcept
{
	return static_cast<status>(
		pmemkv_count_equal_above(this->db_.get(), key.data(), key.size(), &cnt));
}

/**
 * It returns number of currently stored elements in pmem::kv::db, whose keys
 * are lower than or equal to the given *key*.
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the lower bound of counting
 * @param[out] cnt number of records in pmem::kv::db matching query
 *
 * @return pmem::kv::status
 */
inline status db::count_equal_below(string_view key, std::size_t &cnt) noexcept
{
	return static_cast<status>(
		pmemkv_count_equal_below(this->db_.get(), key.data(), key.size(), &cnt));
}

/**
 * It returns number of currently stored elements in pmem::kv::db, whose keys
 * are less than the given *key*.
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the upper bound of counting
 * @param[out] cnt number of records in pmem::kv::db matching query
 *
 * @return pmem::kv::status
 */
inline status db::count_below(string_view key, std::size_t &cnt) noexcept
{
	return static_cast<status>(
		pmemkv_count_below(this->db_.get(), key.data(), key.size(), &cnt));
}

/**
 * It returns number of currently stored elements in pmem::kv::db, whose keys
 * are greater than the *key1* and less than the *key2*.
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key1 sets the lower bound of counting
 * @param[in] key2 sets the upper bound of counting
 * @param[out] cnt number of records in pmem::kv::db matching query
 *
 * @return pmem::kv::status
 */
inline status db::count_between(string_view key1, string_view key2,
				std::size_t &cnt) noexcept
{
	return static_cast<status>(pmemkv_count_between(this->db_.get(), key1.data(),
							key1.size(), key2.data(),
							key2.size(), &cnt));
}

/**
 * Executes (C-like) *callback* function for every record stored in pmem::kv::db.
 * Arguments passed to the callback function are: pointer to a key, size of the
 * key, pointer to a value, size of the value and *arg* specified by the user.
 * Callback can stop iteration by returning non-zero value. In that case *get_all()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * @param[in] callback function to be called for every element stored in db
 * @param[in] arg additional arguments to be passed to callback
 *
 * @return pmem::kv::status
 */
inline status db::get_all(get_kv_callback *callback, void *arg) noexcept
{
	return static_cast<status>(pmemkv_get_all(this->db_.get(), callback, arg));
}

/**
 * Executes function for every record stored in pmem::kv::db.
 * Callback can stop iteration by returning non-zero value. In that case *get_all()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * @param[in] f function called for each returned element, it is called with params:
 *				key and value
 *
 * @return pmem::kv::status
 */
inline status db::get_all(std::function<get_kv_function> f) noexcept
{
	return static_cast<status>(
		pmemkv_get_all(this->db_.get(), call_get_kv_function, &f));
}

/**
 * Executes (C-like) callback function for every record stored in pmem::kv::db,
 * whose keys are greater than the given *key*.
 * Arguments passed to the callback function are: pointer to a key, size of the
 * key, pointer to a value, size of the value and *arg* specified by the user.
 * Callback can stop iteration by returning non-zero value. In that case *get_above()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the lower bound for querying
 * @param[in] callback function to be called for each returned element
 * @param[in] arg additional arguments to be passed to callback
 *
 * @return pmem::kv::status
 */
inline status db::get_above(string_view key, get_kv_callback *callback,
			    void *arg) noexcept
{
	return static_cast<status>(
		pmemkv_get_above(this->db_.get(), key.data(), key.size(), callback, arg));
}

/**
 * Executes function for every record stored in pmem::kv::db, whose keys are
 * greater than the given *key*.
 * Callback can stop iteration by returning non-zero value. In that case *get_above()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the lower bound for querying
 * @param[in] f function called for each returned element, it is called with params:
 *				key and value
 *
 * @return pmem::kv::status
 */
inline status db::get_above(string_view key, std::function<get_kv_function> f) noexcept
{
	return static_cast<status>(pmemkv_get_above(
		this->db_.get(), key.data(), key.size(), call_get_kv_function, &f));
}

/**
 * Executes (C-like) callback function for every record stored in pmem::kv::db,
 * whose keys are greater than or equal to the given *key*.
 * Arguments passed to the callback function are: pointer to a key, size of the
 * key, pointer to a value, size of the value and *arg* specified by the user.
 * Callback can stop iteration by returning non-zero value. In that case
 * *get_equal_above()* returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues
 * iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the lower bound for querying
 * @param[in] callback function to be called for each returned element
 * @param[in] arg additional arguments to be passed to callback
 *
 * @return pmem::kv::status
 */
inline status db::get_equal_above(string_view key, get_kv_callback *callback,
				  void *arg) noexcept
{
	return static_cast<status>(pmemkv_get_equal_above(this->db_.get(), key.data(),
							  key.size(), callback, arg));
}

/**
 * Executes function for every record stored in pmem::kv::db, whose keys are
 * greater than or equal to the given *key*.
 * Callback can stop iteration by returning non-zero value. In that case
 **get_equal_above()* returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues
 *iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the lower bound for querying
 * @param[in] f function called for each returned element, it is called with params:
 *				key and value
 *
 * @return pmem::kv::status
 */
inline status db::get_equal_above(string_view key,
				  std::function<get_kv_function> f) noexcept
{
	return static_cast<status>(pmemkv_get_equal_above(
		this->db_.get(), key.data(), key.size(), call_get_kv_function, &f));
}

/**
 * Executes (C-like) callback function for every record stored in pmem::kv::db,
 * whose keys are lower than or equal to the given *key*.
 * Arguments passed to the callback function are: pointer to a key, size of the
 * key, pointer to a value, size of the value and *arg* specified by the user.
 * Callback can stop iteration by returning non-zero value. In that case
 * *get_equal_below()* returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues
 * iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the upper bound for querying
 * @param[in] callback function to be called for each returned element
 * @param[in] arg additional arguments to be passed to callback
 *
 * @return pmem::kv::status
 */
inline status db::get_equal_below(string_view key, get_kv_callback *callback,
				  void *arg) noexcept
{
	return static_cast<status>(pmemkv_get_equal_below(this->db_.get(), key.data(),
							  key.size(), callback, arg));
}

/**
 * Executes function for every record stored in pmem::kv::db, whose keys are
 * lower than or equal to the given *key*.
 * Callback can stop iteration by returning non-zero value. In that case
 **get_equal_below()* returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues
 *iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the upper bound for querying
 * @param[in] f function called for each returned element, it is called with params:
 *				key and value
 *
 * @return pmem::kv::status
 */
inline status db::get_equal_below(string_view key,
				  std::function<get_kv_function> f) noexcept
{
	return static_cast<status>(pmemkv_get_equal_below(
		this->db_.get(), key.data(), key.size(), call_get_kv_function, &f));
}

/**
 * Executes (C-like) callback function for every record stored in pmem::kv::db,
 * whose keys are lower than the given *key*.
 * Arguments passed to the callback function are: pointer to a key, size of the
 * key, pointer to a value, size of the value and *arg* specified by the user.
 * Callback can stop iteration by returning non-zero value. In that case *get_below()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the upper bound for querying
 * @param[in] callback function to be called for each returned element
 * @param[in] arg additional arguments to be passed to callback
 *
 * @return pmem::kv::status
 */
inline status db::get_below(string_view key, get_kv_callback *callback,
			    void *arg) noexcept
{
	return static_cast<status>(
		pmemkv_get_below(this->db_.get(), key.data(), key.size(), callback, arg));
}

/**
 * Executes function for every record stored in pmem::kv::db, whose keys are
 * less than the given *key*.
 * Callback can stop iteration by returning non-zero value. In that case *get_below()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key sets the upper bound for querying
 * @param[in] f function called for each returned element, it is called with params:
 *				key and value
 *
 * @return pmem::kv::status
 */
inline status db::get_below(string_view key, std::function<get_kv_function> f) noexcept
{
	return static_cast<status>(pmemkv_get_below(
		this->db_.get(), key.data(), key.size(), call_get_kv_function, &f));
}

/**
 * Executes (C-like) callback function for every record stored in pmem::kv::db,
 * whose keys are greater than the *key1* and less than the *key2*.
 * Arguments passed to the callback function are: pointer to a key, size of the
 * key, pointer to a value, size of the value and *arg* specified by the user.
 * Callback can stop iteration by returning non-zero value. In that case *get_between()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key1 sets the lower bound for querying
 * @param[in] key2 sets the upper bound for querying
 * @param[in] callback function to be called for each returned element
 * @param[in] arg additional arguments to be passed to callback
 *
 * @return pmem::kv::status
 */
inline status db::get_between(string_view key1, string_view key2,
			      get_kv_callback *callback, void *arg) noexcept
{
	return static_cast<status>(pmemkv_get_between(this->db_.get(), key1.data(),
						      key1.size(), key2.data(),
						      key2.size(), callback, arg));
}
/**
 * Executes function for every record stored in pmem::kv::db, whose keys
 * are greater than the *key1* and less than the *key2*.
 * Callback can stop iteration by returning non-zero value. In that case *get_between()*
 * returns pmem::kv::status::STOPPED_BY_CB. Returning 0 continues iteration.
 *
 * Keys are sorted in order specified by a comparator.
 *
 * @param[in] key1 sets the lower bound for querying
 * @param[in] key2 sets the upper bound for querying
 * @param[in] f function called for each returned element, it is called with params:
 *				key and value
 *
 * @return pmem::kv::status
 */
inline status db::get_between(string_view key1, string_view key2,
			      std::function<get_kv_function> f) noexcept
{
	return static_cast<status>(
		pmemkv_get_between(this->db_.get(), key1.data(), key1.size(), key2.data(),
				   key2.size(), call_get_kv_function, &f));
}

/**
 * Checks existence of record with given *key*. If record is present
 * pmem::kv::status::OK is returned, otherwise pmem::kv::status::NOT_FOUND
 * is returned. Other possible return values are described in pmem::kv::status.
 *
 * @param[in] key record's key to query for in database
 *
 * @return pmem::kv::status
 */
inline status db::exists(string_view key) noexcept
{
	return static_cast<status>(
		pmemkv_exists(this->db_.get(), key.data(), key.size()));
}

/**
 * Executes (C-like) *callback* function for record with given *key*. If
 * record is present and no error occurred, the function returns
 * pmem::kv::status::OK. If record does not exist pmem::kv::status::NOT_FOUND
 * is returned. Other possible return values are described in pmem::kv::status.
 * *Callback* is called with the following parameters:
 * pointer to a value, size of the value and *arg* specified by the user.
 * This function is guaranteed to be implemented by all engines.
 *
 * @param[in] key record's key to query for
 * @param[in] callback function to be called for returned element
 * @param[in] arg additional arguments to be passed to callback
 *
 * @return pmem::kv::status
 */
inline status db::get(string_view key, get_v_callback *callback, void *arg) noexcept
{
	return static_cast<status>(
		pmemkv_get(this->db_.get(), key.data(), key.size(), callback, arg));
}

/**
 * Executes function for record with given *key*. If record is present and
 * no error occurred the function returns pmem::kv::status::OK. If record does
 * not exist pmem::kv::status::NOT_FOUND is returned.
 *
 * @param[in] key record's key to query for
 * @param[in] f function called for returned element, it is called with only
 *				one param - value (key is known)
 *
 * @return pmem::kv::status
 */
inline status db::get(string_view key, std::function<get_v_function> f) noexcept
{
	return static_cast<status>(pmemkv_get(this->db_.get(), key.data(), key.size(),
					      call_get_v_function, &f));
}

/**
 * Gets value copy of record with given *key*. In absence of any errors,
 * pmem::kv::status::OK is returned.
 * This function is guaranteed to be implemented by all engines.
 *
 * @param[in] key record's key to query for
 * @param[out] value stores returned copy of the data
 *
 * @return pmem::kv::status
 */
inline status db::get(string_view key, std::string *value) noexcept
{
	return static_cast<status>(pmemkv_get(this->db_.get(), key.data(), key.size(),
					      call_get_copy, value));
}

/**
 * Inserts a key-value pair into pmemkv database.
 * This function is guaranteed to be implemented by all engines.
 *
 * @param[in] key record's key; record will be put into database under its name
 * @param[in] value data to be inserted into this new database record
 *
 * @return pmem::kv::status
 */
inline status db::put(string_view key, string_view value) noexcept
{
	return static_cast<status>(pmemkv_put(this->db_.get(), key.data(), key.size(),
					      value.data(), value.size()));
}

/**
 * Removes from database record with given *key*.
 * This function is guaranteed to be implemented by all engines.
 *
 * @param[in] key record's key to query for, to be removed
 *
 * @return pmem::kv::status
 */
inline status db::remove(string_view key) noexcept
{
	return static_cast<status>(
		pmemkv_remove(this->db_.get(), key.data(), key.size()));
}

/**
 * Defragments approximately 'amount_percent' percent of elements
 * in the database starting from 'start_percent' percent of elements.
 *
 * @param[in] start_percent starting percent of elements to defragment from
 * @param[in] amount_percent amount percent of elements to defragment
 *
 * @return pmem::kv::status
 */
inline status db::defrag(double start_percent, double amount_percent)
{
	return static_cast<status>(
		pmemkv_defrag(this->db_.get(), start_percent, amount_percent));
}

/**
 * Returns new write iterator in pmem::kv::result.
 *
 * @return pmem::kv::result<db::write_iterator>
 */
inline result<db::write_iterator> db::new_write_iterator()
{
	pmemkv_write_iterator *tmp;
	auto ret = static_cast<status>(pmemkv_write_iterator_new(db_.get(), &tmp));
	if (static_cast<status>(ret) == status::OK)
		return {db::iterator<false>{tmp}};
	else
		return {ret};
}

/**
 * Returns new read iterator in pmem::kv::result.
 *
 * @return pmem::kv::result<db::read_iterator>
 */
inline result<db::read_iterator> db::new_read_iterator()
{
	pmemkv_iterator *tmp;
	auto ret = static_cast<status>(pmemkv_iterator_new(db_.get(), &tmp));
	if (ret == status::OK)
		return {db::iterator<true>{tmp}};
	else
		return {ret};
}

/**
 * Returns a human readable string describing the last error.
 * Even if this is a method from the db class, it can return the last error from
 * some other class.
 *
 * @return std::string with a description of the last error
 */
inline std::string db::errormsg()
{
	return std::string(pmemkv_errormsg());
}

/**
 * Returns a human readable string describing the last error.
 *
 * @return std::string with a description of the last error
 */
static inline std::string errormsg()
{
	return std::string(pmemkv_errormsg());
}

/**
 * Starts a pmemkv transaction.
 *
 * @return transaction handle
 */
inline result<tx> db::tx_begin() noexcept
{
	pmemkv_tx *tx_;
	auto s = static_cast<status>(pmemkv_tx_begin(db_.get(), &tx_));

	if (s == status::OK)
		return result<tx>(tx(tx_));
	else
		return result<tx>(s);
}

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_HPP */
