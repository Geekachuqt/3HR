#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;

extern Model* modelRepeat3hr;
extern Model* modelXtenuat3hr;
extern Model* modelFM3HR;
extern Model* modelENVELOOP3HR;
extern Model* modelFMFILT3HR;

// JACKS

struct TinyJack : app::SvgPort {
	TinyJack() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyJack.svg")));
	}
};
// KNOBS

struct Tiny3HRCVPot : app::SvgKnob {
	Tiny3HRCVPot() {
		minAngle = -0.75 * M_PI;
		maxAngle = 0.75 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Tiny3HRCVPot.svg")));
	}
};
