#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

G_BEGIN_DECLS

/*
 * MMContext
 * Base context structure for Moduler Model.
 * All members should not be freed.
 */
typedef struct _MMContext MMContext;
#define MM_ORT_ERROR mm_ort_quark ()

struct _MMContext
{
  const OrtApi *api;
  OrtEnv *env;
  /* default allocator */
  OrtAllocator *allocator;
};

MMContext *mm_context_new (GError **error);
void mm_context_ref (MMContext *context);
void mm_context_unref (MMContext *context);
/*
 * Create GError from OrtStatus*. It's like g_set_error.
 * status will be freed, so you don't have to call
 * context->api->ReleaseStatus().
 */
void mm_context_set_error (MMContext *context, GError **err,
                           OrtStatus *status);
/*
 * Returns available execution providers.
 * These strings can be used to initialize MMProvider by using
 * mm_provider_name_from_str()
 * Returned value should be freed with g_strfreev().
 */
GStrv mm_context_get_available_execution_provider (MMContext *context);

G_END_DECLS
