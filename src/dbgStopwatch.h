#pragma once

#include "dbg.h"

#ifndef DBG_DISABLE
#include <string>
#include <chrono>

class _DbgStopwatch
{
	typedef std::chrono::high_resolution_clock clock;

	std::string name;
	clock::time_point start;
	bool running;
public:
	explicit _DbgStopwatch(std::string&& name)
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
			_DBG_FMT("[DbgStopwatch] [%1%] %2%ms", name,
				std::chrono::duration<double, std::milli>(stop - start).count());
			running = false;
		}
	}

	void discard() {
		running = false;
	}

	~_DbgStopwatch() {
		if (running) {
			stopPrint();
		}
	}
};

template<typename TStopwatch>
class __DbgAggregatedStopwatch
{
	typedef typename TStopwatch::clock clock;
	typedef typename clock::duration duration;
	typedef typename clock::time_point time_point;
	
	std::string name;
	duration total;
public:
	explicit __DbgAggregatedStopwatch(std::string&& name) : name(std::move(name)), total() {
	}

	__DbgAggregatedStopwatch(const __DbgAggregatedStopwatch& other) = delete;

	__DbgAggregatedStopwatch(__DbgAggregatedStopwatch&& other) noexcept = delete;

	__DbgAggregatedStopwatch& operator=(const __DbgAggregatedStopwatch& other) = delete;

	__DbgAggregatedStopwatch& operator=(__DbgAggregatedStopwatch&& other) noexcept = delete;

	void discard() {
		total = duration();
	}

	void stopPrint() {
		if (total.count() != 0) {
			_DBG_FMT("[DbgStopwatch] [%1%] %2%ms", name,
				std::chrono::duration<double, std::milli>(total).count());
			total = duration();
		}
	}

	void add(duration amount) {
		total += amount;
	}

	~__DbgAggregatedStopwatch() {
		stopPrint();
	}
};

class _DbgDependentStopwatch
{
public:
	typedef std::chrono::high_resolution_clock clock;
private:
	typedef __DbgAggregatedStopwatch<_DbgDependentStopwatch> Aggregator;
	
	clock::time_point start;
	bool running;
	Aggregator& aggregator;

public:
	_DbgDependentStopwatch(Aggregator& aggregator)
		: aggregator(aggregator) {
		restart();
	}

	_DbgDependentStopwatch(const _DbgDependentStopwatch& other) = delete;

	_DbgDependentStopwatch(_DbgDependentStopwatch&& other) noexcept = delete;

	_DbgDependentStopwatch& operator=(const _DbgDependentStopwatch& other) = delete;

	_DbgDependentStopwatch& operator=(_DbgDependentStopwatch&& other) noexcept = delete;

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

	~_DbgDependentStopwatch() {
		if (running) {
			stopAdd();
		}
	}
};

typedef __DbgAggregatedStopwatch<_DbgDependentStopwatch> _DbgAggregatedStopwatch;


#define DbgStopwatchVar(var, name) \
	_DbgStopwatch var((name)); \
	(void)var;

#define DbgStopwatch(name) DbgStopwatchVar(stopwatch, (name))


#define DbgAggregatedStopwatchVar(var, name) \
	_DbgAggregatedStopwatch var((name)); \
	(void)var;

#define DbgAggregatedStopwatch(name) DbgAggregatedStopwatchVar(aggrStopwatch, (name))


#define DbgDependentStopwatchVar(var, aggregator) \
	_DbgDependentStopwatch var((aggregator)); \
	(void)var;

#define DbgDependentStopwatch(aggregator) DbgDependentStopwatch(depStopwatch, (aggregator))

#define DbgDependentStopwatchDef() DbgDependentStopwatch(aggrStopwatch)

#define DbgDependentStopwatchDefVar(var) DbgDependentStopwatchVar(var, aggrStopwatch)



#else

#define DbgStopwatchVar(var, name) (void)0
#define DbgStopwatch(name) (void)0

#define DbgAggregatedStopwatchVar(var, name) (void)0
#define DbgAggregatedStopwatch(name) (void)0

#define DbgDependentStopwatchVar(var, aggregator) (void)0
#define DbgDependentStopwatch(aggregator) (void)0
#define DbgDependentStopwatchDef() (void)0
#define DbgDependentStopwatchDefVar(var) (void)0

#endif
