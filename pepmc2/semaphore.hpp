#ifndef PEPMC_SEMAPHORE_HPP
#define PEPMC_SEMAPHORE_HPP

#include <semaphore.h>

class semaphore
{
public:
	semaphore()
	{
		sem_init(&m_sem, 0, 0);
	}

	explicit semaphore(std::size_t initial_count)
	{
		sem_init(&m_sem, 0, initial_count);
	}

	~semaphore()
	{
		sem_destroy(&m_sem);
	}

	void post()
	{
		sem_post(&m_sem);
	}

	void wait()
	{
		sem_wait(&m_sem);
	}

private:
	sem_t m_sem;

	semaphore(semaphore const &);
	semaphore & operator=(semaphore const &);
};

#endif
