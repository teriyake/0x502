#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "logger.hpp"

struct ShaderPair {
    std::string name;
    std::string vertexSource;
    std::string fragmentSource;
    bool isValid;
    std::string errorLog;
    
    ShaderPair() : isValid(false) {}
};

struct ShaderSubscription {
    int64_t glibId;
    int shaderIndex;
    bool isValid;
    
    ShaderSubscription() : glibId(-1), shaderIndex(-1), isValid(false) {}
};

class SharedShaderLibrary {
public:
    static SharedShaderLibrary& getInstance() {
        static SharedShaderLibrary instance;
        return instance;
    }
    
    void registerGlib(int64_t glibId) {
        if (glibShaders.find(glibId) == glibShaders.end()) {
            glibShaders[glibId] = std::vector<ShaderPair>();
            //INFO("Registered new Glib %lld in shader library", glibId);
        }
    }
    
    void addShader(int64_t glibId, const ShaderPair& shader) {
        if (glibShaders.find(glibId) != glibShaders.end()) {
            glibShaders[glibId].push_back(shader);
            //INFO("Added shader '%s' to Glib %lld (total shaders: %d)", shader.name.c_str(), glibId, (int)glibShaders[glibId].size());
        } else {
            WARN("Attempted to add shader to unregistered Glib %lld", glibId);
        }
    }
    
    void subscribeToShader(int64_t moduleId, int64_t glibId, int shaderIndex) {
        //INFO("SharedShaderLibrary: Attempting to subscribe module %lld to Glib %lld, shader %d", moduleId, glibId, shaderIndex);
        
        ShaderSubscription sub;
        sub.glibId = glibId;
        sub.shaderIndex = shaderIndex;
        sub.isValid = isValidSubscription(glibId, shaderIndex);
        
        if (sub.isValid) {
            moduleSubscriptions[moduleId] = sub;
            //INFO("SharedShaderLibrary: Successfully subscribed module %lld to Glib %lld, shader %d", moduleId, glibId, shaderIndex);
        } else {
            WARN("SharedShaderLibrary: Invalid subscription attempt: module %lld to Glib %lld, shader %d", 
                moduleId, glibId, shaderIndex);
        }
    }
    
    const ShaderPair* getShaderForModule(int64_t moduleId) {
        auto subIt = moduleSubscriptions.find(moduleId);
        if (subIt == moduleSubscriptions.end()) {
            //INFO("SharedShaderLibrary: No subscription found for module %lld", moduleId);
            return nullptr;
        }
        
        auto& sub = subIt->second;
        auto glibIt = glibShaders.find(sub.glibId);
        if (glibIt == glibShaders.end()) {
            WARN("SharedShaderLibrary: Subscribed Glib %lld not found for module %lld", 
                sub.glibId, moduleId);
            return nullptr;
        }
        
        auto& shaders = glibIt->second;
        if (sub.shaderIndex < 0 || static_cast<size_t>(sub.shaderIndex) >= shaders.size()) {
            WARN("SharedShaderLibrary: Invalid shader index %d for module %lld (Glib %lld has %d shaders)", 
                sub.shaderIndex, moduleId, sub.glibId, (int)shaders.size());
            return nullptr;
        }
        
        //INFO("SharedShaderLibrary: Found shader for module %lld: Glib %lld, shader %d (%s)", moduleId, sub.glibId, sub.shaderIndex, shaders[sub.shaderIndex].name.c_str());
        return &shaders[sub.shaderIndex];
    }
    
    bool isValidSubscription(int64_t glibId, int shaderIndex) {
        auto glibIt = glibShaders.find(glibId);
        if (glibIt == glibShaders.end()) {
            //INFO("SharedShaderLibrary: Invalid subscription: Glib %lld not found", glibId);
            return false;
        }
        
        auto& shaders = glibIt->second;
        bool valid = shaderIndex >= 0 && static_cast<size_t>(shaderIndex) < shaders.size();
        //INFO("SharedShaderLibrary: Subscription validation: Glib %lld, shader %d -> %s", glibId, shaderIndex, valid ? "valid" : "invalid");
        return valid;
    }

    const ShaderSubscription* getSubscription(int64_t moduleId) const {
        auto it = moduleSubscriptions.find(moduleId);
        /*
        if (it != moduleSubscriptions.end()) {
            INFO("SharedShaderLibrary: Found subscription for module %lld: Glib %lld, shader %d", moduleId, it->second.glibId, it->second.shaderIndex);
        } else {
            INFO("SharedShaderLibrary: No subscription found for module %lld", moduleId);
        }
        */
        return it != moduleSubscriptions.end() ? &it->second : nullptr;
    }

    std::vector<int64_t> getGlibIds() const {
        std::vector<int64_t> ids;
        for (const auto& pair : glibShaders) {
            ids.push_back(pair.first);
        }
        //INFO("Retrieved %d Glib IDs", (int)ids.size());
        return ids;
    }

    const std::vector<ShaderPair>* getShadersForGlib(int64_t glibId) const {
        auto it = glibShaders.find(glibId);
        /*
        if (it != glibShaders.end()) {
            INFO("Found %d shaders for Glib %lld", (int)it->second.size(), glibId);
        }
        */
        return it != glibShaders.end() ? &it->second : nullptr;
    }

private:
    SharedShaderLibrary() {
        INFO("SharedShaderLibrary initialized");
    }
    SharedShaderLibrary(const SharedShaderLibrary&) = delete;
    SharedShaderLibrary& operator=(const SharedShaderLibrary&) = delete;
    
    std::map<int64_t, std::vector<ShaderPair>> glibShaders;
    std::map<int64_t, ShaderSubscription> moduleSubscriptions;
}; 
