
#include "mm-context.h"

typedef struct _MMRealContext MMRealContext;
G_DEFINE_QUARK (mm-ort, mm_ort);

struct _MMRealContext
{
  const OrtApi *api;
  OrtEnv *env;
  OrtAllocator *allocator;
  char **execution_providers;
  int execution_providers_length;
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

  status = api->GetAvailableProviders (&context->execution_providers,
                                       &context->execution_providers_length);
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
  g_assert (
      rcontext->api->ReleaseAvailableProviders (
          rcontext->execution_providers, rcontext->execution_providers_length)
      == NULL);
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

GStrv
mm_context_get_available_execution_provider (MMContext *context)
{
  MMRealContext *rcontext = (MMRealContext *)context;
  GStrvBuilder *builder;
  g_return_val_if_fail (rcontext, NULL);
  builder = g_strv_builder_new ();
  for (int k = 0; k < rcontext->execution_providers_length; k++)
    g_strv_builder_add (builder, rcontext->execution_providers[k]);
  return g_strv_builder_unref_to_strv (builder);
}
