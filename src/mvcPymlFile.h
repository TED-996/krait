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

	bool isDynamic() const override;
	std::string runPyml() const override;
	const IPymlItem* getRootItem() const override;
};
