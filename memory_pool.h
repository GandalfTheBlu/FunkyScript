#pragma once
#include <cassert>
#include <stdlib.h>
#include <new>
#include <type_traits>

template<typename T, size_t SIZE>
class MemoryPool
{
private:
	T* pool;
	bool* available;
	size_t bestGuess;

	T* FindAvailable()
	{
		for (size_t i = 0; i < SIZE; i++)
		{
			size_t j = (i + bestGuess) % SIZE;
			if (available[j])
			{
				available[j] = false;
				bestGuess = j + 1;
				return (pool + j);
			}
		}

		assert(false);
		return nullptr;
	}

public:
	MemoryPool()
	{
		pool = (T*)malloc(sizeof(T) * SIZE);
		available = (bool*)malloc(sizeof(bool) * SIZE);

		for (size_t i = 0; i < SIZE; i++)
			available[i] = true;

		bestGuess = 0;
	}

	~MemoryPool()
	{
		free(pool);
		free(available);
	}

	template<typename... ARGS>
	T* New(ARGS... args)
	{
		return new(FindAvailable()) T(args...);
	}

	void Delete(T* ptr)
	{
		if (std::is_destructible<T>::value)
			ptr->~T();

		size_t poolAdress = reinterpret_cast<size_t>(pool);
		size_t ptrAdress = reinterpret_cast<size_t>(ptr);

		assert((ptrAdress >= poolAdress));

		size_t index = (ptrAdress - poolAdress) / sizeof(T);

		assert((index < SIZE));

		available[index] = true;
	}
};

template<typename T>
MemoryPool<T, 1000>& Memory()
{
	static MemoryPool<T, 1000> instance;
	return instance;
}