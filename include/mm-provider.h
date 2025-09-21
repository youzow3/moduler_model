#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-context.h"

G_BEGIN_DECLS

typedef enum _MMProviderName
{
  MM_PROVIDER_NULL,
  MM_PROVIDER_TENSOR_RT,
  MM_PROVIDER_CUDA,
  MM_PROVIDER_CANN,
  MM_PROVIDER_DNNL,
  MM_PROVIDER_ROCM,
  // MM_PROVIDER_NV_TENSOR_RT,
  // MM_PROVIDER_MI_GRAPH_X,
  // MM_PROVIDER_OPEN_VINO,
} MMProviderName;

MMProviderName mm_provider_name_from_str (const char *name);

/*
 * MMProvider
 * Wraps execution provider options structures.
 */
typedef struct _MMProvider MMProvider;

struct _MMProvider
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
  };
};

MMProvider *mm_provider_new (MMContext *context, MMProviderName name,
                             GError **error);
void mm_provider_ref (MMProvider *provider);
void mm_provider_unref (MMProvider *provider);

G_END_DECLS
