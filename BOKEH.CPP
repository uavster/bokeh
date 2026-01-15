#include "tdl.h"
#include "tinymath.h"
#include "palette.hpp"
#include "app.hpp"
#include "debug.h"
#include "except.hpp"
#include "image.hpp"

#define kDisplayWidth	320
#define kDisplayHeight 200
#define kDisplayBpp	kImageBpp32

#define kMaxKernelSize 17
#define kFilterChangeSinusoidFactor 5.0f

class BokehApp : public VideoApplication {
public:
  BokehApp() : VideoApplication() {}
  BokehApp(const VideoAppOptions &options) : VideoApplication(options) {}

  virtual void InitBeforeVideo() {
    background = Image("back.pcx", kImageBpp32);    
    
    foreground_image = Image("front.pcx", kImageBpp32);

    foreground_alpha = Image("front_a.pcx", kImageBpp8);        
    ExpandAlphaMaskRange(foreground_alpha);

    filtered_foreground_alpha = Image(foreground_alpha.width(), foreground_alpha.height(), foreground_alpha.bpp());
    filtered_foreground_alpha.CopyPaletteFrom(foreground_alpha);
    Blit(filtered_foreground_alpha, foreground_alpha);

    filtered_foreground_image = Image(foreground_image.width(), foreground_image.height(), foreground_image.bpp());
    SetBlitType(filtered_foreground_image, SLIBlitAlphaMap);
    SetAlpha(filtered_foreground_image, GetCurrentFramePtr(filtered_foreground_alpha));

    filtered_background = Image(background.width(), background.height(), background.bpp());

    backbuffer = Image(background.width(), background.height(), kDisplayBpp);
    SetClip(backbuffer, kMaxKernelSize / 2, kMaxKernelSize / 2, backbuffer.width() - kMaxKernelSize / 2, backbuffer.height() - kMaxKernelSize / 2);
    SetPos(backbuffer, 0, (kDisplayHeight - background.height()) / 2);
  }

  virtual void UpdateFrame(float t) {
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
  }
  
  virtual void CleanUpAfterVideo() {
  }

private:
  Image background;
  Image foreground_image;
  Image foreground_alpha;
  Image filtered_foreground_alpha;
  Image filtered_foreground_image;
  Image filtered_background;
  Image backbuffer;
};

int main() {
  VideoAppOptions options;
  options.do_print_fps_at_end(true);
  options.SetVideoMode(kDisplayWidth, kDisplayHeight, kDisplayBpp);
  BokehApp app(options);
  return app.Run();
}
