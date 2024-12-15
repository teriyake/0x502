#include "shader_menu.hpp"
#include "glcv.hpp"
#include "canvas.hpp"

using namespace rack;

GlibMenuItem::GlibMenuItem(Module* module, int64_t glibId, int shaderIndex, const std::string& shaderName)
    : module(module), glibId(static_cast<int64_t>(glibId)), shaderIndex(shaderIndex), shaderName(shaderName) {
    text = shaderName;
    
    auto& shaderLib = SharedShaderLibrary::getInstance();
    const ShaderSubscription* sub = shaderLib.getSubscription(static_cast<int64_t>(module->id));
    /*
    if (sub) {
        INFO("Menu item %s: Module %lld is subscribed to Glib %lld, Shader %d", 
            shaderName.c_str(), (long long)module->id, (long long)sub->glibId, sub->shaderIndex);
    }
    */
    rightText = CHECKMARK(sub && sub->glibId == static_cast<int64_t>(glibId) && sub->shaderIndex == shaderIndex);
}

void GlibMenuItem::onAction(const event::Action& e) {
    int64_t moduleId = static_cast<int64_t>(module->id);
    //INFO("Subscribing module %lld to Glib %lld, shader %d (%s)", (long long)moduleId, (long long)glibId, shaderIndex, shaderName.c_str());
        
    auto& shaderLib = SharedShaderLibrary::getInstance();
    shaderLib.subscribeToShader(moduleId, static_cast<int64_t>(glibId), shaderIndex);
    
    const ShaderSubscription* sub = shaderLib.getSubscription(moduleId);
    if (sub) {
        //INFO("Subscription verified: Module %lld -> Glib %lld, Shader %d, Valid: %d", (long long)moduleId, (long long)sub->glibId, sub->shaderIndex, sub->isValid);
        
        if (auto subscriber = dynamic_cast<ShaderSubscriber*>(module)) {
            subscriber->onShaderSubscribe(static_cast<int64_t>(glibId), shaderIndex);
        }
    } else {
        WARN("Failed to verify subscription for module %lld", (long long)moduleId);
    }
}

GlibSubmenu::GlibSubmenu(Module* module, int64_t glibId, const std::string& name, const std::vector<ShaderPair>* shaders)
    : module(module), glibId(glibId), shaders(shaders) {
    text = name;
    
    if (shaders) {
        text += string::f(" (%d shaders)", (int)shaders->size());
    }
}

Menu* GlibSubmenu::createChildMenu() {
    Menu* menu = new Menu;
    
    if (shaders) {
        for (size_t i = 0; i < shaders->size(); i++) {
            const auto& shader = (*shaders)[i];
            menu->addChild(new GlibMenuItem(module, glibId, i, shader.name));
        }
    }
    
    return menu;
}

void addShaderMenuItems(Menu* menu, Module* module) {
    auto& shaderLib = SharedShaderLibrary::getInstance();
    
    std::vector<int64_t> glibIds = shaderLib.getGlibIds();
    if (glibIds.empty()) {
        menu->addChild(createMenuLabel("No Glib modules available"));
        return;
    }
    
    //INFO("Building menu for module %lld, found %d Glibs", (long long)module->id, (int)glibIds.size());
    
    for (int64_t glibId : glibIds) {
        const auto* shaders = shaderLib.getShadersForGlib(glibId);
        if (shaders) {
            std::string glibName = "Glib " + std::to_string(glibId);
            //INFO("  Adding Glib %lld with %d shaders", (long long)glibId, (int)shaders->size());
            menu->addChild(new GlibSubmenu(module, glibId, glibName, shaders));
        }
    }
} 
