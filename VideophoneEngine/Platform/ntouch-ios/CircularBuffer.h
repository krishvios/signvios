//
//  CircularBuffer.hpp
//  SignTime
//
//  Created by Ernesto Solis Vargas on 22/1/25.
//  Copyright © 2025 Sorenson Communications. All rights reserved.
//

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <boost/circular_buffer.hpp>
#include <mutex>

template <typename T>
class CircularBuffer {
public:
	CircularBuffer(size_t size) : buffer(size) {}

	// Add an item to the buffer
	void put(T item) {
		std::lock_guard<std::mutex> lock(mutex);
		if (buffer.full()) {
			buffer.pop_front(); // Ensure oldest data is removed when full
		}
		buffer.push_back(item);
	}

	// Read a single item from the buffer
	T get() {
		std::lock_guard<std::mutex> lock(mutex);
		if (buffer.empty()) {
			throw std::runtime_error("Buffer is empty");
		}
		T val = buffer.front();
		buffer.pop_front();
		return val;
	}

	// Read multiple items into a buffer (efficient bulk read)
	size_t read(T* outBuffer, size_t maxSize) {
		std::lock_guard<std::mutex> lock(mutex);
		size_t bytesToRead = std::min(buffer.size(), maxSize);
		for (size_t i = 0; i < bytesToRead; ++i) {
			outBuffer[i] = buffer.front();
			buffer.pop_front();
		}
		return bytesToRead;
	}

	// Remove elements in a batch using iterators
	void erase(typename boost::circular_buffer<T>::iterator start, typename boost::circular_buffer<T>::iterator end) {
		std::lock_guard<std::mutex> lock(mutex);
		buffer.erase(start, end);
	}

	// Get iterator to the first element
	typename boost::circular_buffer<T>::iterator begin() {
		return buffer.begin();
	}

	// Get direct access to the first memory block
	std::pair<T*, size_t> array_one() {
		std::lock_guard<std::mutex> lock(mutex);
		return buffer.array_one();
	}

	// Get direct access to the second memory block (if wrapped around)
	std::pair<T*, size_t> array_two() {
		std::lock_guard<std::mutex> lock(mutex);
		return buffer.array_two();
	}

	// Check if the buffer is empty
	bool empty() const {
		std::lock_guard<std::mutex> lock(mutex);
		return buffer.empty();
	}

	// Return the number of stored elements (not capacity!)
	size_t size() const {
		std::lock_guard<std::mutex> lock(mutex);
		return buffer.size();
	}
	
	void clear() {
		std::lock_guard<std::mutex> lock(mutex);
		buffer.clear();
	}

private:
	mutable std::mutex mutex;
	boost::circular_buffer<T> buffer;
};

#endif // CIRCULARBUFFER_H
