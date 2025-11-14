{
  "targets": [
    {
      "target_name": "binding", # internal .node filename

      "dependencies": ["deps/binding.gyp:libopus"],

      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "cflags": [
        "-pthread",
        "-fno-strict-aliasing",
        "-Wall",
        "-Wno-unused-parameter",
        "-Wno-missing-field-initializers",
        "-Wextra",
        "-pipe",
        "-fno-ident",
        "-fdata-sections",
        "-ffunction-sections",
        "-fPIC"
      ],

      "defines": [
        "LARGEFILE_SOURCE",
        "_FILE_OFFSET_BITS=64",
        "WEBRTC_TARGET_PC",
        "WEBRTC_LINUX",
        "WEBRTC_THREAD_RR",
        "EXPAT_RELATIVE_PATH",
        "GTEST_RELATIVE_PATH",
        "JSONCPP_RELATIVE_PATH",
        "WEBRTC_RELATIVE_PATH",
        "POSIX",
        "__STDC_FORMAT_MACROS",
        "DYNAMIC_ANNOTATIONS_ENABLED=0",
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        # this is what prebuildify/node-gyp use to set the Node-API version
        "NAPI_VERSION=<(napi_build_version)"
      ],

      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],

      "sources": [
        "src/node-opus.cc"
      ]
    }
  ]
}
