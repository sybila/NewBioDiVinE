#ifndef SYNC_FLAG_HPP
#define SYNC_FLAG_HPP

#ifndef __GNUC__

#include <windows.h>

class sync_flag
{
public:
	sync_flag()
	{
		m_hEvent = ::CreateEvent(0, FALSE, FALSE, 0);
	}

	sync_flag(sync_flag && other)
	{
		m_hEvent = other.m_hEvent;
		other.m_hEvent = 0;
	}

	~sync_flag()
	{
		if (m_hEvent)
			::CloseHandle(m_hEvent);
	}

	sync_flag & operator=(sync_flag && other)
	{
		if (m_hEvent)
			::CloseHandle(m_hEvent);
		m_hEvent = other.m_hEvent;
		other.m_hEvent = 0;
		return *this;
	}

	void set()
	{
		::SetEvent(m_hEvent);
	}

	void wait()
	{
		::WaitForSingleObject(m_hEvent, INFINITE);
	}

private:
	HANDLE m_hEvent;

	sync_flag(sync_flag const & other);
	sync_flag & operator=(sync_flag const & other);
};

#else

#include <semaphore.h>
struct mrevent {
	sem_t sem;
};
void mrevent_init(struct mrevent *ev) {
	sem_init(&ev->sem, 0, 0);
}
void mrevent_destroy(struct mrevent *ev) {
	sem_destroy(&ev->sem);
}

void mrevent_trigger(struct mrevent *ev)
{
	sem_post(&ev->sem);
}

void mrevent_wait(struct mrevent *ev)
{
	sem_wait(&ev->sem);
}


class sync_flag
{
public:
	sync_flag()
	{
		m_event = new mrevent;
		mrevent_init(m_event);
	}

	sync_flag(sync_flag && other)
	{
		m_event = other.m_event;
		other.m_event = 0;
	}

	~sync_flag()
	{
		if (m_event)
		{
			mrevent_destroy(m_event);
			delete m_event;
		}
	}

	sync_flag & operator=(sync_flag && other)
	{
		if (m_event)
		{
			mrevent_destroy(m_event);
			delete m_event;
		}
		m_event = other.m_event;
		other.m_event = 0;
		return *this;
	}

	void set()
	{
		mrevent_trigger(m_event);
	}

	void wait()
	{
		mrevent_wait(m_event);
	}

private:
    mrevent * m_event;

	sync_flag(sync_flag const & other);
	sync_flag & operator=(sync_flag const & other);
};

#endif

#endif
