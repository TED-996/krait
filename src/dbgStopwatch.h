#pragma once
#include <string>

#include "dbg.h"

#ifndef DBG_DISABLE
#include <chrono>

class DbgStopwatch
{
	typedef std::chrono::high_resolution_clock clock;

	std::string name;
	clock::time_point start;
	bool running;
public:
	explicit DbgStopwatch(std::string&& name)
		: name(std::move(name)) {
		restart();
	}

	void restart() {
		start = clock::now();
		running = true;
	}

	void stopPrint() {
		if (running) {
			clock::time_point stop = clock::now();
			DBG_FMT("[DbgStopwatch] [%1%] %2%ms", name,
				std::chrono::duration<double, std::milli>(stop - start).count());
			running = false;
		}
	}

	void discard() {
		running = false;
	}

	~DbgStopwatch() {
		if (running) {
			stopPrint();
		}
	}
};

template<typename TStopwatch>
class _DbgAggregatedStopwatch
{
	typedef typename TStopwatch::clock clock;
	typedef typename clock::duration duration;
	typedef typename clock::time_point time_point;
	
	std::string name;
	duration total;
public:
	explicit _DbgAggregatedStopwatch(std::string&& name) : name(std::move(name)), total() {
	}

	_DbgAggregatedStopwatch(const _DbgAggregatedStopwatch& other) = delete;

	_DbgAggregatedStopwatch(_DbgAggregatedStopwatch&& other) noexcept = delete;

	_DbgAggregatedStopwatch& operator=(const _DbgAggregatedStopwatch& other) = delete;

	_DbgAggregatedStopwatch& operator=(_DbgAggregatedStopwatch&& other) noexcept = delete;

	void discard() {
		total = duration();
	}

	void stopPrint() {
		if (total.count() != 0) {
			DBG_FMT("[DbgStopwatch] [%1%] %2%ms", name,
				std::chrono::duration<double, std::milli>(total).count());
			total = duration();
		}
	}

	void add(duration amount) {
		total += amount;
	}

	~_DbgAggregatedStopwatch() {
		stopPrint();
	}
};

class DbgDependentStopwatch
{
public:
	typedef std::chrono::high_resolution_clock clock;
private:
	typedef _DbgAggregatedStopwatch<DbgDependentStopwatch> Aggregator;
	
	clock::time_point start;
	bool running;
	Aggregator& aggregator;

public:
	DbgDependentStopwatch(Aggregator& aggregator)
		: aggregator(aggregator) {
		restart();
	}

	DbgDependentStopwatch(const DbgDependentStopwatch& other) = delete;

	DbgDependentStopwatch(DbgDependentStopwatch&& other) noexcept = delete;

	DbgDependentStopwatch& operator=(const DbgDependentStopwatch& other) = delete;

	DbgDependentStopwatch& operator=(DbgDependentStopwatch&& other) noexcept = delete;

	void restart() {
		start = clock::now();
		running = true;
	}

	void stopAdd() {
		if (running) {
			clock::time_point stop = clock::now();
			aggregator.add(stop - start);
			running = false;
		}
	}

	void discard() {
		running = false;
	}

	~DbgDependentStopwatch() {
		if (running) {
			stopAdd();
		}
	}
};

typedef _DbgAggregatedStopwatch<DbgDependentStopwatch> DbgAggregatedStopwatch;

#else

class DbgStopwatch
{
public:
	explicit DbgStopwatch(const std::string& name){
	}

	void restart() {
	}

	void stopPrint() {
	}

	void discard() {
	}
};

template<typename TStopwatch>
class _DbgAggregatedStopwatch
{
	typedef int clock;
	typedef int duration;
	typedef int time_point;

public:
	explicit _DbgAggregatedStopwatch(const std::string& name) {
	}

	void discard() {
	}

	void stopPrint() {
	}

	void add(duration amount) {
	}
};

class DbgDependentStopwatch
{
public:
	typedef int clock;

private:
	typedef _DbgAggregatedStopwatch<DbgDependentStopwatch> Aggregator;
public:
	DbgDependentStopwatch(Aggregator& aggregator){
	}

	void restart() {
	}

	void stopAdd() {
	}

	void discard() {
	}
};

typedef _DbgAggregatedStopwatch<DbgDependentStopwatch> DbgAggregatedStopwatch;

#endif
