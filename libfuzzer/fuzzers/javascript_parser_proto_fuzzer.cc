// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "javascript_parser.pb.h"  // from out/gen
#include "testing/libfuzzer/fuzzers/javascript_parser_proto_to_string.h"
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"

#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

// Silence logging from the protobuf library.
protobuf_mutator::protobuf::LogSilencer log_silencer;

v8::Isolate* isolate = nullptr;

std::string protobuf_to_string(
    const javascript_parser_proto_fuzzer::Source& source_protobuf) {
  std::string source;
  for (const auto& token : source_protobuf.tokens()) {
    source += token_to_string(token) + std::string(" ");
  }
  return source;
}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  v8::V8::InitializeICUDefaultLocation((*argv)[0]);
  v8::V8::InitializeExternalStartupData((*argv)[0]);

  v8::Platform* platform = v8::platform::CreateDefaultPlatform();
  v8::V8::InitializePlatform(platform);
  v8::V8::Initialize();

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate = v8::Isolate::New(create_params);
  return 0;
}

DEFINE_BINARY_PROTO_FUZZER(
    const javascript_parser_proto_fuzzer::Source& source_protobuf) {
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  std::string source_string = protobuf_to_string(source_protobuf);
  v8::Local<v8::String> source =
      v8::String::NewFromUtf8(isolate, source_string.c_str(),
                              v8::NewStringType::kNormal)
          .ToLocalChecked();

  {
    v8::TryCatch try_catch(isolate);

    v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source);
    // TODO(marja): Figure out a more elegant way to silence the warning.
    script.IsEmpty();

    // TODO(crbug.com/775796): run the code once we find a way to avoid endless
    // loops.
  }
}