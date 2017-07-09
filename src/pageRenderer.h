#pragma once
#include "IResponseRenderer.h"
#include "cacheController.h"

class ResponseRenderer : public IResponseRenderer
{
private:
	CacheController& cacheController;

public:
	explicit ResponseRenderer(CacheController& cacheController);

	bool render(const IPymlFile& source, std::string filename, Response* destination) override;
};
