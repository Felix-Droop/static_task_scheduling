#pragma once

#include <iostream>
#include <optional>

#include <io/command_line_arguments.hpp>

#include <clipp.h>

namespace io {

std::optional<command_line_arguments> parse_command_line(int argc, char *argv[]) {
    using namespace clipp;
    command_line_arguments args{};

    auto cluster_option = required("-c", "--cluster") & value("cluster_file", args.cluster_input);
    auto task_bags_option = required("-t", "--tasks") & value("tasks_file", args.task_bag_input);
    auto dependency_option = required("-d", "--dependencies") & value("dependencies_file", args.dependency_input);

    auto output_option = option("-o", "--output") & value("output_file", args.output);
    auto verbose_option = option("-v", "--verbose")
        .set(args.verbose);

    std::string const cluster_doc = (
        "File in .csv format that describes the cluster architecture. "
        "It should contain exactly the fields bandwidth, performance, memory and num_cores."
    );
    std::string const task_bags_doc = (
        "File in .csv format that describes the tasks of the workflow. "
        "It should contain exactly the fields workload, input_data_size, output_data_size, "
        "memory and cardinality. "
    );
    std::string const dependency_doc = (
        "File that contains the dependencies for the workflow tasks. " 
        "Can either be in csv format or in xml format. "
        "A csv file should contain exactly the fields from_id and to_id. "
        "An xml file should model the schema at https://pegasus.isi.edu/schema/dax-2.1.xsd."
    );
    std::string const output_doc = (
        "If given, the verbose output of this program is written to this file as plain text."
    );
    std::string const verbosity_doc = (
        "If given, all metrics and the full solution are printed to the command line."
    );

    auto cli = (
        "Input" % (
            cluster_option % cluster_doc,
            task_bags_option % task_bags_doc,
            dependency_option % dependency_doc
        ),
        "Output" % (
            output_option % output_doc,
            verbose_option % verbosity_doc
        )
    );

    auto res = parse(argc, argv, cli);

    if(res.any_error()) {
        std::cout << make_man_page(cli, "static_task_scheduling");
        return std::nullopt;
    }

    return args;
}

} // namespace io
