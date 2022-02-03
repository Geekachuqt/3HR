#include "plugin.hpp"
using simd::float_4;

template <typename T> struct Xtenuat3hrCore{

float_4 Mix = 0;

	void Advance(float_4 CrossAmount, float_4 InputA, float_4 InputB){
		Mix = InputA*(1-CrossAmount) + InputB*(CrossAmount);
	}
	float_4 GetMix(){
		return Mix;
	}
};

struct Xtenuat3hr : Module {
	enum ParamId {
		CROSS_PARAM,
		CROSS_CV_PARAM,
		ATTEN_PARAM,
		ATTEN_CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		B_INPUT,
		CROSS_CV_INPUT,
		ATTEN_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Xtenuat3hr() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CROSS_PARAM, 0.f, 1.f, 0.5f, "Crossfade");
		configParam(CROSS_CV_PARAM, 0.f, 1.f, 0.f, "Crossfade CV");
		configParam(ATTEN_PARAM, -1.f, 1.f, 0.f, "Attenuverter");
		configParam(ATTEN_CV_PARAM, 0.f, 1.f, 0.f, "Attenuverter CV");
		configInput(A_INPUT, "Audio Signal A");
		configInput(B_INPUT, "Audio Signal B");
		configInput(CROSS_CV_INPUT, "Crossfade CV");
		configInput(ATTEN_CV_INPUT, "Attenuverter CV");
		configOutput(AUDIO_OUTPUT, "Audio");
	}

	float CrossKnob = 0;
	float CrossCV = 0;
	float AttenKnob = 0;
	float AttenCV = 0;

	float_4 Mix = 0;

	float_4 CrossAmount = 0;
	float_4 AttenAmount = 0;

	float Channels;

	Xtenuat3hrCore<float_4> xtenuat3hrCore[4];

	void process(const ProcessArgs& args) override {
		Channels = std::max(inputs[A_INPUT].getChannels(),inputs[B_INPUT].getChannels());
		Channels = std::max(Channels,1.f);
		outputs[AUDIO_OUTPUT].setChannels(Channels);
		for(int c = 0; c < Channels; c+=4){
			Collect(c);
			xtenuat3hrCore[c/4].Advance(CrossAmount,inputs[A_INPUT].getPolyVoltageSimd<float_4>(c),inputs[B_INPUT].getPolyVoltageSimd<float_4>(c));
			float_4 audio_in  = xtenuat3hrCore[c/4].GetMix();
			float_4 audio_out;
			for(int c = 0; c <= 3; c++){
				audio_out[c] = (AttenAmount[c] < 0) ? abs(audio_in[c]-10)*abs(AttenAmount[c]) : audio_in[c]*AttenAmount[c];	
			}
			outputs[AUDIO_OUTPUT].setVoltageSimd(audio_out,c);
		}
	}
	void Collect(int c){
		CrossKnob = params[CROSS_PARAM].getValue();
		CrossCV = params[CROSS_CV_PARAM].getValue();
		AttenKnob = params[ATTEN_PARAM].getValue();
		AttenCV = params[ATTEN_CV_PARAM].getValue();

		CrossAmount = clamp(CrossKnob + CrossCV * inputs[CROSS_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.1F,0.f,1.f);
		AttenAmount = clamp(AttenKnob + AttenCV * inputs[ATTEN_CV_INPUT].getPolyVoltageSimd<float_4>(c)*0.2F,-1.f,1.f);
	}
};



struct Xtenuat3hrWidget : ModuleWidget {
	Xtenuat3hrWidget(Xtenuat3hr* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Xtenuat3hr.svg")));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.08, 30.403)), module, Xtenuat3hr::CROSS_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(5.08, 40.403)), module, Xtenuat3hr::CROSS_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.08, 82.403)), module, Xtenuat3hr::ATTEN_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(5.08, 92.403)), module, Xtenuat3hr::ATTEN_CV_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 10.403)), module, Xtenuat3hr::A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 20.403)), module, Xtenuat3hr::B_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(5.08, 50.403)), module, Xtenuat3hr::CROSS_CV_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(5.08, 102.403)), module, Xtenuat3hr::ATTEN_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 112.403)), module, Xtenuat3hr::AUDIO_OUTPUT));
	}
};


Model* modelXtenuat3hr = createModel<Xtenuat3hr, Xtenuat3hrWidget>("Xtenuat3hr");