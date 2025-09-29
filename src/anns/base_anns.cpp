#include "sage_db/anns/base_anns.h"
#include <chrono>
#include <stdexcept>

namespace sage_db {
namespace anns {

// ANNSRegistry implementation
ANNSRegistry& ANNSRegistry::instance() {
    static ANNSRegistry instance;
    return instance;
}

void ANNSRegistry::register_algorithm(const std::string& name, 
                                     std::unique_ptr<ANNSFactory> factory) {
    if (factories_.find(name) != factories_.end()) {
        throw std::runtime_error("Algorithm '" + name + "' is already registered");
    }
    factories_[name] = std::move(factory);
}

std::unique_ptr<BaseANNS> ANNSRegistry::create_algorithm(const std::string& name) const {
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        throw std::runtime_error("Algorithm '" + name + "' is not registered");
    }
    return it->second->create();
}

std::vector<std::string> ANNSRegistry::get_available_algorithms() const {
    std::vector<std::string> algorithms;
    algorithms.reserve(factories_.size());
    
    for (const auto& pair : factories_) {
        algorithms.push_back(pair.first);
    }
    
    return algorithms;
}

bool ANNSRegistry::is_algorithm_available(const std::string& name) const {
    return factories_.find(name) != factories_.end();
}

const ANNSFactory* ANNSRegistry::get_factory(const std::string& name) const {
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        return nullptr;
    }
    return it->second.get();
}

} // namespace anns
} // namespace sage_db