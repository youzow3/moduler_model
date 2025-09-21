
#include "mm-model-options.h"

typedef struct _MMRealModelOptions MMRealModelOptions;

struct _MMRealModelOptions
{
  MMContext *context;
  OrtSessionOptions *session_options;
  OrtRunOptions *run_options;
  GPtrArray *providers;
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

  status = context->api->SetSessionGraphOptimizationLevel(session_options, ORT_ENABLE_ALL);
  if (status)
    goto on_ort_error;

  status = context->api->CreateRunOptions (&run_options);
  if (status)
    goto on_ort_error;

  mm_context_ref (context);

  model_options->context = context;
  model_options->session_options = session_options;
  model_options->run_options = run_options;
  model_options->providers = g_ptr_array_new_with_free_func((GDestroyNotify)mm_provider_unref);
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
  g_ptr_array_unref (rmodel_options->providers);
  rmodel_options->context->api->ReleaseRunOptions (model_options->run_options);
  rmodel_options->context->api->ReleaseSessionOptions (
      model_options->session_options);
  mm_context_unref (model_options->context);
  g_free (model_options);
}

gboolean
mm_model_options_append_provider (MMModelOptions *model_options,
                                  MMProvider *provider, GError **error)
{
  MMRealModelOptions *rmodel_options = (MMRealModelOptions *)model_options;
  MMContext *context;
  OrtStatus *status;
  g_return_val_if_fail (rmodel_options, FALSE);
  g_return_val_if_fail (provider, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);

  context = model_options->context;
  switch (provider->name)
    {
    case MM_PROVIDER_TENSOR_RT:
      status
          = context->api->SessionOptionsAppendExecutionProvider_TensorRT_V2 (
              rmodel_options->session_options, provider->tensor_rt);
      break;
    case MM_PROVIDER_CUDA:
      status = context->api->SessionOptionsAppendExecutionProvider_CUDA_V2 (
          rmodel_options->session_options, provider->cuda);
      break;
    case MM_PROVIDER_CANN:
      status = context->api->SessionOptionsAppendExecutionProvider_CANN (
          rmodel_options->session_options, provider->cann);
      break;
    case MM_PROVIDER_DNNL:
      status = context->api->SessionOptionsAppendExecutionProvider_Dnnl (
          rmodel_options->session_options, provider->dnnl);
      break;
    case MM_PROVIDER_ROCM:
      status = context->api->SessionOptionsAppendExecutionProvider_ROCM (
          rmodel_options->session_options, provider->rocm);
      break;
    default:
      g_warn_if_reached ();
      status = NULL;
      break;
    }

  if (status)
    goto on_error;

  mm_provider_ref (provider);
  g_ptr_array_add (rmodel_options->providers, provider);
  return TRUE;
on_error:
  mm_context_set_error (model_options->context, error, status);
  return FALSE;
}
