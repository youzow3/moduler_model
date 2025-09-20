#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-model-io.h"
#include "mm-model-options.h"

G_BEGIN_DECLS

/*
 * MMModel
 * Holds OrtSession and other essential data.
 */
typedef struct _MMModel MMModel;

struct _MMModel
{
  MMModelOptions *options;
  OrtSession *session;
  OrtAllocator *allocator;
  /* Array of MMValueInfo * for inputs */
  GPtrArray *input_infos;
  /* Array of MMValueInfo * for outputs */
  GPtrArray *output_infos;
};

MMModel *mm_model_new (MMModelOptions *options, const char *file_path,
                       GError **error);
void mm_model_ref (MMModel *model);
void mm_model_unref (MMModel *model);
/* Run model. input and output should hold valid names and values. */
gboolean mm_model_run (MMModel *model, MMModelInput *input,
                       MMModelOutput *output, GError **error);

G_END_DECLS
