#include "tdl.h"
#include "tinymath.h"
#include "palette.hpp"

#define kDisplayWidth	320
#define kDisplayHeight 200
#define kDisplayBpp	32

#define kMaxKernelSize 17
#define kFilterChangeSinusoidFactor 5.0f

volatile bool loop = true;

int EscHandler() {
	loop = false;
	return 0;
}

int main() {
	int error = 0;
	add_key(&EscHandler, Key_ESC);

	SLI *background = LoadGFX("back.pcx", 32);
	if (background == NULL) {
		error = -1;
		goto error_exit;
	}

	SLI *foreground_image = LoadGFX("front.pcx", 32);
	if (foreground_image == NULL) {
		error = -2;
		goto error_exit;
	}

	SLI *foreground_alpha = LoadGFX("front_a.pcx", 8);
	if (foreground_alpha == NULL) {
		error = -3;
		goto error_exit;
	}
  ExpandAlphaMaskRange(foreground_alpha);

  SLI *filtered_foreground_alpha = CreateSLI(foreground_alpha->SLIXSize, foreground_alpha->SLIYSize, foreground_alpha->SLIColorBits * 8, /*nframes=*/1);
  if (filtered_foreground_alpha == NULL) {
    error = -4;
    goto error_exit;
  }
  SetPalette(filtered_foreground_alpha, foreground_alpha->SLIPalette);
  Blit(filtered_foreground_alpha, foreground_alpha);

  SLI *filtered_foreground_image = CreateSLI(foreground_image->SLIXSize, foreground_image->SLIYSize, foreground_image->SLIColorBits * 8, /*nframes=*/1);
  if (filtered_foreground_image == NULL) {
    error = -5;
    goto error_exit;
  }
  SetBlitType(filtered_foreground_image, SLIBlitAlphaMap);
  SetAlpha(filtered_foreground_image, GetCurrentFramePtr(filtered_foreground_alpha));

  SLI *filtered_background = CreateSLI(background->SLIXSize, background->SLIYSize, background->SLIColorBits * 8, /*nframes=*/1);
  if (filtered_background == NULL) {
    error = -6;
    goto error_exit;
  }

	SLI *backbuffer = CreateSLI(background->SLIXSize, background->SLIYSize, kDisplayBpp, /*nframes=*/1);
	if (backbuffer == NULL) {
		error = -6;
		goto error_exit;
	}
  SetClip(backbuffer, kMaxKernelSize / 2, kMaxKernelSize / 2, backbuffer->SLIXSize - kMaxKernelSize / 2, backbuffer->SLIYSize - kMaxKernelSize / 2);
  SetPos(backbuffer, 0, (kDisplayHeight - background->SLIYSize) / 2);

	error = CreateVideoSLI(kDisplayWidth, kDisplayHeight, kDisplayBpp);
	if (error) {
		error = -7;
		goto error_exit;
	}

  float t = 0, t_inc = 0.01;
  StartFrameRate();
	while(loop) {
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
  StopFrameRate();
	
error_exit:
	DestroyVideoSLI(3);	
	if (error) {
		printf("Error code: 0x%dhn\n", error);
	}

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
	if (backbuffer != NULL) {
		DestroySLI(backbuffer);
	}

  ShowFrameRateResult();
	return error;
}