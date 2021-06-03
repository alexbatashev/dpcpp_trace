#include "options.hpp"

#include <string_view>
#include <iostream>


static void parseInfoOptions(int argc, const char *argv[]) {
  (void)argv;

  if (argc > 2) {
    std::cerr << "info does not accept arguments";
    std::terminate();
  }
}

void options::parseRecordOptions(int argc, char *argv[]) {
  int i = 2;
  bool hasExtraOpts = false;

  while (i < argc) {
    std::string_view opt{argv[i]};

    if (opt[0] != '-') {
      mInput = opt;
    } else if (opt == "--") {
      hasExtraOpts = true;
      i++;
      break;
    } else if (opt == "--output" || opt == "-o") {
      if (i + 1 >= argc) {
        std::cerr << "--output requires an argument\n";
        std::terminate();
      }
      mOutput = argv[++i];
    }

    i++;
  }

  if (hasExtraOpts) {
    for (int k = i; k < argc; k++) {
      mArguments.emplace_back(argv[k]);
    }
  }

  if (mInput.empty()) {
    std::cerr << "input is required\n";
    std::terminate();
  }
  if (mOutput.empty()) {
    std::cerr << "output is required\n";
    std::terminate();
  }
}

void options::parsePrintOptions(int argc, char *argv[]) {
  int i = 2;
  bool hasExtraOpts = false;

  while (i < argc) {
    std::string_view opt{argv[i]};

    if (opt[0] != '-') {
      mInput = opt;
    }

    i++;
  }

  if (mInput.empty()) {
    std::cerr << "input is required\n";
    std::terminate();
  }
}

options::options(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Use prp info to see available options";
    std::terminate();
  }

  std::string_view command(argv[1]);

  if (command == "record") {
    mMode = mode::record;
    parseRecordOptions(argc, argv);
  } else if (command == "replay") {
    mMode = mode::replay;
  } else if (command == "print") {
    mMode = mode::print;
    parsePrintOptions(argc, argv);
  } else if (command == "info") {
    mMode = mode::info;
  }
}
