#pragma once
#include "plugin.hpp"
#include "shader_manager.hpp"
#include "gl_utils.hpp"
#include "glib.hpp"
#include "shader_menu.hpp"

struct Glab;

struct GlabTextField : LedDisplayTextField {
	float scrollPos = 0.f;
	const float lineHeight = 12.f;
	bool isFocused = false;
	int selectionStart = -1;
	int selectionEnd = -1;
	bool isDragging = false;
    Vec dragPos;
	
	void draw(const DrawArgs& args) override;
	void onSelectKey(const event::SelectKey& e) override;
	void onHoverScroll(const event::HoverScroll& e) override;
	void onButton(const event::Button& e) override;
	void onDragMove(const event::DragMove& e) override;
	void insertSpaces(int numSpaces);
	float getContentHeight();
	void onSelect(const event::Select& e) override { isFocused = true; }
	void onDeselect(const event::Deselect& e) override { isFocused = false; }
	int getTextPosition(math::Vec mousePos) override;
    //std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
    std::string fontPath = asset::plugin(pluginInstance, "res/fonts/SpaceMono-Regular.ttf");
    NVGcolor textColor = nvgRGB(0x8E, 0xC0, 0x7C);
	
protected:
	void drawLayer(const DrawArgs& args, int layer) override;
	std::vector<std::string> wrapText(const std::string& text, float maxWidth);
};

struct GlabVertexField : GlabTextField {
	Glab* module;
	void step() override;
	void onChange(const ChangeEvent& e) override;
};

struct GlabFragmentField : GlabTextField {
	Glab* module;
	void step() override;
	void onChange(const ChangeEvent& e) override;
};

struct GlabErrorField : GlabTextField {
	Glab* module;
	void step() override;
};

struct GlabDisplay : LedDisplay {
	void setModule(Glab* module, bool isVertex);
};

struct GlabErrorDisplay : LedDisplay {
	void setModule(Glab* module);
};

struct Glab : Module, ShaderSubscriber {
	enum ParamId {
		PARAM_COMP_V,
		PARAM_COMP_F,
		PARAM_PUBLISH,
		PARAMS_LEN
	};
	enum InputId {
		INPUT_L,
		INPUT_R,
		INPUT_CV_1,
		INPUT_CV_2,
		INPUT_CV_3,
		INPUT_CV_4,
		INPUT_COMP_V,
		INPUT_COMP_F,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_VERT,
		OUTPUT_FRAG,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHT_COMP_V,
		LIGHT_COMP_F,
		ENUMS(LIGHT_PUBLISH, 3),
		LIGHTS_LEN
	};

	std::string vertexShaderText;
	std::string fragmentShaderText;
	std::string errorText;
	bool vertexShaderValid = false;
	bool fragmentShaderValid = false;
	bool isDirty = false;
	ShaderSubscription subscription;
	dsp::SchmittTrigger compileVertexTrigger;
	dsp::SchmittTrigger compileFragTrigger;
	dsp::SchmittTrigger publishTrigger;
	
	float audioL = 0.f;
	float audioR = 0.f;
	float cv[4] = {0.f, 0.f, 0.f, 0.f};
	
	bool requestVertexCompile = false;
	bool requestFragmentCompile = false;
	
	bool wasPublishPressed = false;

	Glab();
	void process(const ProcessArgs& args) override;
	void onReset() override;
	void updatePublishLight();
	void publishToGlib();
	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
	void onShaderSubscribe(int64_t glibId, int shaderIndex) override;
};

struct GlabWidget : ModuleWidget {
	void step() override;
	void appendContextMenu(Menu* menu) override;
	GlabWidget(Glab* module);
};
