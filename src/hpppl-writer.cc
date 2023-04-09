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

#include "wabt/hpppl-writer.h"

#include <cctype>
#include <cinttypes>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <string_view>
#include <vector>

#include "wabt/cast.h"
#include "wabt/common.h"
#include "wabt/ir.h"
#include "wabt/literal.h"
#include "wabt/sha256.h"
#include "wabt/stream.h"
#include "wabt/string-util.h"

#define INDENT_SIZE 2

#define UNIMPLEMENTED(x) printf("unimplemented: %s\n", (x)), abort()

// code to be inserted into the generated output
extern const char* s_header_top;
extern const char* s_header_bottom;
extern const char* s_source_includes;
extern const char* s_source_declarations;

namespace wabt {

namespace {

class HPPPLWriter {
 public:
  HPPPLWriter(Stream* hpppl_stream,
          const WriteHPPPLOptions& options)
      : options_(options),
        hpppl_stream_(hpppl_stream) {
  }

  Result WriteModule(const Module&);

 private:
  const WriteHPPPLOptions& options_;
  const Module* module_ = nullptr;
  Stream* hpppl_stream_ = nullptr;
  Result result_ = Result::Ok;
};

Result HPPPLWriter::WriteModule(const Module& module) {
  WABT_USE(options_);
  module_ = &module;
  return result_;
}

}  // end anonymous namespace

Result WriteHPPPL(Stream* hpppl_stream,
              const Module* module,
              const WriteHPPPLOptions& options) {
  HPPPLWriter hpppl_writer(hpppl_stream, options);
  return hpppl_writer.WriteModule(*module);
}

}  // namespace wabt
