#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-context.h"

typedef struct _MMValueInfo MMValueInfo;

struct _MMValueInfo
{
  ONNXTensorElementDataType dtype;
  size_t ndim;
  int64_t *dim;
  /* string array, and element can be NULL */
  char **dim_name;
  char *name;
};

MMValueInfo *mm_value_info_new (MMContext *context,
                                const OrtTensorTypeAndShapeInfo *tensor_info,
                                const char *name, GError **error);
MMValueInfo *mm_value_info_copy (MMValueInfo *value_info);
void mm_value_info_ref (MMValueInfo *value_info);
void mm_value_info_unref (MMValueInfo *value_info);
gboolean mm_value_info_match_shape (MMValueInfo *value_info,
                                    MMValueInfo *other);
/* hash_table should be (char *, int64_t) */
void mm_value_info_set_dimension (MMValueInfo *value_info,
                                  GHashTable *hash_table);
