#ifndef SYNC_COUNTER_HPP
#define SYNC_COUNTER_HPP

#include "cas.hpp"

class sync_counter
{
public:
	explicit sync_counter(std::size_t value)
		: m_counter(value)
	{
	}

	std::size_t get() const
	{
		return load_acquire(m_counter);
	}

	std::size_t add(std::size_t value)
	{
		std::size_t o = 0;
		for (;;)
		{
			std::size_t n = cas(m_counter, o + value, o);
			if (n == o)
				return o + value;
			o = n;
		}
	}

	std::size_t sub(std::size_t value)
	{
		std::size_t o = 0;
		for (;;)
		{
			std::size_t n = cas(m_counter, o - value, o);
			if (n == o)
				return o - value;
			o = n;
		}
	}

	bool test_and_inc()
	{
		std::size_t o = load_acquire(m_counter);
		while (o > 0)
		{
			std::size_t n = cas(m_counter, o + 1, o);
			if (n == o)
				return true;
			o = n;
		}
		return false;
	}

private:
	std::size_t m_counter;
};

#endif
