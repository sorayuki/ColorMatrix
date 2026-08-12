// Minimal classic AviSynth 2.6 C++ API3 header for the ColorMatrix x64 build.
//
// This header deliberately targets the ABI used by Hybrid's classic x64
// AviSynth runtime. It follows the classic interface-v6 AVS_Linkage table,
// with the x64 data layouts used by the historical AviSynth64 port. The
// AVS_Linkage Size field is an int, matching Hybrid's runtime and the
// maintained AviSynth C++ header's classic-v6 compatibility prefix.
//
// Only the public API surface needed to build ColorMatrix is included.
//
// Runtime validation recorded 2026-08-09:
// the source-built x64 ColorMatrix64.dll produced with this header
// (SHA-256 2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF)
// was confirmed by the reporter in Selur Hybrid 2026.03.21.1 on Windows 10
// Pro 22H2 x64. It completed the Rec.601-to-Rec.709 workflow without a crash
// and without the erroneous FCC-to-SMPTE-240M color shift. This validates the
// reported Hybrid workflow, not every possible classic AviSynth host.

#ifndef COLORMATRIX_AVISYNTH26_API3_X64_H
#define COLORMATRIX_AVISYNTH26_API3_X64_H

#ifndef AVISYNTH64
#define AVISYNTH64 1
#endif

enum { AVISYNTH_INTERFACE_VERSION = 6 };

#include <windef.h>
#include <stddef.h>

#pragma pack(push, 8)

#define FRAME_ALIGN 16

typedef float SFLOAT;

enum {
  SAMPLE_INT8  = 1 << 0,
  SAMPLE_INT16 = 1 << 1,
  SAMPLE_INT24 = 1 << 2,
  SAMPLE_INT32 = 1 << 3,
  SAMPLE_FLOAT = 1 << 4
};

enum {
  PLANAR_Y = 1 << 0,
  PLANAR_U = 1 << 1,
  PLANAR_V = 1 << 2,
  PLANAR_ALIGNED = 1 << 3,
  PLANAR_Y_ALIGNED = PLANAR_Y | PLANAR_ALIGNED,
  PLANAR_U_ALIGNED = PLANAR_U | PLANAR_ALIGNED,
  PLANAR_V_ALIGNED = PLANAR_V | PLANAR_ALIGNED,
  PLANAR_A = 1 << 4,
  PLANAR_R = 1 << 5,
  PLANAR_G = 1 << 6,
  PLANAR_B = 1 << 7,
  PLANAR_A_ALIGNED = PLANAR_A | PLANAR_ALIGNED,
  PLANAR_R_ALIGNED = PLANAR_R | PLANAR_ALIGNED,
  PLANAR_G_ALIGNED = PLANAR_G | PLANAR_ALIGNED,
  PLANAR_B_ALIGNED = PLANAR_B | PLANAR_ALIGNED
};

class AvisynthError {
public:
  const char* const msg;
  AvisynthError(const char* _msg) : msg(_msg) {}
};

struct __single_inheritance VideoInfo;
class __single_inheritance VideoFrameBuffer;
class __single_inheritance VideoFrame;
class IClip;
class __single_inheritance PClip;
class __single_inheritance PVideoFrame;
class IScriptEnvironment;
class __single_inheritance AVSValue;

/*
 * Classic AviSynth 2.6 C++ plugin linkage table.
 *
 * IMPORTANT: Hybrid's x64 runtime exposes Size as a 32-bit int. Natural
 * 8-byte alignment places the first member-function pointer at offset 8.
 * Do not change the field order.
 */
struct AVS_Linkage {
  int Size;

  // VideoInfo (42 entries)
  bool    (VideoInfo::*HasVideo)() const;
  bool    (VideoInfo::*HasAudio)() const;
  bool    (VideoInfo::*IsRGB)() const;
  bool    (VideoInfo::*IsRGB24)() const;
  bool    (VideoInfo::*IsRGB32)() const;
  bool    (VideoInfo::*IsYUV)() const;
  bool    (VideoInfo::*IsYUY2)() const;
  bool    (VideoInfo::*IsYV24)() const;
  bool    (VideoInfo::*IsYV16)() const;
  bool    (VideoInfo::*IsYV12)() const;
  bool    (VideoInfo::*IsYV411)() const;
  bool    (VideoInfo::*IsY8)() const;
  bool    (VideoInfo::*IsColorSpace)(int c_space) const;
  bool    (VideoInfo::*Is)(int property) const;
  bool    (VideoInfo::*IsPlanar)() const;
  bool    (VideoInfo::*IsFieldBased)() const;
  bool    (VideoInfo::*IsParityKnown)() const;
  bool    (VideoInfo::*IsBFF)() const;
  bool    (VideoInfo::*IsTFF)() const;
  bool    (VideoInfo::*IsVPlaneFirst)() const;
  int     (VideoInfo::*BytesFromPixels)(int pixels) const;
  int     (VideoInfo::*RowSize)(int plane) const;
  int     (VideoInfo::*BMPSize)() const;
  __int64 (VideoInfo::*AudioSamplesFromFrames)(int frames) const;
  int     (VideoInfo::*FramesFromAudioSamples)(__int64 samples) const;
  __int64 (VideoInfo::*AudioSamplesFromBytes)(__int64 bytes) const;
  __int64 (VideoInfo::*BytesFromAudioSamples)(__int64 samples) const;
  int     (VideoInfo::*AudioChannels)() const;
  int     (VideoInfo::*SampleType)() const;
  bool    (VideoInfo::*IsSampleType)(int testtype) const;
  int     (VideoInfo::*SamplesPerSecond)() const;
  int     (VideoInfo::*BytesPerAudioSample)() const;
  void    (VideoInfo::*SetFieldBased)(bool isfieldbased);
  void    (VideoInfo::*Set)(int property);
  void    (VideoInfo::*Clear)(int property);
  int     (VideoInfo::*GetPlaneWidthSubsampling)(int plane) const;
  int     (VideoInfo::*GetPlaneHeightSubsampling)(int plane) const;
  int     (VideoInfo::*BitsPerPixel)() const;
  int     (VideoInfo::*BytesPerChannelSample)() const;
  void    (VideoInfo::*SetFPS)(unsigned numerator, unsigned denominator);
  void    (VideoInfo::*MulDivFPS)(unsigned multiplier, unsigned divisor);
  bool    (VideoInfo::*IsSameColorspace)(const VideoInfo& vi) const;

  // VideoFrameBuffer (5 entries)
  const BYTE* (VideoFrameBuffer::*VFBGetReadPtr)() const;
  BYTE*       (VideoFrameBuffer::*VFBGetWritePtr)();
  size_t      (VideoFrameBuffer::*GetDataSize)() const;
  int         (VideoFrameBuffer::*GetSequenceNumber)() const;
  int         (VideoFrameBuffer::*GetRefcount)() const;

  // VideoFrame (9 entries)
  int               (VideoFrame::*GetPitch)(int plane) const;
  int               (VideoFrame::*GetRowSize)(int plane) const;
  int               (VideoFrame::*GetHeight)(int plane) const;
  VideoFrameBuffer* (VideoFrame::*GetFrameBuffer)() const;
  size_t            (VideoFrame::*GetOffset)(int plane) const;
  const BYTE*       (VideoFrame::*VFGetReadPtr)(int plane) const;
  bool              (VideoFrame::*IsWritable)() const;
  BYTE*             (VideoFrame::*VFGetWritePtr)(int plane) const;
  void              (VideoFrame::*VideoFrame_DESTRUCTOR)();

  // PClip (6 entries)
  void (PClip::*PClip_CONSTRUCTOR0)();
  void (PClip::*PClip_CONSTRUCTOR1)(const PClip& x);
  void (PClip::*PClip_CONSTRUCTOR2)(IClip* x);
  void (PClip::*PClip_OPERATOR_ASSIGN0)(IClip* x);
  void (PClip::*PClip_OPERATOR_ASSIGN1)(const PClip& x);
  void (PClip::*PClip_DESTRUCTOR)();

  // PVideoFrame (6 entries)
  void (PVideoFrame::*PVideoFrame_CONSTRUCTOR0)();
  void (PVideoFrame::*PVideoFrame_CONSTRUCTOR1)(const PVideoFrame& x);
  void (PVideoFrame::*PVideoFrame_CONSTRUCTOR2)(VideoFrame* x);
  void (PVideoFrame::*PVideoFrame_OPERATOR_ASSIGN0)(VideoFrame* x);
  void (PVideoFrame::*PVideoFrame_OPERATOR_ASSIGN1)(const PVideoFrame& x);
  void (PVideoFrame::*PVideoFrame_DESTRUCTOR)();

  // AVSValue (31 entries)
  void            (AVSValue::*AVSValue_CONSTRUCTOR0)();
  void            (AVSValue::*AVSValue_CONSTRUCTOR1)(IClip* c);
  void            (AVSValue::*AVSValue_CONSTRUCTOR2)(const PClip& c);
  void            (AVSValue::*AVSValue_CONSTRUCTOR3)(bool b);
  void            (AVSValue::*AVSValue_CONSTRUCTOR4)(int i);
  void            (AVSValue::*AVSValue_CONSTRUCTOR5)(float f);
  void            (AVSValue::*AVSValue_CONSTRUCTOR6)(double f);
  void            (AVSValue::*AVSValue_CONSTRUCTOR7)(const char* s);
  void            (AVSValue::*AVSValue_CONSTRUCTOR8)(const AVSValue* a, int size);
  void            (AVSValue::*AVSValue_CONSTRUCTOR9)(const AVSValue& v);
  void            (AVSValue::*AVSValue_DESTRUCTOR)();
  AVSValue&       (AVSValue::*AVSValue_OPERATOR_ASSIGN)(const AVSValue& v);
  const AVSValue& (AVSValue::*AVSValue_OPERATOR_INDEX)(int index) const;
  bool            (AVSValue::*Defined)() const;
  bool            (AVSValue::*IsClip)() const;
  bool            (AVSValue::*IsBool)() const;
  bool            (AVSValue::*IsInt)() const;
  bool            (AVSValue::*IsFloat)() const;
  bool            (AVSValue::*IsString)() const;
  bool            (AVSValue::*IsArray)() const;
  PClip           (AVSValue::*AsClip)() const;
  bool            (AVSValue::*AsBool1)() const;
  int             (AVSValue::*AsInt1)() const;
  const char*     (AVSValue::*AsString1)() const;
  double          (AVSValue::*AsFloat1)() const;
  bool            (AVSValue::*AsBool2)(bool def) const;
  int             (AVSValue::*AsInt2)(int def) const;
  double          (AVSValue::*AsDblDef)(double def) const;
  double          (AVSValue::*AsFloat2)(float def) const;
  const char*     (AVSValue::*AsString2)(const char* def) const;
  int             (AVSValue::*ArraySize)() const;
};

extern const AVS_Linkage* AVS_linkage;

#define AVS_HAS_ENTRY(entry) \
  (AVS_linkage != 0 && (size_t)AVS_linkage->Size > offsetof(AVS_Linkage, entry))

struct VideoInfo {
  int width, height;
  unsigned fps_numerator, fps_denominator;
  int num_frames;
  int pixel_type;
  int audio_samples_per_second;
  int sample_type;
  __int64 num_audio_samples;
  int nchannels;
  int image_type;

  enum {
    CS_BGR = 1 << 28,
    CS_YUV = 1 << 29,
    CS_INTERLEAVED = 1 << 30,
    CS_PLANAR = (int)(1U << 31),
    CS_UNKNOWN = 0,
    CS_BGR24 = (1 << 0) | CS_BGR | CS_INTERLEAVED,
    CS_BGR32 = (1 << 1) | CS_BGR | CS_INTERLEAVED,
    CS_YUY2 = (1 << 2) | CS_YUV | CS_INTERLEAVED,
    CS_YV12 = (1 << 3) | CS_YUV | CS_PLANAR,
    CS_I420 = (1 << 4) | CS_YUV | CS_PLANAR,
    CS_IYUV = CS_I420,
    IT_BFF = 1 << 0,
    IT_TFF = 1 << 1,
    IT_FIELDBASED = 1 << 2
  };

  bool IsYUY2() const {
    return AVS_HAS_ENTRY(IsYUY2) ? (this->*(AVS_linkage->IsYUY2))() : false;
  }
  bool IsYV12() const {
    return AVS_HAS_ENTRY(IsYV12) ? (this->*(AVS_linkage->IsYV12))() : false;
  }
  bool IsFieldBased() const {
    return AVS_HAS_ENTRY(IsFieldBased) ? (this->*(AVS_linkage->IsFieldBased))() : false;
  }
  void SetFieldBased(bool value) {
    if (AVS_HAS_ENTRY(SetFieldBased))
      (this->*(AVS_linkage->SetFieldBased))(value);
  }
};

class VideoFrameBuffer {
  BYTE* const data;
  const size_t data_size;
  volatile long sequence_number;
  volatile long refcount;

protected:
  VideoFrameBuffer(size_t);
  VideoFrameBuffer();
  ~VideoFrameBuffer();

public:
  const BYTE* GetReadPtr() const {
    return AVS_HAS_ENTRY(VFBGetReadPtr) ? (this->*(AVS_linkage->VFBGetReadPtr))() : 0;
  }
  BYTE* GetWritePtr() {
    return AVS_HAS_ENTRY(VFBGetWritePtr) ? (this->*(AVS_linkage->VFBGetWritePtr))() : 0;
  }
  size_t GetDataSize() const {
    return AVS_HAS_ENTRY(GetDataSize) ? (this->*(AVS_linkage->GetDataSize))() : 0;
  }
  int GetSequenceNumber() const {
    return AVS_HAS_ENTRY(GetSequenceNumber) ? (this->*(AVS_linkage->GetSequenceNumber))() : 0;
  }
  int GetRefcount() const {
    return AVS_HAS_ENTRY(GetRefcount) ? (this->*(AVS_linkage->GetRefcount))() : 0;
  }
};

class VideoFrame {
  volatile long refcount;
  VideoFrameBuffer* const vfb;
  const size_t offset;
  const int pitch, row_size, height;
  const size_t offsetU, offsetV;
  const int pitchUV, row_sizeUV, heightUV;

  friend class PVideoFrame;
  void AddRef();
  void Release();

  VideoFrame(VideoFrameBuffer*, size_t, int, int, int);
  VideoFrame(VideoFrameBuffer*, size_t, int, int, int, size_t, size_t, int, int, int);

public:
  int GetPitch(int plane = 0) const {
    return AVS_HAS_ENTRY(GetPitch) ? (this->*(AVS_linkage->GetPitch))(plane) : 0;
  }
  int GetRowSize(int plane = 0) const {
    return AVS_HAS_ENTRY(GetRowSize) ? (this->*(AVS_linkage->GetRowSize))(plane) : 0;
  }
  int GetHeight(int plane = 0) const {
    return AVS_HAS_ENTRY(GetHeight) ? (this->*(AVS_linkage->GetHeight))(plane) : 0;
  }
  VideoFrameBuffer* GetFrameBuffer() const {
    return AVS_HAS_ENTRY(GetFrameBuffer) ? (this->*(AVS_linkage->GetFrameBuffer))() : 0;
  }
  size_t GetOffset(int plane = 0) const {
    return AVS_HAS_ENTRY(GetOffset) ? (this->*(AVS_linkage->GetOffset))(plane) : 0;
  }
  const BYTE* GetReadPtr(int plane = 0) const {
    return AVS_HAS_ENTRY(VFGetReadPtr) ? (this->*(AVS_linkage->VFGetReadPtr))(plane) : 0;
  }
  bool IsWritable() const {
    return AVS_HAS_ENTRY(IsWritable) ? (this->*(AVS_linkage->IsWritable))() : false;
  }
  BYTE* GetWritePtr(int plane = 0) const {
    return AVS_HAS_ENTRY(VFGetWritePtr) ? (this->*(AVS_linkage->VFGetWritePtr))(plane) : 0;
  }
  ~VideoFrame() {
    if (AVS_HAS_ENTRY(VideoFrame_DESTRUCTOR))
      (this->*(AVS_linkage->VideoFrame_DESTRUCTOR))();
  }
};

// Old 2.5 cache-hint values retained for ColorMatrix's historical behavior.
enum {
  CACHE_NOTHING = 0,
  CACHE_RANGE = 1,
  CACHE_ALL = 2,
  CACHE_AUDIO = 3,
  CACHE_AUDIO_NONE = 4
};

class IClip {
  friend class PClip;
  friend class AVSValue;
  volatile long refcnt;

  void AddRef() { InterlockedIncrement((LONG*)&refcnt); }
  void Release() {
    if (InterlockedDecrement((LONG*)&refcnt) == 0)
      delete this;
  }

public:
  IClip() : refcnt(0) {}
  virtual int __stdcall GetVersion() { return AVISYNTH_INTERFACE_VERSION; }
  virtual PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) = 0;
  virtual bool __stdcall GetParity(int n) = 0;
  virtual void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) = 0;
  virtual int __stdcall SetCacheHints(int cachehints, int frame_range) = 0;
  virtual const VideoInfo& __stdcall GetVideoInfo() = 0;
  virtual __stdcall ~IClip() {}
};

class PClip {
  IClip* p;

public:
  PClip() : p(0) {
    if (AVS_HAS_ENTRY(PClip_CONSTRUCTOR0))
      (this->*(AVS_linkage->PClip_CONSTRUCTOR0))();
  }
  PClip(const PClip& x) : p(0) {
    if (AVS_HAS_ENTRY(PClip_CONSTRUCTOR1))
      (this->*(AVS_linkage->PClip_CONSTRUCTOR1))(x);
  }
  PClip(IClip* x) : p(0) {
    if (AVS_HAS_ENTRY(PClip_CONSTRUCTOR2))
      (this->*(AVS_linkage->PClip_CONSTRUCTOR2))(x);
  }
  void operator=(IClip* x) {
    if (AVS_HAS_ENTRY(PClip_OPERATOR_ASSIGN0))
      (this->*(AVS_linkage->PClip_OPERATOR_ASSIGN0))(x);
  }
  void operator=(const PClip& x) {
    if (AVS_HAS_ENTRY(PClip_OPERATOR_ASSIGN1))
      (this->*(AVS_linkage->PClip_OPERATOR_ASSIGN1))(x);
  }
  IClip* operator->() const { return p; }
  operator void*() const { return p; }
  bool operator!() const { return !p; }
  ~PClip() {
    if (AVS_HAS_ENTRY(PClip_DESTRUCTOR))
      (this->*(AVS_linkage->PClip_DESTRUCTOR))();
  }
};

class PVideoFrame {
  VideoFrame* p;

public:
  PVideoFrame() : p(0) {
    if (AVS_HAS_ENTRY(PVideoFrame_CONSTRUCTOR0))
      (this->*(AVS_linkage->PVideoFrame_CONSTRUCTOR0))();
  }
  PVideoFrame(const PVideoFrame& x) : p(0) {
    if (AVS_HAS_ENTRY(PVideoFrame_CONSTRUCTOR1))
      (this->*(AVS_linkage->PVideoFrame_CONSTRUCTOR1))(x);
  }
  PVideoFrame(VideoFrame* x) : p(0) {
    if (AVS_HAS_ENTRY(PVideoFrame_CONSTRUCTOR2))
      (this->*(AVS_linkage->PVideoFrame_CONSTRUCTOR2))(x);
  }
  void operator=(VideoFrame* x) {
    if (AVS_HAS_ENTRY(PVideoFrame_OPERATOR_ASSIGN0))
      (this->*(AVS_linkage->PVideoFrame_OPERATOR_ASSIGN0))(x);
  }
  void operator=(const PVideoFrame& x) {
    if (AVS_HAS_ENTRY(PVideoFrame_OPERATOR_ASSIGN1))
      (this->*(AVS_linkage->PVideoFrame_OPERATOR_ASSIGN1))(x);
  }
  VideoFrame* operator->() const { return p; }
  operator void*() const { return p; }
  bool operator!() const { return !p; }
  ~PVideoFrame() {
    if (AVS_HAS_ENTRY(PVideoFrame_DESTRUCTOR))
      (this->*(AVS_linkage->PVideoFrame_DESTRUCTOR))();
  }
};

class AVSValue {
public:
  AVSValue() : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR0))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR0))();
  }
  AVSValue(IClip* c) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR1))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR1))(c);
  }
  AVSValue(const PClip& c) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR2))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR2))(c);
  }
  AVSValue(bool b) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR3))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR3))(b);
  }
  AVSValue(int i) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR4))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR4))(i);
  }
  AVSValue(float f) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR5))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR5))(f);
  }
  AVSValue(double f) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR6))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR6))(f);
  }
  AVSValue(const char* s) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR7))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR7))(s);
  }
  AVSValue(const AVSValue* a, int size) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR8))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR8))(a, size);
  }
  AVSValue(const AVSValue& v) : integer(0), type('v'), array_size(0) {
    if (AVS_HAS_ENTRY(AVSValue_CONSTRUCTOR9))
      (this->*(AVS_linkage->AVSValue_CONSTRUCTOR9))(v);
  }
  ~AVSValue() {
    if (AVS_HAS_ENTRY(AVSValue_DESTRUCTOR))
      (this->*(AVS_linkage->AVSValue_DESTRUCTOR))();
  }
  AVSValue& operator=(const AVSValue& v) {
    return AVS_HAS_ENTRY(AVSValue_OPERATOR_ASSIGN)
      ? (this->*(AVS_linkage->AVSValue_OPERATOR_ASSIGN))(v)
      : *this;
  }
  const AVSValue& operator[](int index) const {
    return AVS_HAS_ENTRY(AVSValue_OPERATOR_INDEX)
      ? (this->*(AVS_linkage->AVSValue_OPERATOR_INDEX))(index)
      : *this;
  }
  bool Defined() const {
    return AVS_HAS_ENTRY(Defined) ? (this->*(AVS_linkage->Defined))() : false;
  }
  bool IsClip() const {
    return AVS_HAS_ENTRY(IsClip) ? (this->*(AVS_linkage->IsClip))() : false;
  }
  bool IsBool() const {
    return AVS_HAS_ENTRY(IsBool) ? (this->*(AVS_linkage->IsBool))() : false;
  }
  bool IsInt() const {
    return AVS_HAS_ENTRY(IsInt) ? (this->*(AVS_linkage->IsInt))() : false;
  }
  bool IsFloat() const {
    return AVS_HAS_ENTRY(IsFloat) ? (this->*(AVS_linkage->IsFloat))() : false;
  }
  bool IsString() const {
    return AVS_HAS_ENTRY(IsString) ? (this->*(AVS_linkage->IsString))() : false;
  }
  bool IsArray() const {
    return AVS_HAS_ENTRY(IsArray) ? (this->*(AVS_linkage->IsArray))() : false;
  }
  PClip AsClip() const {
    return AVS_HAS_ENTRY(AsClip) ? (this->*(AVS_linkage->AsClip))() : PClip();
  }
  bool AsBool() const {
    return AVS_HAS_ENTRY(AsBool1) ? (this->*(AVS_linkage->AsBool1))() : false;
  }
  int AsInt() const {
    return AVS_HAS_ENTRY(AsInt1) ? (this->*(AVS_linkage->AsInt1))() : 0;
  }
  const char* AsString() const {
    return AVS_HAS_ENTRY(AsString1) ? (this->*(AVS_linkage->AsString1))() : 0;
  }
  double AsFloat() const {
    return AVS_HAS_ENTRY(AsFloat1) ? (this->*(AVS_linkage->AsFloat1))() : 0.0;
  }
  bool AsBool(bool def) const {
    return AVS_HAS_ENTRY(AsBool2) ? (this->*(AVS_linkage->AsBool2))(def) : def;
  }
  int AsInt(int def) const {
    return AVS_HAS_ENTRY(AsInt2) ? (this->*(AVS_linkage->AsInt2))(def) : def;
  }
  double AsDblDef(double def) const {
    return AVS_HAS_ENTRY(AsDblDef) ? (this->*(AVS_linkage->AsDblDef))(def) : def;
  }
  double AsFloat(float def) const {
    return AVS_HAS_ENTRY(AsFloat2) ? (this->*(AVS_linkage->AsFloat2))(def) : def;
  }
  const char* AsString(const char* def) const {
    return AVS_HAS_ENTRY(AsString2) ? (this->*(AVS_linkage->AsString2))(def) : def;
  }
  int ArraySize() const {
    return AVS_HAS_ENTRY(ArraySize) ? (this->*(AVS_linkage->ArraySize))() : 0;
  }

private:
  union {
    IClip* clip;
    bool boolean;
    __int64 integer;
    double floating_pt;
    const char* string;
    const AVSValue* array;
  };
  short type;
  short array_size;
};

class GenericVideoFilter : public IClip {
protected:
  PClip child;
  VideoInfo vi;

public:
  GenericVideoFilter(PClip _child) : child(_child) { vi = child->GetVideoInfo(); }
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return child->GetFrame(n, env); }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    child->GetAudio(buf, start, count, env);
  }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return child->GetParity(n); }
  int __stdcall SetCacheHints(int, int) { return 0; }
};

enum {
  CPUF_FORCE       = 0x0001,
  CPUF_FPU         = 0x0002,
  CPUF_MMX         = 0x0004,
  CPUF_INTEGER_SSE = 0x0008,
  CPUF_SSE         = 0x0010,
  CPUF_SSE2        = 0x0020,
  CPUF_3DNOW       = 0x0040,
  CPUF_3DNOW_EXT   = 0x0080,
  CPUF_X86_64      = 0x00A0,
  CPUF_SSE3        = 0x0100,
  CPUF_SSSE3       = 0x0200,
  CPUF_SSE4        = 0x0400,
  CPUF_SSE4_1      = 0x0400,
  CPUF_SSE4_2      = 0x1000
};

class IScriptEnvironment {
public:
  virtual __stdcall ~IScriptEnvironment() {}
  virtual long __stdcall GetCPUFlags() = 0;
  virtual char* __stdcall SaveString(const char* s, int length = -1) = 0;
  virtual char* __stdcall Sprintf(const char* fmt, ...) = 0;
  virtual char* __stdcall VSprintf(const char* fmt, void* val) = 0;
  __declspec(noreturn) virtual void __stdcall ThrowError(const char* fmt, ...) = 0;

  class NotFound {};
  typedef AVSValue (__cdecl *ApplyFunc)(AVSValue args, void* user_data, IScriptEnvironment* env);

  virtual void __stdcall AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data) = 0;
  virtual bool __stdcall FunctionExists(const char* name) = 0;
  virtual AVSValue __stdcall Invoke(const char* name, const AVSValue args, const char* const* arg_names = 0) = 0;
  virtual AVSValue __stdcall GetVar(const char* name) = 0;
  virtual bool __stdcall SetVar(const char* name, const AVSValue& val) = 0;
  virtual bool __stdcall SetGlobalVar(const char* name, const AVSValue& val) = 0;
  virtual void __stdcall PushContext(int level = 0) = 0;
  virtual void __stdcall PopContext() = 0;
  virtual PVideoFrame __stdcall NewVideoFrame(const VideoInfo& vi, int align = FRAME_ALIGN) = 0;
  virtual bool __stdcall MakeWritable(PVideoFrame* pvf) = 0;
  virtual void __stdcall BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch,
                                int row_size, int height) = 0;

  typedef void (__cdecl *ShutdownFunc)(void* user_data, IScriptEnvironment* env);
  virtual void __stdcall AtExit(ShutdownFunc function, void* user_data) = 0;
  virtual void __stdcall CheckVersion(int version = AVISYNTH_INTERFACE_VERSION) = 0;
  virtual PVideoFrame __stdcall Subframe(PVideoFrame src, int rel_offset, int new_pitch,
                                         int new_row_size, int new_height) = 0;
  virtual int __stdcall SetMemoryMax(int mem) = 0;
  virtual int __stdcall SetWorkingDir(const char* newdir) = 0;
  virtual void* __stdcall ManageCache(int key, void* data) = 0;

  enum PlanarChromaAlignmentMode {
    PlanarChromaAlignmentOff,
    PlanarChromaAlignmentOn,
    PlanarChromaAlignmentTest
  };

  virtual bool __stdcall PlanarChromaAlignment(PlanarChromaAlignmentMode key) = 0;
  virtual PVideoFrame __stdcall SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch,
                                                int new_row_size, int new_height, int rel_offsetU,
                                                int rel_offsetV, int new_pitchUV) = 0;
  virtual void __stdcall DeleteScriptEnvironment() = 0;
  virtual void __stdcall ApplyMessage(PVideoFrame* frame, const VideoInfo& vi, const char* message,
                                      int size, int textcolor, int halocolor, int bgcolor) = 0;
  virtual const AVS_Linkage* const __stdcall GetAVSLinkage() = 0;
  virtual AVSValue __stdcall GetVarDef(const char* name, const AVSValue& def = AVSValue()) = 0;
};

// ABI checks derived from the classic x64 API3 layout used by Hybrid.
static_assert(sizeof(bool (VideoInfo::*)() const) == 8, "unexpected x64 member-function pointer ABI");
static_assert(offsetof(AVS_Linkage, HasVideo) == 0x08, "AVS_Linkage first entry must begin at 0x08");
static_assert(offsetof(AVS_Linkage, PClip_OPERATOR_ASSIGN1) == 0x1E8, "PClip linkage offset mismatch");
static_assert(offsetof(AVS_Linkage, PVideoFrame_DESTRUCTOR) == 0x220, "PVideoFrame linkage offset mismatch");
static_assert(offsetof(AVS_Linkage, AVSValue_DESTRUCTOR) == 0x278, "AVSValue linkage offset mismatch");
static_assert(sizeof(VideoInfo) == 48, "VideoInfo x64 ABI mismatch");
static_assert(sizeof(VideoFrameBuffer) == 24, "VideoFrameBuffer x64 ABI mismatch");
static_assert(sizeof(VideoFrame) == 72, "VideoFrame x64 ABI mismatch");
static_assert(sizeof(IClip) == 16, "IClip x64 ABI mismatch");
static_assert(sizeof(PClip) == 8, "PClip x64 ABI mismatch");
static_assert(sizeof(PVideoFrame) == 8, "PVideoFrame x64 ABI mismatch");
static_assert(sizeof(AVSValue) == 16, "AVSValue x64 ABI mismatch");

#undef AVS_HAS_ENTRY
#pragma pack(pop)

#endif // COLORMATRIX_AVISYNTH26_API3_X64_H
