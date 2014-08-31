#ifndef LOCK_FREE_QUEUE_HPP
#define LOCK_FREE_QUEUE_HPP

#include "cas.hpp"

namespace detail
{

template <typename T>
struct lfq_node
{
	typedef T value_type;

	template <typename U>
	lfq_node(U && value, lfq_node * next)
		: m_value(std::forward<U>(value)), m_next(next), m_prev(0)
	{
	}

	value_type m_value;
	lfq_node * m_next;
	lfq_node * m_prev;
};

}

template <typename T>
class local_queue;

template <typename T>
class global_queue
{
	friend class local_queue<T>;
public:
	typedef T value_type;
	typedef detail::lfq_node<value_type> node;

	global_queue()
		: m_head(0)
	{
	}

	global_queue(global_queue && other)
		: m_head(other.m_head)
	{
		other.m_head = 0;
	}

	~global_queue()
	{
		while (m_head != 0)
		{
			node * n = m_head;
			m_head = n->m_next;
			delete n;
		}
	}

	global_queue & operator=(global_queue && other)
	{
		// TODO: clear first
		m_head = other.m_head;
		other.m_head = 0;
		return *this;
	}

	// Returns true if the queue was empty
	template <typename U>
	bool push(U && value)
	{
		node * n = new node(std::forward<U>(value), 0);

		node * new_next = m_head;
		node * old_next;
		do
		{
			n->m_next = old_next = new_next;
			new_next = cas(m_head, n, new_next);
		}
		while (new_next != old_next);

		return new_next == 0;
	}

private:
	node * m_head;

	global_queue(global_queue const &);
	global_queue & operator=(global_queue const &);
};

template <typename T>
class local_queue
{
public:
	typedef T value_type;
	typedef detail::lfq_node<value_type> node;

	local_queue()
		: m_head(0), m_tail(0), m_size(0)
	{
	}

	~local_queue()
	{
		this->clear();
	}

	std::size_t size() const
	{
		return m_size;
	}

	bool empty() const
	{
		return m_head == 0;
	}

	void clear()
	{
		while (!this->empty())
			this->pop();
	}

	template <typename U>
	void push(U && value)
	{
		m_head = new node(std::forward<U>(value), m_head);
		if (m_tail == 0)
			m_tail = m_head;
		else
			m_head->m_next->m_prev = m_head;
		++m_size;
	}

	value_type const & top() const
	{
		return m_tail->m_value;
	}

	void pop()
	{
		node * n = m_tail;
		m_tail = n->m_prev;
		if (m_tail == 0)
			m_head = 0;
		else
			m_tail->m_next = 0;
		--m_size;
		delete n;
	}

	// Returns true if the global queue was empty
	bool merge(global_queue<value_type> & gq)
	{
		if (m_tail == 0)
			return false;

		node * next = gq.m_head;
		do
		{
			m_tail->m_next = next;
			next = cas(gq.m_head, m_head, next);
		}
		while (m_tail->m_next != next);

		m_head = m_tail = 0;
		m_size = 0;

		return next == 0;
	}

	void pull(global_queue<value_type> & gq)
	{
		this->clear();

		node * new_head = gq.m_head;
		do
		{
			m_head = new_head;
			new_head = cas(gq.m_head, (node *)0, new_head);
		}
		while (m_head != new_head);

		// Fix-up the pulled list
		if (m_head == 0)
			return;

		++m_size;
		node * cur = m_head;
		while (cur->m_next != 0)
		{
			cur->m_next->m_prev = cur;
			++m_size;
			cur = cur->m_next;
		}

		m_tail = cur;
	}

private:
	node * m_head;
	node * m_tail;
	std::size_t m_size;

	local_queue(local_queue const &);
	local_queue & operator=(local_queue const &);
};

#endif
