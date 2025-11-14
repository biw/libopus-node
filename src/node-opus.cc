// opus_encoder.cpp – N‑API C++ wrapper around libopus
// Rewritten with proper error propagation, memory management and safety checks.
// Build this as part of the node‑gyp addon (binding name: opus).

#include <napi.h>
#include <cstring>
#include "../deps/opus/include/opus.h"

using namespace Napi;

// -----------------------------------------------------------------------------
// Constants (keep in sync with your JavaScript layer)
// -----------------------------------------------------------------------------
static constexpr int MAX_FRAME_SIZE = 5760;  // 120 ms @ 48 kHz mono
static constexpr int MAX_PACKET_SIZE = 1276; // per Opus spec

// -----------------------------------------------------------------------------
// Utility: translate libopus error codes to strings
// -----------------------------------------------------------------------------
static const char *StrError(int code)
{
  switch (code)
  {
  case OPUS_OK:
    return "OK";
  case OPUS_BAD_ARG:
    return "One or more invalid/out‑of‑range arguments";
  case OPUS_BUFFER_TOO_SMALL:
    return "Buffer too small";
  case OPUS_INTERNAL_ERROR:
    return "Internal libopus error";
  case OPUS_INVALID_PACKET:
    return "Corrupted compressed data";
  case OPUS_UNIMPLEMENTED:
    return "Invalid/unsupported request";
  case OPUS_INVALID_STATE:
    return "Encoder/decoder in invalid state";
  case OPUS_ALLOC_FAIL:
    return "Memory allocation failed";
  default:
    return "Unknown libopus error";
  }
}

// -----------------------------------------------------------------------------
// OpusEncoder class – JS visible
// -----------------------------------------------------------------------------
class OpusEncoderWrap : public ObjectWrap<OpusEncoderWrap>
{
public:
  static Object Init(Napi::Env env, Object exports);
  OpusEncoderWrap(const CallbackInfo &);
  ~OpusEncoderWrap();

private:
  // JS‑exposed methods
  Value Encode(const CallbackInfo &);
  Value Decode(const CallbackInfo &);
  Value ApplyEncoderCTL(const CallbackInfo &);
  Value ApplyDecoderCTL(const CallbackInfo &);
  Value SetBitrate(const CallbackInfo &);
  Value GetBitrate(const CallbackInfo &);

  // Helpers
  int EnsureEncoder();
  int EnsureDecoder();

  // Members
  opus_int32 rate_{0};
  int channels_{0};
  int application_{OPUS_APPLICATION_AUDIO};
  OpusEncoder *enc_{nullptr};
  OpusDecoder *dec_{nullptr};
  opus_int16 *outPcm_{nullptr};     // MAX_FRAME_SIZE * channels_
  unsigned char *outOpus_{nullptr}; // MAX_PACKET_SIZE
};

// -----------------------------------------------------------------------------
// Constructor / destructor
// -----------------------------------------------------------------------------
OpusEncoderWrap::OpusEncoderWrap(const CallbackInfo &info) : ObjectWrap<OpusEncoderWrap>(info)
{
  if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    throw TypeError::New(info.Env(), "Expected (rate: number, channels: number)");

  rate_ = info[0].ToNumber().Int32Value();
  channels_ = info[1].ToNumber().Int32Value();

  outPcm_ = new opus_int16[channels_ * MAX_FRAME_SIZE]();
  outOpus_ = new unsigned char[MAX_PACKET_SIZE]();
}

OpusEncoderWrap::~OpusEncoderWrap()
{
  if (enc_)
    opus_encoder_destroy(enc_);
  if (dec_)
    opus_decoder_destroy(dec_);
  delete[] outPcm_;
  delete[] outOpus_;
}

// -----------------------------------------------------------------------------
// Lazy init helpers
// -----------------------------------------------------------------------------
int OpusEncoderWrap::EnsureEncoder()
{
  if (enc_)
    return OPUS_OK;
  int err;
  OpusEncoder *tmp = opus_encoder_create(rate_, channels_, application_, &err);
  if (err == OPUS_OK)
    enc_ = tmp; // keep null on failure
  return err;
}

int OpusEncoderWrap::EnsureDecoder()
{
  if (dec_)
    return OPUS_OK;
  int err;
  OpusDecoder *tmp = opus_decoder_create(rate_, channels_, &err);
  if (err == OPUS_OK)
    dec_ = tmp;
  return err;
}

// -----------------------------------------------------------------------------
// Encode PCM -> Opus packet (returns Buffer)
// -----------------------------------------------------------------------------
Value OpusEncoderWrap::Encode(const CallbackInfo &info)
{
  Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsBuffer())
    throw TypeError::New(env, "Argument must be a Buffer containing 16‑bit PCM");

  if (EnsureEncoder() != OPUS_OK)
    throw Error::New(env, "Failed to create libopus encoder (bad params?)");

  Buffer<char> buf = info[0].As<Buffer<char>>();
  if (buf.Length() % (2 * channels_) != 0)
    throw RangeError::New(env, "PCM buffer length must be multiple of (channels*2 bytes)");

  const opus_int16 *pcm = reinterpret_cast<const opus_int16 *>(buf.Data());
  int frameSize = buf.Length() / 2 / channels_;
  if (frameSize > MAX_FRAME_SIZE)
    throw RangeError::New(env, "Frame exceeds MAX_FRAME_SIZE");

  int clen = opus_encode(enc_, pcm, frameSize, outOpus_, MAX_PACKET_SIZE);
  if (clen < 0)
    throw Error::New(env, StrError(clen));

  return Buffer<char>::Copy(env, reinterpret_cast<char *>(outOpus_), clen);
}

// -----------------------------------------------------------------------------
// Decode Opus packet -> PCM buffer
// -----------------------------------------------------------------------------
Value OpusEncoderWrap::Decode(const CallbackInfo &info)
{
  Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsBuffer())
    throw TypeError::New(env, "Argument must be a Buffer");

  if (EnsureDecoder() != OPUS_OK)
    throw Error::New(env, "Failed to create libopus decoder");

  Buffer<unsigned char> buf = info[0].As<Buffer<unsigned char>>();
  int dlen = opus_decode(dec_, buf.Data(), buf.Length(), outPcm_, MAX_FRAME_SIZE, 0);
  if (dlen < 0)
    throw Error::New(env, StrError(dlen));

  size_t bytes = static_cast<size_t>(dlen) * channels_ * sizeof(opus_int16);
  return Buffer<char>::Copy(env, reinterpret_cast<char *>(outPcm_), bytes);
}

// -----------------------------------------------------------------------------
// CTL helpers (encoder / decoder generic)
// -----------------------------------------------------------------------------
Value OpusEncoderWrap::ApplyEncoderCTL(const CallbackInfo &info)
{
  Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    throw TypeError::New(env, "Expected (ctl: number, value: number)");

  if (EnsureEncoder() != OPUS_OK)
    throw Error::New(env, "Encoder not initialised");

  int ctl = info[0].ToNumber().Int32Value();
  int value = info[1].ToNumber().Int32Value();
  int rc = opus_encoder_ctl(enc_, ctl, value);
  if (rc != OPUS_OK)
    throw Error::New(env, StrError(rc));
  return Number::New(env, rc);
}

Value OpusEncoderWrap::ApplyDecoderCTL(const CallbackInfo &info)
{
  Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    throw TypeError::New(env, "Expected (ctl: number, value: number)");

  if (EnsureDecoder() != OPUS_OK)
    throw Error::New(env, "Decoder not initialised");

  int ctl = info[0].ToNumber().Int32Value();
  int value = info[1].ToNumber().Int32Value();
  int rc = opus_decoder_ctl(dec_, ctl, value);
  if (rc != OPUS_OK)
    throw Error::New(env, StrError(rc));
  return Number::New(env, rc);
}

// -----------------------------------------------------------------------------
// Set / get bitrate with proper error propagation
// -----------------------------------------------------------------------------
Value OpusEncoderWrap::SetBitrate(const CallbackInfo &info)
{
  Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsNumber())
    throw TypeError::New(env, "Expected bitrate (number)");

  int bitrate = info[0].ToNumber().Int32Value();
  if (EnsureEncoder() != OPUS_OK)
    throw Error::New(env, "Encoder not initialised");

  int rc = opus_encoder_ctl(enc_, OPUS_SET_BITRATE(bitrate));
  if (rc != OPUS_OK)
    throw Error::New(env, StrError(rc));
  return Number::New(env, rc);
}

Value OpusEncoderWrap::GetBitrate(const CallbackInfo &info)
{
  Env env = info.Env();
  if (EnsureEncoder() != OPUS_OK)
    throw Error::New(env, "Encoder not initialised");

  opus_int32 br = 0;
  int rc = opus_encoder_ctl(enc_, OPUS_GET_BITRATE(&br));
  if (rc != OPUS_OK)
    throw Error::New(env, StrError(rc));
  return Number::New(env, br);
}

// -----------------------------------------------------------------------------
// JS class registration
// -----------------------------------------------------------------------------
Object OpusEncoderWrap::Init(Napi::Env env, Object exports)
{
  Function ctor = DefineClass(env, "OpusEncoder", {
                                                      InstanceMethod("encode", &OpusEncoderWrap::Encode),
                                                      InstanceMethod("decode", &OpusEncoderWrap::Decode),
                                                      InstanceMethod("applyEncoderCTL", &OpusEncoderWrap::ApplyEncoderCTL),
                                                      InstanceMethod("applyDecoderCTL", &OpusEncoderWrap::ApplyDecoderCTL),
                                                      InstanceMethod("setBitrate", &OpusEncoderWrap::SetBitrate),
                                                      InstanceMethod("getBitrate", &OpusEncoderWrap::GetBitrate),
                                                  });
  exports.Set("OpusEncoder", ctor);
  return exports;
}

// -----------------------------------------------------------------------------
// Addon entry point
// -----------------------------------------------------------------------------
Object InitAll(Napi::Env env, Object exports)
{
  return OpusEncoderWrap::Init(env, exports);
}

NODE_API_MODULE(opus, InitAll)
