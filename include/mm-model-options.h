#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-context.h"

typedef struct _MMModelOptions MMModelOptions;

struct _MMModelOptions
{
  MMContext *context;
  OrtSessionOptions *session_options;
  OrtRunOptions *run_options;
};

MMModelOptions *mm_model_options_new (MMContext *context, GError **error);
void mm_model_options_ref (MMModelOptions *model_options);
void mm_model_options_unref (MMModelOptions *model_options);
