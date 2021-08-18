#include "pdu/pdu.h"

#include "pdu/io.h"
#include "pdu/query.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <vector>

struct params_t {
    params_t(int argc, char* argv[]) {
        namespace po = boost::program_options;

        boost::program_options::options_description options("Allowed options");
        boost::program_options::positional_options_description pos_options;

        // clang-format off
        options.add_options()
            ("dir,d", po::value(&statsDir)->required(), "Prometheus stats directory")
            ("query,q", po::value(&query), "Prometheus query");

        pos_options.add("dir", 1);
        // clang-format on

        po::variables_map vm;
        try {
            auto parsed = po::command_line_parser(argc, argv)
                                  .options(options)
                                  .positional(pos_options)
                                  .run();
            po::store(parsed, vm);
            po::notify(vm);
            valid = true;
        } catch (const po::error& e) {
            fmt::print("{}\n", e.what());
            fmt::print(
                    "Usage:\n"
                    "    pdump <options> <dir>\n"
                    "e.g.,\n"
                    "    pdump -q '{foo=\"bar\"}' ./stats_data\n\n");
            fmt::print("{}\n", options);
            valid = false;
        }
    }
    std::string statsDir = "";
    std::string query = "";
    bool valid = false;
};

class SampleDumpVisitor : public OrderedSeriesVisitor {
public:
    using OrderedSeriesVisitor::visit;
    void visit(const Series& series) override {
        std::cout << series << "\n";
        last = 0;
    }
    void visit(const Sample& sample) override {
        std::cout << sample.timestamp << " " << sample.value << "\n";
        if (sample.timestamp < last) {
            throw std::runtime_error(
                    "SampleDumpVisitor encountered non-monotonic timestamps "
                    "(bug)");
        }
        last = sample.timestamp;
    }

    // track the most recently seen timestamp, used for sanity checking on
    // index visit ordering.

    int64_t last = 0;
};

int main(int argc, char* argv[]) {
    // parse command line args
    params_t params(argc, argv);
    if (!params.valid) {
        // if parsing failed, a usage message will already have been printed.
        // just exit now.
        return 1;
    }

    auto data = pdu::load(params.statsDir);

    SeriesFilter filter;
    //filter.addFilter("__name__", "sysproc_page_faults_raw");

    SampleDumpVisitor foo;
    foo.visit(data.filtered(filter));

    std::cout.flush();

    return 0;
}
