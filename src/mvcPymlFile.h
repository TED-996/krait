#pragma once
#include <boost/python/object.hpp>
#include "IPymlFile.h"
#include "IPymlCache.h"

class MvcPymlFile : public IPymlFile
{
	const boost::python::object& ctrlClass;
	std::unique_ptr<const IPymlItem> rootItem;
	IPymlCache& cache;

private:
	void setRootItem();

public:
	MvcPymlFile(const boost::python::object& ctrlClass, IPymlCache& cache);

	std::string runPyml() const override;
	
	bool isDynamic() const override {
		return rootItem->isDynamic();
	}
	
	const IPymlItem* getRootItem() const override {
		return &*rootItem;
	}
	
	bool canConvertToCode() const override {
		return getRootItem()->canConvertToCode();
	}
};
