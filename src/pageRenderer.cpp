#include "pageRenderer.h"


ResponseRenderer::ResponseRenderer(CacheController& cacheController, std::string )
	: cacheController(cacheController) {
}


bool ResponseRenderer::render(const IPymlFile& source, std::string filename, Response* destination) {
	bool isDynamic = source.isDynamic();

	CacheController::CachePragma cachePragma = cacheController.getCacheControl(filename, !isDynamic);

}
