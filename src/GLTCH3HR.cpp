#include "plugin.hpp"
using simd::float_4;

struct GLTCH3HR : Module {
	enum ParamId {
		END_CV_PARAM_PARAM,
		START_CV_PARAM_PARAM,
		END_PARAM_PARAM,
		START_PARAM_PARAM,
		THRESHOLD_PARAM_PARAM,
		SMOOTHEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_IN_INPUT,
		END_CV_IN_INPUT,
		START_CV_IN_INPUT,
		THRESHOLD_IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ACTIVE_LIGHT_LIGHT,
		LIGHTS_LEN
	};

	GLTCH3HR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(END_CV_PARAM_PARAM, 0.f, 1.f, 0.f, "End CV");
		configParam(START_CV_PARAM_PARAM, 0.f, 1.f, 0.f, "Start CV");
		configParam(END_PARAM_PARAM, 0.01f, 1.f, 0.1f, "End");
		configParam(START_PARAM_PARAM, 0.f, 1.f, 0.f, "Start");
		configParam(THRESHOLD_PARAM_PARAM, 0.f, 10.f, 1.f, "Threshold");
		configParam(SMOOTHEN_PARAM, 0.f, 1.f, 0.1f, "Smoothen");
		configInput(AUDIO_IN_INPUT, "Signal");
		configInput(END_CV_IN_INPUT, "End CV");
		configInput(START_CV_IN_INPUT, "Start CV");
		configInput(THRESHOLD_IN_INPUT, "Threshold signal");
		configOutput(AUDIO_OUT_OUTPUT, "Signal");
	}

	std::vector<float> PlayBuffer = {};
	float Threshold = 0;
	float ThresholdInput = 0;
	float StartPoint = 0;
	float EndPoint = 0;
	float Length = 0;
	float Direction = 1;
	float PlayCounter = 0;
	float BufferCounter = 0;
	float WaitSamples = 0;
	float PlayHead = 0;
	float Output = 0;
	float EndPointBuffer = 0;
	float CrossSamples = 1;
	float CrossCounter = 0;
	float FadeOutSamples = 0;
	float FadeOutCounter = 0;
	float CrossStart = 0;
	float Smoothen = 0;
	enum RecordingStates{IDLE, STARTED, RECORDING, CROSSTART, CROSSING, PLAYING, ENDING, RESET};
	RecordingStates RecordingState = IDLE;
	bool Initalized = false;

	void process(const ProcessArgs& args) override {
			if(Initalized == false){
			PlayBuffer.resize(args.sampleRate*0.5f);
			Initalized = true;
		}

		Smoothen = params[SMOOTHEN_PARAM].getValue();
		Threshold = params[THRESHOLD_PARAM_PARAM].getValue();
		ThresholdInput = inputs[THRESHOLD_IN_INPUT].getVoltage();
		StartPoint = inputs[START_CV_IN_INPUT].isConnected() ? clamp((params[START_PARAM_PARAM].getValue()+params[START_CV_PARAM_PARAM].getValue()*inputs[START_CV_IN_INPUT].getVoltage()*0.1f),0.f,1.f)*args.sampleRate*0.5f : params[START_PARAM_PARAM].getValue()*args.sampleRate*0.5f;
		EndPoint = inputs[END_CV_IN_INPUT].isConnected() ? clamp((params[END_PARAM_PARAM].getValue()+params[END_CV_PARAM_PARAM].getValue()*inputs[END_CV_IN_INPUT].getVoltage()*0.1f),0.01f,1.f)*args.sampleRate*0.5f : params[END_PARAM_PARAM].getValue()*args.sampleRate*0.5f;
		Length = abs(StartPoint - EndPoint);
		Direction = (Length/(StartPoint - EndPoint))*-1;

		if(ThresholdInput > Threshold){
			lights[ACTIVE_LIGHT_LIGHT].setBrightness(1.f);
		}
		else{
			lights[ACTIVE_LIGHT_LIGHT].setBrightnessSmooth(0.f,args.sampleTime);
		}

		if(RecordingState != IDLE && BufferCounter < PlayBuffer.size()){
			PlayBuffer[BufferCounter] = inputs[AUDIO_IN_INPUT].getVoltage();
			BufferCounter++;
		}
		switch(RecordingState){
			case IDLE:
				if(ThresholdInput > Threshold && RecordingState == IDLE){
						RecordingState = STARTED;
				}

				break;

			case STARTED:
				CrossSamples = round(1+3000*Smoothen);
				WaitSamples = (StartPoint+Length);
				PlayHead = StartPoint;
				RecordingState = RECORDING;
				EndPointBuffer = EndPoint;

				break;

			case RECORDING:

				PlayCounter++;

				if(PlayCounter >= WaitSamples){
					RecordingState = CROSSTART;
				}

				break;

			case CROSSTART:

				CrossCounter = 0;
				CrossStart = Output;
				RecordingState = CROSSING;

				break;

			case CROSSING:

				Output = PlayBuffer[PlayHead]*(CrossCounter/CrossSamples)+CrossStart*((CrossSamples-CrossCounter)/CrossSamples);
				PlayHead = clamp(PlayHead+Direction,0.f,args.sampleRate*0.5f);

				if(CrossCounter != CrossSamples){
					CrossCounter++;
				}
				else{
					CrossCounter++;
					RecordingState = PLAYING;
				}

				break;
			
			case PLAYING:

				Output = PlayBuffer[PlayHead];
				PlayHead = clamp(PlayHead+Direction,0.f,args.sampleRate*0.5f);

				if(Direction < 0){
					if(PlayHead <= EndPointBuffer){
						PlayHead = StartPoint;
						EndPointBuffer = EndPoint;
						RecordingState = CROSSTART;
						FadeOutSamples = abs(EndPoint - StartPoint);
					}
				}
				
				if(Direction > 0){
					if(PlayHead >= EndPointBuffer){
						PlayHead = StartPoint;
						EndPointBuffer = EndPoint;
						RecordingState = CROSSTART;
						FadeOutSamples = abs(EndPoint - StartPoint);
					}
				}

				if(ThresholdInput < Threshold){
					RecordingState = ENDING;
				}

				break;

			case ENDING:

				Output = PlayBuffer[PlayHead]*((FadeOutSamples-FadeOutCounter)/FadeOutSamples);
				PlayHead = clamp(PlayHead+Direction,0.f,args.sampleRate*0.5f);

				FadeOutCounter++;
				
				if(Direction < 0){
					if(PlayHead <= EndPoint){
						RecordingState = RESET;
					}
				}
				if(Direction > 0){
					if(PlayHead >= EndPoint){
						RecordingState = RESET;
					}
				}

				break;

			case RESET:
				PlayBuffer.clear();
				PlayBuffer.resize(args.sampleRate*0.5f);
				PlayHead = 0;
				Output = 0;
				BufferCounter = 0;
				FadeOutCounter = 0;
				RecordingState = IDLE;

				break;

		}

		outputs[AUDIO_OUT_OUTPUT].setVoltage(clamp(Output,-10.f,10.f));

	}
};

struct GLTCH3HRWidget : ModuleWidget {
	GLTCH3HRWidget(GLTCH3HR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GLTCH3HR.svg")));

		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(19.009, 41.612)), module, GLTCH3HR::END_CV_PARAM_PARAM));
		addParam(createParamCentered<Tiny3HRCVPot>(mm2px(Vec(11.464, 41.612)), module, GLTCH3HR::START_CV_PARAM_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(25.464, 44.316)), module, GLTCH3HR::END_PARAM_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.009, 44.316)), module, GLTCH3HR::START_PARAM_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.237, 77.386)), module, GLTCH3HR::THRESHOLD_PARAM_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.237, 90.386)), module, GLTCH3HR::SMOOTHEN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.237, 27.614)), module, GLTCH3HR::AUDIO_IN_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(18.981, 47.397)), module, GLTCH3HR::END_CV_IN_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(11.492, 47.397)), module, GLTCH3HR::START_CV_IN_INPUT));
		addInput(createInputCentered<TinyJack>(mm2px(Vec(15.237, 65.462)), module, GLTCH3HR::THRESHOLD_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.237, 111.343)), module, GLTCH3HR::AUDIO_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.237, 56.378)), module, GLTCH3HR::ACTIVE_LIGHT_LIGHT));
	}
};


Model* modelGLTCH3HR = createModel<GLTCH3HR, GLTCH3HRWidget>("GLTCH3HR");