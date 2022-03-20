#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <schedule/time_interval.hpp>
#include <util/di_graph.hpp>
#include <workflow/dependency.hpp>
#include <workflow/task.hpp>

namespace workflow {

class workflow {
public:
    using iterator = di_graph<task, double>::vertex_iterator;

private:
    di_graph<task, double> g;

public:
    // create a DAG workflow represetation based on the input specifications
    // it is assumed that the ids in from_ids and to_ids refer to the indices of the other arguments
    workflow(
        std::vector<task> tasks,
        std::vector<double> input_data_sizes,
        std::vector<double> output_data_sizes,
        std::vector<dependency> dependencies
    ) {
        if (
            tasks.size() != input_data_sizes.size()
            || tasks.size() != output_data_sizes.size()
        ) {
            throw std::invalid_argument("Arguments for task parameters must have the same size.");
        }

        for (task const & t : tasks) {
            if (t.workload == 0) {
                throw std::invalid_argument("All tasks need a workload > 0");
            }

            g.add_vertex(t);
        }

        for (dependency const & dep : dependencies) {
            if (output_data_sizes.at(dep.from_id) != input_data_sizes.at(dep.to_id)) {
                throw std::invalid_argument("Input/Output data sizes for a dependency don't match.");
            }

            bool const was_created = g.add_edge(
                dep.from_id, 
                dep.to_id, 
                output_data_sizes.at(dep.from_id)
            );

            if (!was_created) {
                throw std::invalid_argument("Task ids for dependency endpoints are invalid.");
            }
        }
    }

    std::unordered_map<task_id, double> all_upward_ranks(
        double const mean_cluster_performance,
        double const mean_cluster_bandwidth
    ) const {
        std::unordered_map<task_id, double> upward_ranks{};
        
        auto topological_order = g.topological_order().value();
        for (task_id const t_id : std::views::reverse(topological_order)) {
            double const upward_rank = compute_upward_rank(
                upward_ranks, 
                mean_cluster_performance, 
                mean_cluster_bandwidth, 
                t_id
            );
            upward_ranks.insert({t_id, upward_rank});
        }

        return upward_ranks;
    }

    schedule::time_t get_sequential_makespan(double const best_cluster_node_performance) const {
        auto const & tasks = g.get_all_vertices();
        
        return std::transform_reduce(
            tasks.begin(),
            tasks.end(), 
            0.0,
            std::plus<>(),
            [best_cluster_node_performance] (auto const & t) {
                return t.workload / best_cluster_node_performance;
            }
        );
    }

    std::string to_string(std::optional<double> best_performance_opt = std::nullopt) const {
        std::stringstream out;

        out << "########## Workflow: ##########\n";

        out << "TODO\n";

        if (best_performance_opt) {
            out << "sequential makespan: " 
                << get_sequential_makespan(best_performance_opt.value())
                << "\n";
        }

        out << '\n';

        return out.str();
    }

    std::unordered_map<task_id, double> const & get_task_incoming_edges(task_id const t_id) const {
        return g.get_incoming_edges(t_id);
    }

    std::unordered_map<task_id, double> const & get_task_outgoing_edges(task_id const t_id) const {
        return g.get_outgoing_edges(t_id);
    }

    task const & get_task(task_id const t_id) const {
        return g.get_vertex(t_id);
    }

    iterator begin() const {
        return g.get_all_vertices().begin();
    }

    iterator end() const {
        return g.get_all_vertices().end();
    }

    // number of tasks in the workflow
    size_t size() const {
        return g.get_all_vertices().size();
    }

private:
    double compute_upward_rank(
        std::unordered_map<task_id, double> const & upward_ranks,
        double const mean_cluster_performance,
        double const mean_cluster_bandwidth,
        task_id const t_id
    ) const {
        double upward_rank = g.get_vertex(t_id).workload * mean_cluster_performance;

        auto outgoing_ranks = g.get_outgoing_edges(t_id) 
            | std::views::transform([&upward_ranks, mean_cluster_bandwidth] (auto const & edge) {
                auto const & [neighbor_id, data_transfer] = edge;
                return data_transfer / mean_cluster_bandwidth + upward_ranks.at(neighbor_id);
            }
        );

        auto const max_it = std::max_element(outgoing_ranks.begin(), outgoing_ranks.end());
        if (max_it != outgoing_ranks.end()) {
            upward_rank += *max_it;
        }

        return upward_rank;
    }
};

} // namespace workflow