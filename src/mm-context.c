
#include "mm-context.h"

typedef struct _MMRealContext MMRealContext;
G_DEFINE_QUARK (mm - ort, mm_ort);

struct _MMRealContext
{
  const OrtApi *api;
  OrtEnv *env;
  OrtAllocator *allocator;
  gatomicrefcount ref_count;
};

MMContext *
mm_context_new (GError **error)
{
  MMRealContext *context;
  const OrtApiBase *base;
  const OrtApi *api;
  OrtEnv *env = NULL;
  OrtAllocator *allocator;
  OrtStatus *status;
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  context = g_new (MMRealContext, 1);

  base = OrtGetApiBase ();
  api = base->GetApi (ORT_API_VERSION);
  status = api->CreateEnv (ORT_LOGGING_LEVEL_FATAL, "ort", &env);
  if (status)
    goto on_ort_error;

  status = api->GetAllocatorWithDefaultOptions (&allocator);
  if (status)
    goto on_ort_error;

  context->api = api;
  context->env = env;
  context->allocator = allocator;
  g_atomic_ref_count_init (&context->ref_count);

  return (MMContext *)context;
on_ort_error:
  g_set_error (error, MM_ORT_ERROR, api->GetErrorCode (status), "%s",
               api->GetErrorMessage (status));
  api->ReleaseStatus (status);
  if (env)
    api->ReleaseEnv (env);
  g_free (context);
  return NULL;
}

void
mm_context_ref (MMContext *context)
{
  MMRealContext *rcontext = (MMRealContext *)context;
  g_return_if_fail (rcontext);
  g_atomic_ref_count_inc (&rcontext->ref_count);
}

void
mm_context_unref (MMContext *context)
{
  MMRealContext *rcontext = (MMRealContext *)context;
  g_return_if_fail (rcontext);
  if (!g_atomic_ref_count_dec (&rcontext->ref_count))
    return;
  rcontext->api->ReleaseEnv (rcontext->env);
  g_free (context);
}

void
mm_context_set_error (MMContext *context, GError **err, OrtStatus *status)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (status != NULL);
  g_set_error (err, MM_ORT_ERROR, context->api->GetErrorCode (status), "%s",
               context->api->GetErrorMessage (status));
  context->api->ReleaseStatus (status);
}
