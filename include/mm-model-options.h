#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-context.h"
#include "mm-provider.h"

G_BEGIN_DECLS

/*
 * MMModelOptions
 * Currently, this just wraps OrtSessionOptions and RunOptions.
 * Thus, you should call context->api->* to interact with options.
 */
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
gboolean mm_model_options_append_provider (MMModelOptions *model_options,
                                           MMProvider *provider,
                                           GError **error);

G_END_DECLS
