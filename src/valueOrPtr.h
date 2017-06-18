#pragma once

template<typename T>
class ValueOrPtr
{
private:
	const T* ptr;
	T* valuePtr;
	bool isValue;

public:
	ValueOrPtr(const T* ptr) {
		this->ptr = ptr;
		this->valuePtr = NULL;
		this->isValue = false;
	}

	ValueOrPtr(const T value) {
		this->ptr = NULL;
		this->valuePtr = new T(value);
		this->isValue = true;
	}

	ValueOrPtr(const ValueOrPtr& source) {
		this->isValue = source.isValue;
		this->ptr = source.ptr;

		if (source.isValue) {
			this->valuePtr = new T(*source.valuePtr);
		}
		else {
			this->valuePtr = NULL;
		}
	}

	ValueOrPtr(ValueOrPtr&& source) {
		this->isValue = source.isValue;
		this->ptr = source.ptr;
		this->valuePtr = source.valuePtr;

		source.isValue = true;
		source.valuePtr = NULL;
		source.ptr = NULL;
	}

	~ValueOrPtr() {
		if (this->isValue && this->valuePtr != NULL) {
			delete this->valuePtr;
		}
		this->isValue = true;
		this->ptr = NULL;
		this->valuePtr = NULL;
	}

	const T* get() const {
		if (isValue) {
			return valuePtr;
		}
		else {
			return ptr;
		}
	}
};
