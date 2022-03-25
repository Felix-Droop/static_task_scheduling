#pragma once

#include <algorithm>
#include <iostream>
#include <functional>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <cluster/cluster_node.hpp>

namespace cluster {

class cluster {
public:
    using const_iterator = std::vector<cluster_node>::const_iterator;

private:
    std::vector<cluster_node> const nodes{};

public:
    cluster(std::vector<cluster_node> const nodes_) 
    : nodes(std::move(nodes_)) 
    {}

    double mean_node_performance() const {
        double const performance_sum = std::transform_reduce(
            begin(), 
            end(),
            0.0,
            std::plus<>(),
            [] (auto const & n) {
                return n.performance();
            }
        );

        return performance_sum / size();
    }

    double best_node_performance() const {
        // safe dereference because cluster size enforced to be > 0
        return std::max_element(
            begin(),
            end(),
            [] (auto const & n0, auto const & n1) {
                return n0.performance() < n1.performance();
            }
        )->performance();
    }

    double mean_node_bandwidth() const {
        double const performance_sum = std::transform_reduce(
            begin(), 
            end(),
            0.0,
            std::plus<>(),
            [] (auto const & n) {
                return n.network_bandwidth;
            }
        );

        return performance_sum / size();
    }

    std::string to_string() const {
        std::stringstream out;

        out << "########## Cluster: ##########\n";
        for(cluster_node const & node : nodes) {
            out << node.to_string() << '\n';
        }
        out << '\n';

        return out.str();
    }

    size_t size() const {
        return nodes.size();
    }

    const_iterator begin() const {
        return nodes.begin();
    }

    const_iterator end() const {
        return nodes.end();
    }
};

} // namespace cluster
