#include "plugin.hpp"
#include "gl_utils.hpp"
#include "shader_manager.hpp"
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <osdialog.h>

bool hasExtension(const std::string& filename, const std::string& ext) {
    if (filename.length() < ext.length()) return false;
    return filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0;
}

std::string getStem(const std::string& filename) {
    size_t lastDot = filename.find_last_of('.');
    if (lastDot == std::string::npos) return filename;
    return filename.substr(0, lastDot);
}

bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

struct Glib : Module {
    enum ParamId {
        PARAM_LOAD,
        PARAM_PREV,
        PARAM_NEXT,
        PARAMS_LEN
    };
    enum InputId {
        INPUT_PREV,
        INPUT_NEXT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUT_VERT,
        OUTPUT_FRAG,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHT_LOADED,
        LIGHT_VERT_VALID,
        LIGHT_FRAG_VALID,
        LIGHTS_LEN
    };

    std::vector<ShaderPair> shaderLibrary;
    int currentShaderIndex = 0;
    dsp::SchmittTrigger prevTrigger;
    dsp::SchmittTrigger nextTrigger;
    dsp::SchmittTrigger loadTrigger;
    bool needsValidation = false;
    bool requestFileDialog = false;

    Glib() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configButton(PARAM_PREV, "previous shader");
        configButton(PARAM_NEXT, "next shader");
        configButton(PARAM_LOAD, "load shader");
        configInput(INPUT_PREV, "previous trigger");
        configInput(INPUT_NEXT, "next trigger");
        configOutput(OUTPUT_VERT, "vertex shader");
        configOutput(OUTPUT_FRAG, "fragment shader");
    }

    void onAdd() override {
        auto& shaderLib = SharedShaderLibrary::getInstance();
        int64_t glibId = static_cast<int64_t>(id);
        shaderLib.registerGlib(glibId);
        //INFO("Registered Glib module %lld with shader library (actual ID)", (long long)glibId);
    }

    void loadShaderFromPath(const std::string& vertPath) {
        std::string baseName = getStem(vertPath);
        std::string fragPath = vertPath.substr(0, vertPath.length() - 5) + ".frag";
        
        /*
        INFO("Loading shader pair: %s for Glib %d", baseName.c_str(), id);
        INFO("  Vertex shader: %s", vertPath.c_str());
        INFO("  Fragment shader: %s", fragPath.c_str());
        */
        
        if (fileExists(fragPath)) {
            ShaderPair pair;
            pair.name = baseName;
            pair.vertexSource = gl::readShaderFile(vertPath);
            pair.fragmentSource = gl::readShaderFile(fragPath);
            
            pair.isValid = !pair.vertexSource.empty() && !pair.fragmentSource.empty();
            if (!pair.isValid) {
                WARN("Failed to load shader sources for %s", baseName.c_str());
                if (pair.vertexSource.empty()) WARN("  Vertex shader is empty");
                if (pair.fragmentSource.empty()) WARN("  Fragment shader is empty");
                return;
            }
            
            shaderLibrary.push_back(pair);
            currentShaderIndex = shaderLibrary.size() - 1;
            needsValidation = true;
            
            auto& shaderLib = SharedShaderLibrary::getInstance();
            shaderLib.addShader(id, pair);
            
            //INFO("Successfully loaded shader pair: %s", baseName.c_str());
            //INFO("  Added to Glib %d at index %d", id, (int)shaderLibrary.size() - 1);
            
            /*
            auto ids = shaderLib.getGlibIds();
            INFO("Current Glibs in library:");
            for (int glibId : ids) {
                const auto* shaders = shaderLib.getShadersForGlib(glibId);
                if (shaders) {
                    INFO("  Glib %d: %d shaders", glibId, (int)shaders->size());
                }
            }
            */
        } else {
            WARN("Could not find matching fragment shader for %s", baseName.c_str());
        }
    }

    void validateShaderPair(ShaderPair& pair) {
        try {
            pair.isValid = !pair.vertexSource.empty() && !pair.fragmentSource.empty();
            
            if (pair.isValid) {
                //INFO("Validated shader pair: %s", pair.name.c_str());
            } else {
                WARN("Invalid shader pair: %s", pair.name.c_str());
                if (pair.vertexSource.empty()) WARN("  Vertex shader is empty");
                if (pair.fragmentSource.empty()) WARN("  Fragment shader is empty");
            }
        }
        catch (const std::exception& e) {
            WARN("Shader validation error: %s", e.what());
            pair.isValid = false;
            pair.errorLog = e.what();
        }
    }

    void process(const ProcessArgs& args) override {
        if (prevTrigger.process(params[PARAM_PREV].getValue() || inputs[INPUT_PREV].getVoltage() > 1.0f)) {
            if (currentShaderIndex > 0) {
                currentShaderIndex--;
            }
        }
        if (nextTrigger.process(params[PARAM_NEXT].getValue() || inputs[INPUT_NEXT].getVoltage() > 1.0f)) {
            if (currentShaderIndex < (int)shaderLibrary.size() - 1) {
                currentShaderIndex++;
            }
        }

        if (loadTrigger.process(params[PARAM_LOAD].getValue())) {
            requestFileDialog = true;
        }

        if (needsValidation && !shaderLibrary.empty()) {
            for (auto& pair : shaderLibrary) {
                validateShaderPair(pair);
            }
            needsValidation = false;
        }

        if (!shaderLibrary.empty()) {
            ShaderPair& current = shaderLibrary[currentShaderIndex];
            lights[LIGHT_LOADED].setBrightness(1.0f);
            lights[LIGHT_VERT_VALID].setBrightness(current.isValid ? 1.0f : 0.0f);
            lights[LIGHT_FRAG_VALID].setBrightness(current.isValid ? 1.0f : 0.0f);
            outputs[OUTPUT_VERT].setVoltage(current.isValid ? 10.0f : 0.0f);
            outputs[OUTPUT_FRAG].setVoltage(current.isValid ? 10.0f : 0.0f);
        } else {
            lights[LIGHT_LOADED].setBrightness(0.0f);
            lights[LIGHT_VERT_VALID].setBrightness(0.0f);
            lights[LIGHT_FRAG_VALID].setBrightness(0.0f);
            outputs[OUTPUT_VERT].setVoltage(0.0f);
            outputs[OUTPUT_FRAG].setVoltage(0.0f);
        }
    }
};

struct GlibWidget : ModuleWidget {
	Glib* module;

	GlibWidget(Glib* module) {
		this->module = module;
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/glib.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<VCVButton>(mm2px(Vec(20.32, 42.488)), module, Glib::PARAM_LOAD));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(10.478, 72.486)), module, Glib::PARAM_PREV));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(30.162, 72.486)), module, Glib::PARAM_NEXT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.478, 93.927)), module, Glib::INPUT_PREV));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.162, 93.927)), module, Glib::INPUT_NEXT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.478, 115.863)), module, Glib::OUTPUT_VERT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.162, 115.863)), module, Glib::OUTPUT_FRAG));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(20.32, 53.111)), module, Glib::LIGHT_LOADED));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(13.124, 85.176)), module, Glib::LIGHT_VERT_VALID));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(27.516, 85.176)), module, Glib::LIGHT_FRAG_VALID));
	}

	void step() override {
		if (module) {
			if (module->requestFileDialog) {
				module->requestFileDialog = false;
				openFileDialog();
			}
		}
		ModuleWidget::step();
	}

	void openFileDialog() {
		osdialog_filters* filters = osdialog_filters_parse("Vertex Shader:vert");
		char* path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, filters);
		osdialog_filters_free(filters);
		
		if (path) {
			module->loadShaderFromPath(path);
			free(path);
		}
	}
};

Model* modelGlib = createModel<Glib, GlibWidget>("glib");
