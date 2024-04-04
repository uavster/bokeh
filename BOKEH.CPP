#include "tdl.h"
#include "tinymath.h"
#include "palette.hpp"
#include "app.hpp"
#include "debug.h"
#include "except.hpp"
#include "debug.h"

#define kDisplayWidth	320
#define kDisplayHeight 200
#define kDisplayBpp	32

#define kMaxKernelSize 17
#define kFilterChangeSinusoidFactor 5.0f

class BokehApp : public Application {
public:
  virtual void Init() {
    background = NULL;
    foreground_image = NULL;
    foreground_alpha = NULL;
    filtered_foreground_alpha = NULL;
    filtered_foreground_image = NULL;
    filtered_background = NULL;
    backbuffer = NULL;

    background = LoadGFX("back.pcx", 32);
    if (background == NULL) {
      throw Exception("Can't load back.pcx");
    }

    foreground_image = LoadGFX("front.pcx", 32);
    if (foreground_image == NULL) {
      throw Exception("Can't load front.pcx");
    }

    foreground_alpha = LoadGFX("front_a.pcx", 8);
    if (foreground_alpha == NULL) {
      throw Exception("Can't load front_a.pcx");
    }
    ExpandAlphaMaskRange(foreground_alpha);

    filtered_foreground_alpha = CreateSLI(foreground_alpha->SLIXSize, foreground_alpha->SLIYSize, foreground_alpha->SLIColorBits * 8, /*nframes=*/1);
    if (filtered_foreground_alpha == NULL) {
      throw Exception("Can't create filtered_foreground_alpha");
    }
    SetPalette(filtered_foreground_alpha, foreground_alpha->SLIPalette);
    Blit(filtered_foreground_alpha, foreground_alpha);

    filtered_foreground_image = CreateSLI(foreground_image->SLIXSize, foreground_image->SLIYSize, foreground_image->SLIColorBits * 8, /*nframes=*/1);
    if (filtered_foreground_image == NULL) {
      throw Exception("Can't create filtered_foreground_image");
    }
    SetBlitType(filtered_foreground_image, SLIBlitAlphaMap);
    SetAlpha(filtered_foreground_image, GetCurrentFramePtr(filtered_foreground_alpha));

    filtered_background = CreateSLI(background->SLIXSize, background->SLIYSize, background->SLIColorBits * 8, /*nframes=*/1);
    if (filtered_background == NULL) {
      throw Exception("Can't create filtered_background");
    }

    backbuffer = CreateSLI(background->SLIXSize, background->SLIYSize, kDisplayBpp, /*nframes=*/1);
    if (backbuffer == NULL) {
      throw Exception("Can't create backbuffer");
    }
    SetClip(backbuffer, kMaxKernelSize / 2, kMaxKernelSize / 2, backbuffer->SLIXSize - kMaxKernelSize / 2, backbuffer->SLIYSize - kMaxKernelSize / 2);
    SetPos(backbuffer, 0, (kDisplayHeight - background->SLIYSize) / 2);

    if (CreateVideoSLI(kDisplayWidth, kDisplayHeight, kDisplayBpp)) {
      throw Exception("Can't create video SLI");
    }

    t = 0;
    t_inc = 0.01;
    StartFrameRate();
  }

  virtual void Update() {
    float sinusoid = kFilterChangeSinusoidFactor * cos(2.0f * 3.14159f * 0.1f * t);
    if (sinusoid > 1.0f) {
      sinusoid = 1.0f;
    }
    if (sinusoid < -1.0f) {
      sinusoid = -1.0f;
    }
    float smooth_square_signal = 0.5f * (sinusoid + 1.0f);
    float fg_kernel_size = (kMaxKernelSize - 1.0f) * smooth_square_signal + 1.0f;
    int fg_kernel_size_int = static_cast<int>(fg_kernel_size);
    if ((fg_kernel_size_int & 1) == 0) {
      ++fg_kernel_size_int;
    }
    int bg_kernel_size_int = kMaxKernelSize - fg_kernel_size_int + 1;
    if ((bg_kernel_size_int & 1) == 0) {
      --bg_kernel_size_int;
    }
    BoxFilter(background, bg_kernel_size_int, bg_kernel_size_int, filtered_background);
    BoxFilter(foreground_image, fg_kernel_size_int, fg_kernel_size_int, filtered_foreground_image);
    BoxFilter(foreground_alpha, fg_kernel_size_int, fg_kernel_size_int, filtered_foreground_alpha);
    Blit(backbuffer, filtered_background);
    float amplitude_modulator = 0.5f * (sin(2.0f * 3.14159f * 0.1f * t) + 1);
    SetPos(filtered_foreground_image, amplitude_modulator * 0.5f * (cos(2.0f * 3.14159f * 4.0f * t) + 1), amplitude_modulator * 0.5f * (cos(2.0f * 3.14159f * 4.0f * t) + 1));
    Blit(backbuffer, filtered_foreground_image);
    Blit(GetVideoSLI(), backbuffer);

    ShowVideoSLI();

    GetElapsedTime();
    IncFloat(&t, &t_inc);

    IncFrameCounter();
  }
  
  virtual void CleanUp() {
    StopFrameRate();

    if (background != NULL) {
      DestroySLI(background);
    }
    if (foreground_image != NULL) {
      DestroySLI(foreground_image);
    }
    if (foreground_alpha != NULL) {
      DestroySLI(foreground_alpha);
    }
    if (filtered_foreground_image != NULL) {
      DestroySLI(filtered_foreground_image);
    }
    if (filtered_foreground_alpha != NULL) {
      DestroySLI(filtered_foreground_alpha);
    }
    if (filtered_background != NULL) {
      DestroySLI(filtered_background);
    }
    if (backbuffer != NULL) {
      DestroySLI(backbuffer);
    }

    ShowFrameRateResult();
  }

private:
  SLI *background;
  SLI *foreground_image;
  SLI *foreground_alpha;
  SLI *filtered_foreground_alpha;
  SLI *filtered_foreground_image;
  SLI *filtered_background;
  SLI *backbuffer;
  float t;
  float t_inc;
};

int main() {
  BokehApp app;
  return app.Run();
}
