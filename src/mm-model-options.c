
#include "mm-model-options.h"

typedef struct _MMRealModelOptions MMRealModelOptions;

struct _MMRealModelOptions
{
  MMContext *context;
  OrtSessionOptions *session_options;
  OrtRunOptions *run_options;
  gatomicrefcount ref_count;
};

MMModelOptions *
mm_model_options_new (MMContext *context, GError **error)
{
  MMRealModelOptions *model_options;
  OrtSessionOptions *session_options = NULL;
  OrtRunOptions *run_options = NULL;
  OrtStatus *status;

  g_return_val_if_fail (context, NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  model_options = g_new (MMRealModelOptions, 1);

  status = context->api->CreateSessionOptions (&session_options);
  if (status)
    goto on_ort_error;

  status = context->api->CreateRunOptions (&run_options);
  if (status)
    goto on_ort_error;

  mm_context_ref (context);

  model_options->context = context;
  model_options->session_options = session_options;
  model_options->run_options = run_options;
  g_atomic_ref_count_init (&model_options->ref_count);

  return (MMModelOptions *)model_options;
on_ort_error:
  mm_context_set_error (context, error, status);
  if (run_options)
    context->api->ReleaseRunOptions (run_options);
  if (session_options)
    context->api->ReleaseSessionOptions (session_options);
  g_free (model_options);
  return NULL;
}

void
mm_model_options_ref (MMModelOptions *model_options)
{
  MMRealModelOptions *rmodel_options = (MMRealModelOptions *)model_options;
  g_return_if_fail (rmodel_options != NULL);
  g_atomic_ref_count_inc (&rmodel_options->ref_count);
}
void
mm_model_options_unref (MMModelOptions *model_options)
{
  MMRealModelOptions *rmodel_options = (MMRealModelOptions *)model_options;
  g_return_if_fail (rmodel_options != NULL);

  if (!g_atomic_ref_count_dec (&rmodel_options->ref_count))
    return;
  rmodel_options->context->api->ReleaseRunOptions (model_options->run_options);
  rmodel_options->context->api->ReleaseSessionOptions (
      model_options->session_options);
  mm_context_unref (model_options->context);
  g_free (model_options);
}
