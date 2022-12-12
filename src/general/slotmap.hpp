// Author: Pixelcluster
// 12/14/2022
// https://github.com/pixelcluster/VanadiumEngine/blob/master/include/helper/VSlotmap.hpp
#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

template <typename T> class slotmap;

template <typename T> struct slot {
	u64 value = LONG_MAX;
    
	bool   operator==(const slot<T>& other) const {
		  return value == other.value;
	}
};

template <typename T> class slotmap {
 public:
	class iterator {
	 public:
		iterator() {}

		explicit iterator(T* elementPointer) : elementPointer(elementPointer) {}

		iterator& operator++() {
			++elementPointer;
			return *this;
		}
		iterator operator++(int32_t) {
			iterator returnValue = *this;
			++elementPointer;
			return returnValue;
		}
		iterator& operator--() {
			--elementPointer;
			return *this;
		}
		iterator operator--(int32_t) {
			iterator returnValue = *this;
			--elementPointer;
			return returnValue;
		}
		iterator& operator+=(u64 amount) {
			elementPointer += amount;
			return *this;
		}
		iterator& operator-=(u64 amount) {
			elementPointer -= amount;
			return *this;
		}
		iterator operator-(u64 amount) const {
			iterator returnValue = iterator(elementPointer);
			returnValue.elementPointer -= amount;
			return returnValue;
		}
		iterator operator+(u64 amount) const {
			iterator returnValue = iterator(elementPointer);
			returnValue.elementPointer += amount;
			return returnValue;
		}
		ptrdiff_t operator-(const iterator& other) const {
			return elementPointer - other.elementPointer;
		}
		ptrdiff_t operator+(const iterator& other) const {
			return elementPointer + other.elementPointer;
		}

		bool operator==(iterator other) const {
			return elementPointer == other.elementPointer;
		}

		bool operator!=(iterator other) const {
			return elementPointer != other.elementPointer;
		}

		T& operator*() const {
			return *elementPointer;
		}
		T* operator->() {
			return elementPointer;
		}
		const T* operator->() const {
			return elementPointer;
		}

		operator T*() const {
			return elementPointer;
		}

	 private:
		T* elementPointer;
	};

	class const_iterator {
	 public:
		const_iterator() {}

		explicit const_iterator(T* elementPointer) : elementPointer(elementPointer) {}

		const_iterator& operator++() {
			++elementPointer;
			return *this;
		}
		const_iterator operator++(int32_t) {
			const_iterator returnValue = *this;
			++elementPointer;
			return returnValue;
		}
		const_iterator& operator--() {
			--elementPointer;
			return *this;
		}
		const_iterator operator--(int32_t) {
			const_iterator returnValue = *this;
			--elementPointer;
			return returnValue;
		}
		const_iterator& operator+=(u64 amount) {
			elementPointer += amount;
			return *this;
		}
		const_iterator& operator-=(u64 amount) {
			elementPointer -= amount;
			return *this;
		}
		const_iterator operator-(u64 amount) const {
			const_iterator returnValue = const_iterator(elementPointer);
			returnValue.elementPointer -= amount;
			return returnValue;
		}
		const_iterator operator+(u64 amount) const {
			const_iterator returnValue = const_iterator(elementPointer);
			returnValue.elementPointer += amount;
			return returnValue;
		}
		ptrdiff_t operator-(const const_iterator& other) const {
			return elementPointer - other.elementPointer;
		}
		ptrdiff_t operator+(const const_iterator& other) const {
			return elementPointer + other.elementPointer;
		}

		bool operator==(const const_iterator& other) const {
			return elementPointer == other.elementPointer;
		}

		bool operator!=(const const_iterator& other) const {
			return elementPointer != other.elementPointer;
		}

		T& operator*() const {
			return *elementPointer;
		}
		T* operator->() {
			return elementPointer;
		}
		const T* operator->() const {
			return elementPointer;
		}

	 private:
		T* elementPointer;
	};

	inline slot<T> add_element(const T& newElement);
	inline slot<T> add_element(T&& newElement);

	// Gets the element that belongs to the specified handle. If "handle" is invalid, an exception will thrown.
	inline T&		element_at(slot<T> handle);
	inline const T& element_at(slot<T> handle) const;

    inline bool valid(slot<T> handle) const;

	// Removes the element specified by its handle. If the handle is invalid, nothing happens.
	inline void remove_element(slot<T> handle);
	inline void remove_value(T& value);

	inline T&		operator[](slot<T> handle);
	inline const T& operator[](slot<T> handle) const;

	inline void clear();

	inline u64 size() const;

	inline iterator			 begin();
	inline iterator			 end();
	inline const_iterator	 cbegin() const;
	inline const_iterator	 cend() const;
	inline iterator			 find(slot<T> handle);
	inline const_iterator	 find(slot<T> handle) const;
	inline slot<T> handle(const iterator& handleIterator);
    
	// All elements.
	std::vector<T>   elements;
	std::vector<u64> keys;
	std::vector<u64> eraseMap;
	u64				 freeKeyHead = 0;
	u64				 freeKeyTail = 0;
};

template <typename T> slot<T> slotmap<T>::add_element(const T& newElement) {
	if (keys.size() == 0)
		keys.push_back(0);
	elements.push_back(newElement);
	eraseMap.push_back(freeKeyHead);
	if (freeKeyHead == freeKeyTail) {
		u64 newFreeSlotIndex = keys.size();

		keys.push_back(newFreeSlotIndex);

		keys[freeKeyTail] = newFreeSlotIndex;
		freeKeyTail		  = newFreeSlotIndex;
	}
	u64 nextFreeIndex = keys[freeKeyHead];

	keys[freeKeyHead] = elements.size() - 1;

	u64 returnIndex = freeKeyHead;
	freeKeyHead		   = nextFreeIndex;

	return {returnIndex};
}

template <typename T> slot<T> slotmap<T>::add_element(T&& newElement) {
	if (keys.size() == 0)
		keys.push_back(0);
	elements.push_back(std::forward<T>(newElement));
	eraseMap.push_back(freeKeyHead);
	if (freeKeyHead == freeKeyTail) {
		u64 newFreeSlotIndex = keys.size();

		keys.push_back(newFreeSlotIndex);

		keys[freeKeyTail] = newFreeSlotIndex;
		freeKeyTail		  = newFreeSlotIndex;
	}
	u64 nextFreeIndex = keys[freeKeyHead];

	keys[freeKeyHead] = elements.size() - 1;

	u64 returnIndex = freeKeyHead;
	freeKeyHead		   = nextFreeIndex;

	return {returnIndex};
}

template <typename T> T& slotmap<T>::element_at(slot<T> handle) {
	assert(keys.size() > handle.value);
	assert(eraseMap[keys[handle.value]] == handle.value);
	return elements[keys[handle.value]];
}

template <typename T> const T& slotmap<T>::element_at(slot<T> handle) const {
	assert(keys.size() > handle.value);
	assert(eraseMap[keys[handle.value]] == handle.value);
	return elements[keys[handle.value]];
}

template <typename T> void slotmap<T>::remove_element(slot<T> handle) {
	assert(keys.size() > handle.value);

	u64 eraseElementIndex = keys[handle.value];

	// Move last element in, update erase table
	elements[eraseElementIndex] = std::move(elements[elements.size() - 1]);
	eraseMap[eraseElementIndex] = eraseMap[elements.size() - 1];

	// Update key index of what was the last element
	keys[eraseMap[eraseElementIndex]] = eraseElementIndex;

	// Update erase table/element std::vector sizes
	elements.erase(elements.begin() + (elements.size() - 1));
	eraseMap.erase(eraseMap.begin() + (eraseMap.size() - 1));

	// Update free list nodes
	keys[freeKeyTail]  = handle.value;
	keys[handle.value] = handle.value;
	freeKeyTail		   = handle.value;
}

template <typename T> void slotmap<T>::remove_value(T& value) {
	slotmap<T>::iterator it = begin();
	for (; it != end(); it++) {
		if (*it == value)
			break;
	}
	if (it != end())
		remove_element(handle(it));
}

template <typename T> T& slotmap<T>::operator[](slot<T> handle) {
	return element_at(handle);
}

template <typename T> const T& slotmap<T>::operator[](slot<T> handle) const {
	return element_at(handle);
}

template <typename T> void slotmap<T>::clear() {
	elements.clear();
}

template <typename T> u64 slotmap<T>::size() const {
	return elements.size();
}

template <typename T> slotmap<T>::iterator slotmap<T>::begin() {
	return iterator(elements.data());
}

template <typename T> slotmap<T>::iterator slotmap<T>::end() {
	return iterator(elements.data() + elements.size());
}

template <typename T> slotmap<T>::const_iterator slotmap<T>::cbegin() const {
	return const_iterator(elements.data());
}

template <typename T> slotmap<T>::const_iterator slotmap<T>::cend() const {
	return const_iterator(elements.data() + elements.size());
}

template <typename T> slotmap<T>::iterator slotmap<T>::find(slot<T> handle) {
	if (handle.value >= keys.size())
		return iterator(elements.data() + elements.size());
	return iterator(elements.data() + keys[handle.value]);
}

template <typename T> slotmap<T>::const_iterator slotmap<T>::find(slot<T> handle) const {
	if (handle.value >= keys.size())
		return const_iterator(elements.data() + elements.size());
	return const_iterator(elements.data() + keys[handle.value]);
}

template <typename T> slot<T> slotmap<T>::handle(const slotmap<T>::iterator& handleIterator) {
	return {eraseMap[static_cast<T*>(handleIterator) - elements.data()], this};
}

template <typename T> bool slotmap<T>::valid(slot<T> handle) const {
	if (keys.size() <= handle.value) return false;
	if (eraseMap.size() <= keys[handle.value]) return false;
	return eraseMap[keys[handle.value]] == handle.value;
}

namespace std {
template <typename T> struct hash<slot<T>> {
	u64 operator()(const slot<T>& handle) const {
		return std::hash<u64>()(handle.value);
	}
};


} // namespace std