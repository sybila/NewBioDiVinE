#ifndef CAS_HPP
#define CAS_HPP

#ifndef __GNUC__

#include <windows.h>

template <typename T>
T cas(T & destination, T new_value, T old_value)
{
	return (T)InterlockedCompareExchangePointer((void **)&destination, (void *)new_value, (void *)old_value);
}

template <typename T>
T load_acquire(T const & v)
{

	T res = v;

	return res;
}

template <typename T>
void store_release(T & t, T v)
{

	t = v;

}

#else

template <typename T>
T cas(T & destination, T new_value, T old_value)
{
	return __sync_val_compare_and_swap(&destination, old_value, new_value);
}



template <typename T>
T load_acquire(T const & v)
{
	__sync_synchronize();
	T res = v;
	__sync_synchronize();
	return res;
}

template <typename T>
void store_release(T & t, T v)
{
	__sync_synchronize();
	t = v;
	__sync_synchronize();
}

#endif

#endif