#pragma once
#include "shader_manager.hpp"
#include "rack.hpp"

struct Glcv;
struct Canvas;

class ShaderSubscriber {
public:
    virtual void onShaderSubscribe(int64_t glibId, int shaderIndex) = 0;
    virtual ~ShaderSubscriber() = default;
};

struct GlibMenuItem : rack::MenuItem {
    rack::Module* module;
    int64_t glibId;
    int shaderIndex;
    std::string shaderName;
    
    GlibMenuItem(rack::Module* module, int64_t glibId, int shaderIndex, const std::string& shaderName);
    void onAction(const rack::event::Action& e) override;
};

struct GlibSubmenu : rack::MenuItem {
    rack::Module* module;
    int64_t glibId;
    const std::vector<ShaderPair>* shaders;
    
    GlibSubmenu(rack::Module* module, int64_t glibId, const std::string& name, const std::vector<ShaderPair>* shaders);
    rack::Menu* createChildMenu() override;
};

void addShaderMenuItems(rack::Menu* menu, rack::Module* module); 
