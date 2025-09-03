#pragma once
#include<mutex>
#include<array>
#include<memory.h>
#include<malloc.h>
#include<cstdint>
#include<assert.h>
#include "audio-io.h"


//template<int ARRAY_SIZE>
class CRingBuffer
{
protected:
//#define MUTEX std::lock_guard<std::mutex>m(m_ring_buff_mutex)
//#define MUTEX_F(X) void X##_mutex(){MUTEX;X();}
//#define MUTEX_F_ARG_1(X, AGR_TYPE_1, ARG1) void X##_mutex(AGR_TYPE_1 ARG1){MUTEX;X(ARG1);}
//#define MUTEX_F_ARGS_2(X, AGR_TYPE_1, ARG1, AGR_TYPE_2, ARG2) void X##_mutex(AGR_TYPE_1 ARG1, AGR_TYPE_2 ARG2){MUTEX;X(ARG1, ARG2);}
//#define MUTEX_F_ARGS_3(X, AGR_TYPE_1, ARG1, AGR_TYPE_2, ARG2, AGR_TYPE_3, ARG3) void X##_mutex(AGR_TYPE_1 ARG1, AGR_TYPE_2 ARG2, AGR_TYPE_3 ARG3){MUTEX;X(ARG1,ARG2,ARG3);}

public:
	//static constexpr int SIZE = ARRAY_SIZE;
	int SIZE = 2; //channel nums

	/*MUTEX_F(init)
	MUTEX_F(free)
	MUTEX_F_ARG_1(reorder_data, size_t, new_capacity)
	MUTEX_F(ensure_capacity)
	MUTEX_F_ARG_1(reserve, size_t, new_capacity)
	MUTEX_F_ARG_1(upsize, size_t, s)
	MUTEX_F_ARGS_3(place, size_t ,position, const void** ,d, size_t, s)
	MUTEX_F_ARGS_2(push_back, const void** ,d,size_t ,s)
	MUTEX_F_ARGS_2(push_front, const void** ,d,size_t ,s)
	MUTEX_F_ARG_1(push_back_zero, size_t ,s)
	MUTEX_F_ARG_1(push_front_zero, size_t, s)
	MUTEX_F_ARGS_2(peek_front, void** ,d, size_t ,s)
	MUTEX_F_ARGS_2(peek_back, void**, d, size_t, s)
	MUTEX_F_ARGS_2(pop_front, void**, d, size_t, s)
	MUTEX_F_ARG_1(pop_front_all, void**, d)
	MUTEX_F_ARGS_2(pop_back, void**, d, size_t, s)*/

	//std::array<uint8_t*, ARRAY_SIZE>get_data_mutex(size_t idx)
	//{
	//	MUTEX;
	//	return get_data(idx);
	//}


	inline void init()
	{
		//memset(dq, 0, sizeof(struct deque));
		//data = NULL;
		memset(data, 0, sizeof(data));
		size = 0;
		start_pos = 0;
		end_pos = 0;
		capacity = 0;
		//SIZE = 0;
	}

	inline void free()
	{
		//bfree(data);
		for (int i = 0; i < SIZE; ++i) 
			if (data[i])
			{
				::free(data[i]);
				data[i] = NULL;
			}
		//data = NULL;
		size = 0;
		start_pos = 0;
		end_pos = 0;
		capacity = 0;
	}

	inline void reorder_data(size_t new_capacity)
	{
		size_t difference;
		uint8_t* d;

		if (!size || !start_pos || end_pos > start_pos)
			return;

		difference = new_capacity - capacity;
		for (int i = 0; i < SIZE; ++i)
		{
			d = (uint8_t*)data[i] + start_pos;
			memmove(d + difference, d, capacity - start_pos);
		}
		start_pos += difference;
	}

	inline void ensure_capacity()
	{
		size_t new_capacity;
		if (size <= capacity)
			return;

		new_capacity = capacity * 2;
		if (size > new_capacity)
			new_capacity = size;

		for (int i = 0; i < SIZE; ++i)
			data[i] = realloc(data[i], new_capacity);
		reorder_data(new_capacity);
		capacity = new_capacity;
		//size_t new_capacity;
		//if (size <= capacity)
		//	return;

		//new_capacity = capacity * 2;
		//if (size > new_capacity)
		//	new_capacity = size;

		////data = brealloc(data, new_capacity);
		//for(int i = 0; i < SIZE; ++i)
		//	data[i] = realloc(data[i], new_capacity);

		////reorder_data(new_capacity);
		//size_t difference;
		//uint8_t* d;

		//if (!size || !start_pos || end_pos > start_pos)
		//	return;

		//difference = new_capacity - capacity;

		//for (int i = 0; i < SIZE; ++i) 
		//{
		//	d = (uint8_t*)data[i] + start_pos;
		//	memmove(d + difference, d, capacity - start_pos);
		//}

		//start_pos += difference;

		//capacity = new_capacity;
	}

	inline void reserve(size_t new_capacity)
	{

		if (new_capacity <= capacity)
			return;

		//data = brealloc(data, new_capacity);
		for(int i = 0; i < SIZE; ++i)
			data[i] = realloc(data[i], new_capacity);
		reorder_data(new_capacity);
		capacity = new_capacity;
	}

	inline void upsize(size_t s)
	{
		size_t add_size = s - size;
		size_t new_end_pos = end_pos + add_size;

		if (s <= size)
			return;
		size = s;
		ensure_capacity();
		if (new_end_pos > capacity) {
			size_t back_size = capacity - end_pos;
			size_t loop_size = add_size - back_size;

			if (back_size)
				for(int i = 0; i < SIZE; ++i)
					memset((uint8_t*)data[i] + end_pos, 0, back_size);

			for(int i = 0; i < SIZE; ++i)
				memset(data[i], 0, loop_size);
			new_end_pos -= capacity;
		}
		else {
			for(int i = 0; i < SIZE;++i)
				memset((uint8_t*)data[i] + end_pos, 0, add_size);
		}
		end_pos = new_end_pos;
	}

	/** Overwrites data at a specific point in the buffer (relative).  */
	inline void place(size_t position,
		const void** d, size_t s)
	{
		size_t end_point = position + s;
		size_t data_end_pos;

		if (end_point > size)
			upsize(end_point);

		position += start_pos;
		if (position >= capacity)
			position -= capacity;

		data_end_pos = position + s;
		if (data_end_pos > capacity) {
			size_t back_size = data_end_pos - capacity;
			size_t loop_size = s - back_size;

			for (int i = 0; i < SIZE; ++i) {
				memcpy((uint8_t*)data[i] + position, d[i], loop_size);
				memcpy(data[i], (uint8_t*)d[i] + loop_size, back_size);
			}
		}
		else {
			for (int i = 0; i < SIZE; ++i)
				memcpy((uint8_t*)data[i] + position, d[i], s);
		}
	}

	inline void push_back(const void** d,
		size_t s)
	{
		size_t new_end_pos = end_pos + s;

		size += s;
		ensure_capacity();

		if (new_end_pos > capacity) {
			size_t back_size = capacity - end_pos;
			size_t loop_size = s - back_size;

			if (back_size)
				for(int i =0;i<SIZE;++i)
					memcpy((uint8_t*)data[i] + end_pos, d[i],
					back_size);
			for(int i = 0; i < SIZE; ++i)
				memcpy(data[i], (uint8_t*)d[i] + back_size, loop_size);

			new_end_pos -= capacity;
		}
		else {
			for(int i = 0; i < SIZE;++i)
				memcpy((uint8_t*)data[i] + end_pos, d[i], s);
		}

		end_pos = new_end_pos;
	}

	inline void push_front(const void** d,
		size_t s)
	{
		size += s;
		ensure_capacity();

		if (size == s) {
			start_pos = 0;
			end_pos = s;
			for(int i =0; i < SIZE;++i)
				memcpy((uint8_t*)data[i], d[i], s);
		}
		else if (start_pos < s) {
			size_t back_size = s - start_pos;

			if (start_pos)
				for (int i = 0; i < SIZE; ++i)
					memcpy(data[i], (uint8_t*)d[i] + back_size,
						start_pos);

			start_pos = capacity - back_size;
			for (int i = 0; i < SIZE; ++i)
				memcpy((uint8_t*)data[i] + start_pos, d[i], back_size);
		}
		else {
			start_pos -= s;
			for (int i = 0; i < SIZE; ++i)
				memcpy((uint8_t*)data[i] + start_pos, d[i], s);
		}
	}

	inline void push_back_zero(size_t s)
	{
		size_t new_end_pos = end_pos + s;

		size += s;
		ensure_capacity();

		if (new_end_pos > capacity) {
			size_t back_size = capacity - end_pos;
			size_t loop_size = s - back_size;

			if (back_size)
				for (int i = 0; i < SIZE; ++i)
					memset((uint8_t*)data[i] + end_pos, 0, back_size);
			for (int i = 0; i < SIZE; ++i)
				memset(data[i], 0, loop_size);

			new_end_pos -= capacity;
		}
		else {
			for (int i = 0; i < SIZE; ++i)
				memset((uint8_t*)data[i] + end_pos, 0, s);
		}

		end_pos = new_end_pos;
	}

	inline void push_front_zero(size_t s)
	{
		size += s;
		ensure_capacity();

		if (size == s) {
			start_pos = 0;
			end_pos = s;
			for (int i = 0; i < SIZE; ++i)
				memset((uint8_t*)data[i], 0, s);
		}
		else if (start_pos < s) {
			size_t back_size = s - start_pos;

			if (start_pos)
				for (int i = 0; i < SIZE; ++i)
					memset(data[i], 0, start_pos);

			start_pos = capacity - back_size;
			for (int i = 0; i < SIZE; ++i)
				memset((uint8_t*)data[i] + start_pos, 0, back_size);
		}
		else {
			start_pos -= s;
			for (int i = 0; i < SIZE; ++i)
				memset((uint8_t*)data[i] + start_pos, 0, s);
		}
	}

	inline void peek_front(void** d, size_t s)
	{
		assert(s <= size);

		if (d) {
			size_t start_size = capacity - start_pos;

			if (start_size < s) {
				for (int i = 0; i < SIZE; ++i)
				{
					if (d[i])
					{
						memcpy(d[i], (uint8_t*)data[i] + start_pos,
							start_size);
						memcpy((uint8_t*)d[i] + start_size, data[i],
							s - start_size);
					}
				}
			}
			else {
				for (int i = 0; i < SIZE; ++i)
					if(d[i])
						memcpy(d[i], (uint8_t*)data[i] + start_pos, s);
			}
		}
	}

	inline void peek_back(void** d, size_t s)
	{
		assert(s <= size);

		if (d) {
			size_t back_size = (end_pos ? end_pos : capacity);

			if (back_size < s) {
				size_t front_size = s - back_size;
				size_t new_end_pos = capacity - front_size;
				for (int i = 0; i < SIZE; ++i)
				{
					memcpy((uint8_t*)d[i] + (s - back_size), data[i],
						back_size);
					memcpy(d[i], (uint8_t*)data[i] + new_end_pos,
						front_size);
				}
			}
			else {
				for (int i = 0; i < SIZE; ++i)
					memcpy(d[i], (uint8_t*)data[i] + end_pos - s, s);
			}
		}
	}

	inline void pop_front(void** d, size_t s)
	{
		peek_front(d, s);

		size -= s;
		if (!size) {
			start_pos = end_pos = 0;
			return;
		}

		start_pos += s;
		if (start_pos >= capacity)
			start_pos -= capacity;
	}

	inline void pop_front_all(void** d)
	{
		pop_front(d, size);
	}

	inline void pop_back(void** d, size_t s)
	{
		peek_back(d, s);
		size -= s;
		if (!size) {
			start_pos = end_pos = 0;
			return;
		}

		if (end_pos <= s)
			end_pos = capacity - (s - end_pos);
		else
			end_pos -= s;
	}

	/*inline std::array<uint8_t*,ARRAY_SIZE> get_data(size_t idx)
	{
		uint8_t* ptr = (uint8_t*)data;
		size_t offset = start_pos + idx;

		if (idx >= size)
			return NULL;

		if (offset >= capacity)
			offset -= capacity;

		std::array<uint8_t*, ARRAY_SIZE>arr;
		for (int i = 0; i < SIZE; ++i)
			arr[i] = ptr + offset;
		return arr;
	}*/

	void* data[MAX_AUDIO_CHANNELS];
	size_t size;
	size_t start_pos;
	size_t end_pos;
	size_t capacity;
	//std::mutex m_ring_buff_mutex;
};

