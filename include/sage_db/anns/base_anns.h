#pragma once

#include "common.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace sage_db {
namespace anns {

// Forward declarations
class ANNSIndex;
class ANNSBuilder;

/**
 * @brief Performance metrics collected during operations
 */
struct ANNSMetrics {
    double build_time_seconds = 0.0;
    double search_time_seconds = 0.0;
    size_t memory_usage_bytes = 0;
    size_t distance_computations = 0;
    
    // Algorithm-specific metrics
    std::unordered_map<std::string, double> additional_metrics;
    
    void reset() {
        build_time_seconds = 0.0;
        search_time_seconds = 0.0;
        memory_usage_bytes = 0;
        distance_computations = 0;
        additional_metrics.clear();
    }
};

/**
 * @brief Algorithm parameters for index building
 */
struct ANNSBuildParams {
    // Common parameters
    uint32_t num_threads = 1;
    bool verbose = false;
    
    // Algorithm-specific parameters stored as key-value pairs
    std::unordered_map<std::string, std::string> algorithm_params;
    
    // Helper methods to get typed parameters
    template<typename T>
    T get_param(const std::string& key, const T& default_value) const;
    
    template<typename T>
    void set_param(const std::string& key, const T& value);
};

/**
 * @brief Query parameters for search operations
 */
struct ANNSQueryParams {
    uint32_t k = 10;                    // Number of nearest neighbors
    uint32_t ef = 50;                   // Search parameter (for HNSW-like algorithms)
    uint32_t nprobe = 1;                // Number of clusters to search (for IVF)
    float radius = -1.0f;               // Radius for range search (if > 0)
    bool include_distances = true;       // Whether to return distances
    
    // Algorithm-specific parameters
    std::unordered_map<std::string, std::string> algorithm_params;
    
    // Helper methods
    template<typename T>
    T get_param(const std::string& key, const T& default_value) const;
    
    template<typename T>
    void set_param(const std::string& key, const T& value);
};

/**
 * @brief Base interface for all ANNS algorithms
 * 
 * This interface is designed to be compatible with big-ann-benchmarks
 * and provides a unified API for different ANNS algorithms.
 */
class BaseANNS {
public:
    virtual ~BaseANNS() = default;
    
    /**
     * @brief Get the algorithm name
     */
    virtual std::string algorithm_name() const = 0;
    
    /**
     * @brief Get the algorithm version
     */
    virtual std::string algorithm_version() const = 0;
    
    /**
     * @brief Get supported distance metrics
     */
    virtual std::vector<DistanceMetric> supported_metrics() const = 0;
    
    /**
     * @brief Check if the algorithm supports the given metric
     */
    virtual bool supports_metric(DistanceMetric metric) const = 0;
    
    /**
     * @brief Initialize the algorithm with configuration
     */
    virtual void initialize(const DatabaseConfig& config) = 0;
    
    /**
     * @brief Build index from training data
     * @param data Training vectors
     * @param params Build parameters
     * @return Build metrics
     */
    virtual ANNSMetrics fit(const std::vector<Vector>& data, 
                           const ANNSBuildParams& params = {}) = 0;
    
    /**
     * @brief Load pre-built index from file
     * @param filepath Path to index file
     * @return true if successful, false otherwise
     */
    virtual bool load_index(const std::string& filepath) = 0;
    
    /**
     * @brief Save index to file
     * @param filepath Path to save index
     * @return true if successful, false otherwise
     */
    virtual bool save_index(const std::string& filepath) const = 0;
    
    /**
     * @brief Check if index is ready for queries
     */
    virtual bool is_trained() const = 0;
    
    /**
     * @brief Search for k nearest neighbors
     * @param query Query vector
     * @param params Query parameters
     * @return Search results with metrics
     */
    virtual std::pair<std::vector<QueryResult>, ANNSMetrics> 
    search(const Vector& query, const ANNSQueryParams& params = {}) = 0;
    
    /**
     * @brief Batch search for multiple queries
     * @param queries Query vectors
     * @param params Query parameters
     * @return Batch search results with metrics
     */
    virtual std::pair<std::vector<std::vector<QueryResult>>, ANNSMetrics>
    batch_search(const std::vector<Vector>& queries, 
                const ANNSQueryParams& params = {}) = 0;
    
    /**
     * @brief Range search within given radius
     * @param query Query vector
     * @param radius Search radius
     * @param params Query parameters
     * @return Range search results with metrics
     */
    virtual std::pair<std::vector<QueryResult>, ANNSMetrics>
    range_search(const Vector& query, float radius, 
                const ANNSQueryParams& params = {}) = 0;
    
    /**
     * @brief Add vectors to index (if supported)
     * @param vectors Vectors to add
     * @return Vector IDs of added vectors
     */
    virtual std::vector<VectorId> add_vectors(const std::vector<Vector>& vectors) {
        throw std::runtime_error("Dynamic insertion not supported by " + algorithm_name());
    }
    
    /**
     * @brief Remove vectors from index (if supported)
     * @param ids Vector IDs to remove
     * @return true if successful
     */
    virtual bool remove_vectors(const std::vector<VectorId>& ids) {
        throw std::runtime_error("Dynamic deletion not supported by " + algorithm_name());
    }
    
    /**
     * @brief Get index statistics
     */
    virtual std::unordered_map<std::string, double> get_index_stats() const = 0;
    
    /**
     * @brief Get current memory usage in bytes
     */
    virtual size_t get_memory_usage() const = 0;
    
    /**
     * @brief Get algorithm-specific configuration
     */
    virtual std::unordered_map<std::string, std::string> get_algorithm_config() const = 0;
    
    /**
     * @brief Validate configuration before building
     */
    virtual bool validate_config(const DatabaseConfig& config, 
                                const ANNSBuildParams& params) const = 0;
    
protected:
    // Helper method for timing operations
    template<typename Func>
    std::pair<typename std::result_of<Func()>::type, double> 
    time_operation(Func&& func) const;
};

/**
 * @brief Factory interface for creating ANNS algorithms
 */
class ANNSFactory {
public:
    virtual ~ANNSFactory() = default;
    
    /**
     * @brief Create an instance of the ANNS algorithm
     */
    virtual std::unique_ptr<BaseANNS> create() const = 0;
    
    /**
     * @brief Get algorithm name
     */
    virtual std::string algorithm_name() const = 0;
    
    /**
     * @brief Get algorithm description
     */
    virtual std::string algorithm_description() const = 0;
    
    /**
     * @brief Get default build parameters
     */
    virtual ANNSBuildParams default_build_params() const = 0;
    
    /**
     * @brief Get default query parameters
     */
    virtual ANNSQueryParams default_query_params() const = 0;
};

/**
 * @brief Registry for ANNS algorithms
 */
class ANNSRegistry {
public:
    static ANNSRegistry& instance();
    
    /**
     * @brief Register an algorithm factory
     */
    void register_algorithm(const std::string& name, 
                           std::unique_ptr<ANNSFactory> factory);
    
    /**
     * @brief Create an algorithm instance by name
     */
    std::unique_ptr<BaseANNS> create_algorithm(const std::string& name) const;
    
    /**
     * @brief Get list of available algorithms
     */
    std::vector<std::string> get_available_algorithms() const;
    
    /**
     * @brief Check if algorithm is available
     */
    bool is_algorithm_available(const std::string& name) const;
    
    /**
     * @brief Get algorithm factory by name
     */
    const ANNSFactory* get_factory(const std::string& name) const;

private:
    ANNSRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<ANNSFactory>> factories_;
};

// Template implementations
template<typename T>
T ANNSBuildParams::get_param(const std::string& key, const T& default_value) const {
    auto it = algorithm_params.find(key);
    if (it == algorithm_params.end()) {
        return default_value;
    }
    
    // Type conversion logic here
    if constexpr (std::is_same_v<T, int>) {
        return std::stoi(it->second);
    } else if constexpr (std::is_same_v<T, float>) {
        return std::stof(it->second);
    } else if constexpr (std::is_same_v<T, double>) {
        return std::stod(it->second);
    } else if constexpr (std::is_same_v<T, bool>) {
        return it->second == "true" || it->second == "1";
    } else if constexpr (std::is_same_v<T, std::string>) {
        return it->second;
    } else {
        return default_value;
    }
}

template<typename T>
void ANNSBuildParams::set_param(const std::string& key, const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
        algorithm_params[key] = value;
    } else {
        algorithm_params[key] = std::to_string(value);
    }
}

template<typename T>
T ANNSQueryParams::get_param(const std::string& key, const T& default_value) const {
    auto it = algorithm_params.find(key);
    if (it == algorithm_params.end()) {
        return default_value;
    }
    
    // Type conversion logic (same as ANNSBuildParams)
    if constexpr (std::is_same_v<T, int>) {
        return std::stoi(it->second);
    } else if constexpr (std::is_same_v<T, float>) {
        return std::stof(it->second);
    } else if constexpr (std::is_same_v<T, double>) {
        return std::stod(it->second);
    } else if constexpr (std::is_same_v<T, bool>) {
        return it->second == "true" || it->second == "1";
    } else if constexpr (std::is_same_v<T, std::string>) {
        return it->second;
    } else {
        return default_value;
    }
}

template<typename T>
void ANNSQueryParams::set_param(const std::string& key, const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
        algorithm_params[key] = value;
    } else {
        algorithm_params[key] = std::to_string(value);
    }
}

template<typename Func>
std::pair<typename std::result_of<Func()>::type, double> 
BaseANNS::time_operation(Func&& func) const {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = func();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double seconds = duration.count() / 1000000.0;
    
    return std::make_pair(std::move(result), seconds);
}

} // namespace anns
} // namespace sage_db