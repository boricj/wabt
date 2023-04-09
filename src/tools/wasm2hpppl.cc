/*
 * Copyright 2017 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "wabt/apply-names.h"
#include "wabt/binary-reader-ir.h"
#include "wabt/binary-reader.h"
#include "wabt/error-formatter.h"
#include "wabt/feature.h"
#include "wabt/filenames.h"
#include "wabt/generate-names.h"
#include "wabt/ir.h"
#include "wabt/option-parser.h"
#include "wabt/result.h"
#include "wabt/stream.h"
#include "wabt/validator.h"
#include "wabt/wast-lexer.h"

#include "wabt/hpppl-writer.h"

using namespace wabt;

static int s_verbose;
static std::string s_infile;
static std::string s_outfile;
static Features s_features;
static WriteHPPPLOptions s_write_hpppl_options;
static bool s_read_debug_names = true;
static std::unique_ptr<FileStream> s_log_stream;

static const char s_description[] =
    R"(  Read a file in the WebAssembly binary format, and convert it to
  an HP-PPL program.

examples:
  # parse binary file test.wasm and write test.hpppl
  $ wasm2hpppl test.wasm -o test.hpppl
)";

static const std::string supported_features[] = {};

static bool IsFeatureSupported(const std::string& feature) {
//  return std::find(std::begin(supported_features), std::end(supported_features),
//                   feature) != std::end(supported_features);
  return false;
};

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm2hpppl", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStderr();
  });
  parser.AddOption(
      'o', "output", "FILENAME",
      "Output file for the generated HP-PPL source file, by default use stdout",
      [](const char* argument) {
        s_outfile = argument;
        ConvertBackslashToSlash(&s_outfile);
      });
  s_features.AddOptions(&parser);
  parser.AddOption("no-debug-names", "Ignore debug names in the binary file",
                   []() { s_read_debug_names = false; });
  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) {
                       s_infile = argument;
                       ConvertBackslashToSlash(&s_infile);
                     });
  parser.Parse(argc, argv);

  bool any_non_supported_feature = false;
#define WABT_FEATURE(variable, flag, default_, help)   \
  any_non_supported_feature |=                         \
      (s_features.variable##_enabled() != default_) && \
      !IsFeatureSupported(flag);
#include "wabt/feature.def"
#undef WABT_FEATURE

  if (any_non_supported_feature) {
    fprintf(stderr,
            "wasm2hpppl currently only supports a limited set of features.\n");
    exit(1);
  }
}

// TODO(binji): copied from binary-writer-spec.cc, probably should share.
static std::string_view strip_extension(std::string_view s) {
  std::string_view ext = s.substr(s.find_last_of('.'));
  std::string_view result = s;

  if (ext == ".c")
    result.remove_suffix(ext.length());
  return result;
}

Result Wasm2hppplMain(Errors& errors) {
  std::vector<uint8_t> file_data;
  CHECK_RESULT(ReadFile(s_infile.c_str(), &file_data));

  Module module;
  const bool kStopOnFirstError = true;
  const bool kFailOnCustomSectionError = true;
  ReadBinaryOptions options(s_features, s_log_stream.get(), s_read_debug_names,
                            kStopOnFirstError, kFailOnCustomSectionError);
  CHECK_RESULT(ReadBinaryIr(s_infile.c_str(), file_data.data(),
                            file_data.size(), options, &errors, &module));
  CHECK_RESULT(ValidateModule(&module, &errors, s_features));
  CHECK_RESULT(GenerateNames(&module));
  /* TODO(binji): This shouldn't fail; if a name can't be applied
   * (because the index is invalid, say) it should just be skipped. */
  ApplyNames(&module);

  if (!s_outfile.empty()) {
    std::string hpppl_name_full =
        std::string(strip_extension(s_outfile)) + ".hpppl";
    FileStream hpppl_stream(hpppl_name_full);
    CHECK_RESULT(WriteHPPPL(&hpppl_stream, &module,
                        s_write_hpppl_options));
  } else {
    FileStream stream(stdout);
    CHECK_RESULT(WriteHPPPL(&stream, &module,
                        s_write_hpppl_options));
  }

  return Result::Ok;
}

int ProgramMain(int argc, char** argv) {
  Result result;

  InitStdio();
  ParseOptions(argc, argv);

  Errors errors;
  result = Wasm2hppplMain(errors);
  FormatErrorsToFile(errors, Location::Type::Binary);

  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
