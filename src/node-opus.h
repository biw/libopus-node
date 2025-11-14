#pragma once

#include <napi.h>
#include "../deps/opus/include/opus.h"

class OpusEncoderWrap : public Napi::ObjectWrap<OpusEncoderWrap>
{
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  OpusEncoderWrap(const Napi::CallbackInfo &info);
  ~OpusEncoderWrap();

  // JS-exposed methods
  Napi::Value Encode(const Napi::CallbackInfo &info);
  Napi::Value Decode(const Napi::CallbackInfo &info);
  Napi::Value ApplyEncoderCTL(const Napi::CallbackInfo &info);
  Napi::Value ApplyDecoderCTL(const Napi::CallbackInfo &info);
  Napi::Value SetBitrate(const Napi::CallbackInfo &info);
  Napi::Value GetBitrate(const Napi::CallbackInfo &info);

private:
  // Helpers
  int EnsureEncoder();
  int EnsureDecoder();

  // Members (must match the .cpp)
  opus_int32 rate_{0};
  int channels_{0};
  int application_{OPUS_APPLICATION_AUDIO};

  ::OpusEncoder *enc_{nullptr};
  ::OpusDecoder *dec_{nullptr};

  opus_int16 *outPcm_{nullptr};     // channels_ * MAX_FRAME_SIZE
  unsigned char *outOpus_{nullptr}; // MAX_PACKET_SIZE
};
