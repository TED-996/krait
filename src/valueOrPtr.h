#pragma once

template<typename T>
class ValueOrPtr
{
private:
	const T* ptr;
	T* valuePtr;
	bool isValue;

public:
	explicit ValueOrPtr(const T* ptr) {
		this->ptr = ptr;
		this->valuePtr = nullptr;
		this->isValue = false;
	}

	explicit ValueOrPtr(const T& value) {
		this->ptr = nullptr;
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
			this->valuePtr = nullptr;
		}
	}

	ValueOrPtr(ValueOrPtr&& source) noexcept {
		this->isValue = source.isValue;
		this->ptr = source.ptr;
		this->valuePtr = source.valuePtr;

		source.isValue = true;
		source.valuePtr = nullptr;
		source.ptr = nullptr;
	}

	~ValueOrPtr() {
		if (this->isValue && this->valuePtr != nullptr) {
			delete this->valuePtr;
		}
		this->isValue = true;
		this->ptr = nullptr;
		this->valuePtr = nullptr;
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
