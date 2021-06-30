#include "lwtnn/LightweightNeuralNetwork.hh"
#include "lwtnn/parse_json.hh"
#include "lwtnn/NanReplacer.hh"

#include "test_utilities.hh"

#include <Eigen/Dense>

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

namespace {
  int run_on_files(const lwt::JSONConfig& config,
                   const std::string& vars,
                   const std::string& vals);
  int run_on_generated(const lwt::JSONConfig& config);
}


void usage(const std::string& name) {
  std::cout << "usage: " << name << " <nn config> [<labels> <values>]\n"
    "\n"
    "The <nn config> file should be generated by one of the scripts in\n"
    "`converters/`.\n"
    "\n"
    "Both <labels> and <values> should be whitespace separated.\n"
    "In the case of values, there should be one pattern per line.\n"
    "If the labels and values are omitted generate dummy data.\n";
}

int main(int argc, char* argv[]) {
  if (argc > 4 || argc < 2) {
    usage(argv[0]);
    exit(1);
  }
  // Read in the configuration.
  std::string in_file_name(argv[1]);
  std::ifstream in_file(in_file_name);
  auto config = lwt::parse_json(in_file);

  if ( argc == 4) {
    run_on_files(config, argv[2], argv[3]);
  } else if (argc == 2) {
    run_on_generated(config);
  } else {
    usage(argv[0]);
    exit(1);
  }
  return 0;
}
namespace {
  int run_on_generated(const lwt::JSONConfig& config) {
    lwt::LightweightNeuralNetwork tagger(
      config.inputs, config.layers, config.outputs);
    std::map<std::string, double> in_vals;

    const std::size_t total_inputs = config.inputs.size();
    for (std::size_t nnn = 0; nnn < total_inputs; nnn++) {
      const auto& input = config.inputs.at(nnn);
      double ramp_val = ramp(input, nnn, total_inputs);
      in_vals[input.name] = ramp_val;
    }
    auto out_vals = tagger.compute(in_vals);
    for (const auto& out: out_vals) {
      std::cout << out.first << " " << out.second << std::endl;
    }
    return 0;
  }
  int run_on_files(const lwt::JSONConfig& config,
                   const std::string& vars,
                   const std::string& vals) {
    // initialize the tagger from the configuration
    lwt::LightweightNeuralNetwork tagger(
      config.inputs, config.layers, config.outputs);
    lwt::NanReplacer replacer(config.defaults, lwt::rep::all);

    // buffer
    std::string val_line;

    // read in input labels and values
    std::ifstream vars_stream( vars );
    std::getline(vars_stream, val_line);
    const auto labels = parse_line(val_line);

    std::ifstream values( vals );
    while (std::getline(values, val_line)) {
      auto val_strings = parse_line(val_line);
      if (val_strings.size() == 0) continue;
      assert(val_strings.size() == labels.size());
      std::map<std::string, double> nn_in;
      for (std::size_t iii = 0; iii < labels.size(); iii++) {
        nn_in[labels.at(iii)] = std::stof(val_strings.at(iii));
      }
      auto cleaned_inputs = replacer.replace(nn_in);
      auto out_vals = tagger.compute(cleaned_inputs);
      // look at the outputs
      for (const auto& out: out_vals) {
        std::cout << out.first << " " << out.second << std::endl;
      }
    }

    return 0;
  }

}
