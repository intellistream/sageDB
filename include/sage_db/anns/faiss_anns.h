#pragma once

#include "base_anns.h"
#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/IndexHNSW.h>
#include <faiss/AutoTune.h>
#include <faiss/index_io.h>
#include <memory>

namespace sage_db {
namespace anns {

/**
 * @brief FAISS-based ANNS implementation
 * 
 * Supports multiple FAISS index types:
 * - FLAT: Exact search using brute force
 * - IVF_FLAT: Inverted file with flat quantizer
 * - IVF_PQ: Inverted file with product quantizer
 * - HNSW: Hierarchical NSW (if available)
 */
class FaissANNS : public BaseANNS {
public:
    FaissANNS();
    ~FaissANNS() override;
    
    // BaseANNS interface implementation
    std::string algorithm_name() const override { return "FAISS"; }
    std::string algorithm_version() const override;
    
    std::vector<DistanceMetric> supported_metrics() const override;
    bool supports_metric(DistanceMetric metric) const override;
    
    void initialize(const DatabaseConfig& config) override;
    
    ANNSMetrics fit(const std::vector<Vector>& data, 
                   const ANNSBuildParams& params = {}) override;
    
    bool load_index(const std::string& filepath) override;
    bool save_index(const std::string& filepath) const override;
    bool is_trained() const override;
    
    std::pair<std::vector<QueryResult>, ANNSMetrics> 
    search(const Vector& query, const ANNSQueryParams& params = {}) override;
    
    std::pair<std::vector<std::vector<QueryResult>>, ANNSMetrics>
    batch_search(const std::vector<Vector>& queries, 
                const ANNSQueryParams& params = {}) override;
    
    std::pair<std::vector<QueryResult>, ANNSMetrics>
    range_search(const Vector& query, float radius, 
                const ANNSQueryParams& params = {}) override;
    
    std::vector<VectorId> add_vectors(const std::vector<Vector>& vectors) override;
    bool remove_vectors(const std::vector<VectorId>& ids) override;
    
    std::unordered_map<std::string, double> get_index_stats() const override;
    size_t get_memory_usage() const override;
    std::unordered_map<std::string, std::string> get_algorithm_config() const override;
    
    bool validate_config(const DatabaseConfig& config, 
                        const ANNSBuildParams& params) const override;
    
private:
    // Index creation methods
    std::unique_ptr<faiss::Index> create_flat_index();
    std::unique_ptr<faiss::Index> create_ivf_flat_index(const ANNSBuildParams& params);
    std::unique_ptr<faiss::Index> create_ivf_pq_index(const ANNSBuildParams& params);
    std::unique_ptr<faiss::Index> create_hnsw_index(const ANNSBuildParams& params);
    std::unique_ptr<faiss::Index> create_auto_index(size_t num_vectors, 
                                                   const ANNSBuildParams& params);
    
    // Helper methods
    faiss::MetricType distance_metric_to_faiss(DistanceMetric metric) const;
    std::vector<QueryResult> faiss_results_to_query_results(
        const faiss::idx_t* ids, const float* distances, size_t k) const;
    
    // Configuration and state
    DatabaseConfig config_;
    std::unique_ptr<faiss::Index> index_;
    std::vector<VectorId> vector_ids_;  // Mapping from FAISS internal ID to VectorId
    VectorId next_vector_id_;
    
    // Metrics tracking
    mutable ANNSMetrics last_metrics_;
};

/**
 * @brief Factory for creating FAISS ANNS instances
 */
class FaissANNSFactory : public ANNSFactory {
public:
    std::unique_ptr<BaseANNS> create() const override;
    std::string algorithm_name() const override { return "FAISS"; }
    std::string algorithm_description() const override;
    ANNSBuildParams default_build_params() const override;
    ANNSQueryParams default_query_params() const override;
};

// Registration helper
void register_faiss_algorithm();

} // namespace anns
} // namespace sage_db