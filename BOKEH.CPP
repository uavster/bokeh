#include "tdl.h"
#include "tinymath.h"
#include "palette.hpp"
#include "app.hpp"
#include "debug.h"
#include "except.hpp"
#include "debug.h"
#include "image.hpp"

#define kDisplayWidth	320
#define kDisplayHeight 200
#define kDisplayBpp	kImageBpp32

#define kMaxKernelSize 17
#define kFilterChangeSinusoidFactor 5.0f

class BokehApp : public Application {
public:
  virtual void Init() {
    background = Image("back.pcx", kImageBpp32);
    foreground_image = Image("front.pcx", kImageBpp32);
    foreground_alpha = Image("front_a.pcx", kImageBpp8);
    ExpandAlphaMaskRange(foreground_alpha.sli());
    filtered_foreground_alpha = Image(foreground_alpha.width(), foreground_alpha.height(), foreground_alpha.bpp());
    SetPalette(filtered_foreground_alpha.sli(), foreground_alpha.sli()->SLIPalette);
    Blit(filtered_foreground_alpha.sli(), foreground_alpha.sli());

    filtered_foreground_image = Image(foreground_image.width(), foreground_image.height(), foreground_image.bpp());
    SetBlitType(filtered_foreground_image.sli(), SLIBlitAlphaMap);
    SetAlpha(filtered_foreground_image.sli(), GetCurrentFramePtr(filtered_foreground_alpha.sli()));

    filtered_background = Image(background.width(), background.height(), background.bpp());

    backbuffer = Image(background.width(), background.height(), kDisplayBpp);
    SetClip(backbuffer.sli(), kMaxKernelSize / 2, kMaxKernelSize / 2, backbuffer.width() - kMaxKernelSize / 2, backbuffer.height() - kMaxKernelSize / 2);
    SetPos(backbuffer.sli(), 0, (kDisplayHeight - background.height()) / 2);

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
    BoxFilter(background.sli(), bg_kernel_size_int, bg_kernel_size_int, filtered_background.sli());
    BoxFilter(foreground_image.sli(), fg_kernel_size_int, fg_kernel_size_int, filtered_foreground_image.sli());
    BoxFilter(foreground_alpha.sli(), fg_kernel_size_int, fg_kernel_size_int, filtered_foreground_alpha.sli());
    Blit(backbuffer.sli(), filtered_background.sli());
    float amplitude_modulator = 0.5f * (sin(2.0f * 3.14159f * 0.1f * t) + 1);
    SetPos(filtered_foreground_image.sli(), amplitude_modulator * 0.5f * (cos(2.0f * 3.14159f * 4.0f * t) + 1), amplitude_modulator * 0.5f * (cos(2.0f * 3.14159f * 4.0f * t) + 1));
    Blit(backbuffer.sli(), filtered_foreground_image.sli());
    Blit(GetVideoSLI(), backbuffer.sli());

    ShowVideoSLI();

    GetElapsedTime();
    IncFloat(&t, &t_inc);

    IncFrameCounter();
  }
  
  virtual void CleanUp() {
    StopFrameRate();
    ShowFrameRateResult();
  }

private:
  Image background;
  Image foreground_image;
  Image foreground_alpha;
  Image filtered_foreground_alpha;
  Image filtered_foreground_image;
  Image filtered_background;
  Image backbuffer;
  float t;
  float t_inc;
};

int main() {
  BokehApp app;
  return app.Run();
}
