#ifndef PEPMC_BARRIER_HPP
#define PEPMC_BARRIER_HPP

#include <pthread.h>

class barrier
{
public:
	explicit barrier(std::size_t thread_count)
	{
		pthread_barrier_init(&m_barrier, 0, thread_count);
	}

	~barrier()
	{
		pthread_barrier_destroy(&m_barrier);
	}

	void wait()
	{
		pthread_barrier_wait(&m_barrier);
	}

private:
	pthread_barrier_t m_barrier;
};

#endif
