#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);
	p->addModel(modelRepeat3hr);
	p->addModel(modelXtenuat3hr);
	p->addModel(modelFM3HR);
	p->addModel(modelENVELOOP3HR);
	p->addModel(modelFMFILT3HR);
	p->addModel(modelGLTCH3HR);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}