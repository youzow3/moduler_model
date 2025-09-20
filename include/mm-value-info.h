#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

#include "mm-context.h"

G_BEGIN_DECLS

/*
 * MMValueInfo
 * Holds basic tensor information. See below.
 */
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
/* Copies MMValueInfo with ref count = 1. (NOT ref) */
MMValueInfo *mm_value_info_copy (MMValueInfo *value_info);
void mm_value_info_ref (MMValueInfo *value_info);
void mm_value_info_unref (MMValueInfo *value_info);
/* Compare shapes. For dynamic shapes, this function checks dim_name. */
gboolean mm_value_info_match_shape (MMValueInfo *value_info,
                                    MMValueInfo *other);
/*
 * Set dimension, so MMValue can hold valid OrtValue.
 * hash_table should be (char *, int64_t)
 * returns TRUE if dim is changed, and FALSE if not
 */
gboolean mm_value_info_set_dimension (MMValueInfo *value_info,
                                      GHashTable *hash_table);
/* Returns element count. If the shape is not concrete, returns 0. */
size_t mm_value_info_get_element_count (MMValueInfo *value_info);
/* Returns data size. If the shape is not concrete, returns 0. */
size_t mm_value_info_get_data_size (MMValueInfo *value_info);

G_END_DECLS
