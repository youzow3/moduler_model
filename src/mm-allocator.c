
#include "mm-allocator.h"

typedef struct _MMRealAllocator MMRealAllocator;

struct _MMRealAllocator
{
  OrtAllocator *allocator;

  MMContext *context;
  MMModel *model;
  OrtMemoryInfo *info;
  gatomicrefcount ref_count;
};

MMAllocator *
mm_allocator_new (MMContext *context, MMModel *model, const char *name,
                  OrtAllocatorType type, int id, OrtMemType mem_type,
                  GError **error)
{
  MMRealAllocator *allocator = NULL;
  OrtStatus *status = NULL;

  g_return_val_if_fail (context, NULL);
  g_return_val_if_fail (model, NULL);
  g_return_val_if_fail (name, NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  allocator = g_new0 (MMRealAllocator, 1);
  status = context->api->CreateMemoryInfo (name, type, id, mem_type,
                                           &allocator->info);
  if (status)
    goto on_ort_error;

  status = context->api->CreateAllocator (model->session, allocator->info,
                                          &allocator->allocator);
  if (status)
    goto on_ort_error;

  mm_context_ref (context);
  mm_model_ref (model);

  allocator->context = context;
  allocator->model = model;
  g_atomic_ref_count_init (&allocator->ref_count);

  return (MMAllocator *)allocator;
on_ort_error:
  g_clear_pointer (&allocator->allocator, context->api->ReleaseAllocator);
  g_clear_pointer (&allocator->info, context->api->ReleaseMemoryInfo);
  g_free (allocator);
  mm_context_set_error (context, error, status);
  return NULL;
}

void
mm_allocator_ref (MMAllocator *allocator)
{
  MMRealAllocator *rallocator = (MMRealAllocator *)allocator;
  g_return_if_fail (rallocator);
  g_atomic_ref_count_inc (&rallocator->ref_count);
}

void
mm_allocator_unref (MMAllocator *allocator)
{
  MMRealAllocator *rallocator = (MMRealAllocator *)allocator;
  MMContext *context;
  g_return_if_fail (rallocator);
  if (!g_atomic_ref_count_dec (&rallocator->ref_count))
    return;

  context = rallocator->context;
  context->api->ReleaseAllocator (rallocator->allocator);
  context->api->ReleaseMemoryInfo (rallocator->info);
  mm_model_unref (rallocator->model);
  mm_context_unref (rallocator->context);
  g_free (rallocator);
}
