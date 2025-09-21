
#include "mm-provider.h"

// Should be compatible with OrtApi::GetAvaiableProviders(), but these strings
// could be wrong...
MMProviderName
mm_provider_name_from_str (const char *name)
{
  g_return_val_if_fail (name, MM_PROVIDER_NULL);

  if (!strcmp (name, "TensorRTExecutionProvider"))
    return MM_PROVIDER_TENSOR_RT;
  if (!strcmp (name, "CUDAExecutionProvider"))
    return MM_PROVIDER_CUDA;
  if (!strcmp (name, "CANNExecutionProvider"))
    return MM_PROVIDER_CANN;
  if (!strcmp (name, "DnnlExecutionProvider"))
    return MM_PROVIDER_DNNL;
  if (!strcmp (name, "ROCMExecutionProvider"))
    return MM_PROVIDER_ROCM;
  g_return_val_if_reached (MM_PROVIDER_NULL);
}

typedef struct _MMRealProvider MMRealProvider;

struct _MMRealProvider
{
  MMProviderName name;
  union
  {
    OrtTensorRTProviderOptionsV2 *tensor_rt;
    OrtNvTensorRtRtxProviderOptions *nv_tensor_rt;
    OrtCUDAProviderOptionsV2 *cuda;
    OrtCANNProviderOptions *cann;
    OrtDnnlProviderOptions *dnnl;
    OrtROCMProviderOptions *rocm;
    OrtMIGraphXProviderOptions *mi_graph_x;
    OrtOpenVINOProviderOptions *open_vino;
    void *ep_options;
  };

  MMContext *context;
  gatomicrefcount ref_count;
};

MMProvider *
mm_provider_new (MMContext *context, MMProviderName name, GError **error)
{
  MMRealProvider *provider;
  OrtStatus *status;
  GDestroyNotify release_func;
  g_return_val_if_fail (context, NULL);
  g_return_val_if_fail (name != MM_PROVIDER_NULL, NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  provider = g_new0 (MMRealProvider, 1);
  provider->name = name;
  switch (name)
    {
    case MM_PROVIDER_TENSOR_RT:
      status
          = context->api->CreateTensorRTProviderOptions (&provider->tensor_rt);
      release_func = (GDestroyNotify)context->api->ReleaseTensorRTProviderOptions;
      break;
    case MM_PROVIDER_CUDA:
      status = context->api->CreateCUDAProviderOptions (&provider->cuda);
      release_func = (GDestroyNotify)context->api->ReleaseCUDAProviderOptions;
      break;
    case MM_PROVIDER_CANN:
      status = context->api->CreateCANNProviderOptions (&provider->cann);
      release_func = (GDestroyNotify)context->api->ReleaseCANNProviderOptions;
      break;
    case MM_PROVIDER_DNNL:
      status = context->api->CreateDnnlProviderOptions (&provider->dnnl);
      release_func = (GDestroyNotify)context->api->ReleaseDnnlProviderOptions;
      break;
    case MM_PROVIDER_ROCM:
      status = context->api->CreateROCMProviderOptions (&provider->rocm);
      release_func = (GDestroyNotify)context->api->ReleaseROCMProviderOptions;
      break;
    default:
      g_warn_if_reached ();
      status = NULL;
      release_func = NULL;
      break;
    }

  if (status)
    goto on_error;

  mm_context_ref (context);
  provider->context = context;
  g_atomic_ref_count_init (&provider->ref_count);

  return (MMProvider *)provider;
on_error:
  mm_context_set_error (context, error, status);
  g_clear_pointer(&provider->ep_options, release_func);
  g_free(provider);
  return NULL;
}

void
mm_provider_ref (MMProvider *provider)
{
  MMRealProvider *rprovider = (MMRealProvider *)provider;
  g_return_if_fail (rprovider);
  g_atomic_ref_count_inc (&rprovider->ref_count);
}

void
mm_provider_unref (MMProvider *provider)
{
  MMRealProvider *rprovider = (MMRealProvider *)provider;
  MMContext *context;
  g_return_if_fail (rprovider);
  if (!g_atomic_ref_count_dec (&rprovider->ref_count))
    return;

  context = rprovider->context;
  switch (rprovider->name)
    {
    case MM_PROVIDER_TENSOR_RT:
      context->api->ReleaseTensorRTProviderOptions (rprovider->tensor_rt);
      break;
    case MM_PROVIDER_CUDA:
      context->api->ReleaseCUDAProviderOptions (rprovider->cuda);
      break;
    case MM_PROVIDER_CANN:
      context->api->ReleaseCANNProviderOptions (rprovider->cann);
      break;
    case MM_PROVIDER_DNNL:
      context->api->ReleaseDnnlProviderOptions (rprovider->dnnl);
      break;
    case MM_PROVIDER_ROCM:
      context->api->ReleaseROCMProviderOptions (rprovider->rocm);
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  mm_context_unref (context);
  g_free (rprovider);
}
