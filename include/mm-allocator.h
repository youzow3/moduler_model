#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-context.h"
#include "mm-model.h"

typedef struct _MMAllocator MMAllocator;

struct _MMAllocator
{
  OrtAllocator *allocator;
};

MMAllocator *mm_allocator_new (MMContext *context, MMModel *model,
                               const char *name, OrtAllocatorType type, int id,
                               OrtMemType mem_type, GError **error);
void mm_allocator_ref (MMAllocator *allocator);
void mm_allocator_unref (MMAllocator *allocator);
