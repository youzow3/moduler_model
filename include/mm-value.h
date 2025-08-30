#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-allocator.h"
#include "mm-context.h"
#include "mm-value-info.h"

typedef struct _MMValue MMValue;

struct _MMValue
{
  MMValueInfo *info;
  /* Allocator for value */
  MMAllocator *allocator;
  /* Can be NULL because of symbolic dimension mechanism in ONNXRuntime. */
  OrtValue *value;
  /* name as an input, can be NULL */
  const gchar *input_name;
  /* name as an output, can be NULL */
  const gchar *output_name;
};

MMValue *mm_value_new (MMContext *context, MMValueInfo *info, MMModel *model,
                       const char *input_name, const char *output_name,
                       MMValue *swap, GError **error);
void mm_value_ref (MMValue *value);
void mm_value_unref (MMValue *value);
gboolean mm_value_set_dimension (MMValue *value, GHashTable *hash_table,
                                 GError **error);
gboolean mm_value_set_data (MMValue *value, gpointer data, GError **error);
gpointer mm_value_get_data (MMValue *value, GError **error);
/* Update value->info according to value->value */
gboolean mm_value_update_info (MMValue *value, GError **error);
/* Update value->value according to value->info */
gboolean mm_value_update (MMValue *value, GError **error);
void mm_value_swap (MMValue *value);
