#include "mm-value-info.h"

typedef struct _MMRealValueInfo MMRealValueInfo;

struct _MMRealValueInfo
{
  ONNXTensorElementDataType dtype;
  size_t ndim;
  int64_t *dim;
  char **dim_name;
  char *name;
  gatomicrefcount ref_count;
};

MMValueInfo *
mm_value_info_new (MMContext *context,
                   const OrtTensorTypeAndShapeInfo *tensor_info,
                   const char *name, GError **error)
{
  MMRealValueInfo *value_info;
  OrtStatus *status;
  const char **dim_name = NULL;

  g_return_val_if_fail (context, NULL);
  g_return_val_if_fail (tensor_info, NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  value_info = g_new0 (MMRealValueInfo, 1);

  status
      = context->api->GetTensorElementType (tensor_info, &value_info->dtype);
  if (status)
    goto on_ort_error;

  status = context->api->GetDimensionsCount (tensor_info, &value_info->ndim);
  if (status)
    goto on_ort_error;

  value_info->dim = g_new (int64_t, value_info->ndim);
  status = context->api->GetDimensions (tensor_info, value_info->dim,
                                        value_info->ndim);
  if (status)
    goto on_ort_error;

  /* GLib's strv is NULL-terminated, so allocate ndim + 1 */
  dim_name = g_new0 (const char *, value_info->ndim + 1);
  status = context->api->GetSymbolicDimensions (tensor_info, dim_name,
                                                value_info->ndim);
  if (status)
    goto on_ort_error;

  value_info->dim_name = g_strdupv ((gchar **)dim_name);
  value_info->name = g_strdup (name);
  g_atomic_ref_count_init (&value_info->ref_count);

  return (MMValueInfo *)value_info;
on_ort_error:
  mm_context_set_error (context, error, status);
  g_free (dim_name);

  g_free (value_info->name);
  g_strfreev (value_info->dim_name);
  g_free (value_info->dim);
  g_free (value_info);
  return NULL;
}

MMValueInfo *
mm_value_info_copy (MMValueInfo *value_info)
{
  MMRealValueInfo *rvalue_info = (MMRealValueInfo *)value_info;
  MMRealValueInfo *copy;
  g_return_val_if_fail (value_info, NULL);

  copy = g_new (MMRealValueInfo, 1);
  copy->dtype = rvalue_info->dtype;
  copy->ndim = rvalue_info->ndim;
  copy->dim
      = g_memdup2 (rvalue_info->dim, sizeof (int64_t) * rvalue_info->ndim);
  copy->dim_name = g_strdupv (rvalue_info->dim_name);
  copy->name = g_strdup (rvalue_info->name);
  g_atomic_ref_count_init (&copy->ref_count);
  return (MMValueInfo *)copy;
}

void
mm_value_info_ref (MMValueInfo *value_info)
{
  MMRealValueInfo *rvalue_info = (MMRealValueInfo *)value_info;
  g_return_if_fail (rvalue_info);
  g_atomic_ref_count_inc (&rvalue_info->ref_count);
}

void
mm_value_info_unref (MMValueInfo *value_info)
{
  MMRealValueInfo *rvalue_info = (MMRealValueInfo *)value_info;
  g_return_if_fail (rvalue_info);
  if (!g_atomic_ref_count_dec (&rvalue_info->ref_count))
    return;
  g_free (rvalue_info->name);
  g_strfreev (rvalue_info->dim_name);
  g_free (rvalue_info->dim);
  g_free (rvalue_info);
}

gboolean
mm_value_info_match_shape (MMValueInfo *value_info, MMValueInfo *other)
{
  MMRealValueInfo *rvalue_info = (MMRealValueInfo *)value_info;
  MMRealValueInfo *rother = (MMRealValueInfo *)other;
  g_return_val_if_fail (rvalue_info, FALSE);
  g_return_val_if_fail (rother, FALSE);

  if (rvalue_info->ndim != rother->ndim)
    return FALSE;
  if (memcmp (rvalue_info->dim, rother->dim,
              rvalue_info->ndim * sizeof (int64_t)))
    return FALSE;
  for (size_t k = 0; k < rvalue_info->ndim; k++)
    {
      // TODO check dim_name always non-NULL or not.
      // If non-NULL, value_info->dim_name[k] should be
      // (strlen(value_info->dim_name[k]) != 0)
      if (rvalue_info->dim_name[k]
          && g_strcmp0 (rvalue_info->dim_name[k], rother->dim_name[k]))
        return FALSE;
      else if (rvalue_info->dim[k] != rother->dim[k])
        return FALSE;
    }
  return TRUE;
}

void
mm_value_info_set_dimension (MMValueInfo *value_info, GHashTable *hash_table)
{
  MMRealValueInfo *rvalue_info = (MMRealValueInfo *)value_info;
  g_return_if_fail (rvalue_info);
  g_return_if_fail (hash_table);

  for (size_t k = 0; k < rvalue_info->ndim; k++)
    {
      char *dim_name = rvalue_info->dim_name[k];
      int64_t *data;

      /* TODO check dim_name is always non-NULL or not. */
      if (dim_name == NULL)
        continue;

      data = g_hash_table_lookup (hash_table, dim_name);
      if (data == NULL)
        continue;
      rvalue_info->dim[k] = *data;
    }
}

size_t
mm_value_info_get_element_count (MMValueInfo *value_info)
{
  MMRealValueInfo *rvalue_info = (MMRealValueInfo *)value_info;
  int64_t element;
  g_return_val_if_fail (value_info, 0);

  element = 1;
  for (size_t k = 0; k < rvalue_info->ndim; k++)
    {
      g_return_val_if_fail (rvalue_info->dim[k] > 0, 0);
      element *= rvalue_info->dim[k];
    }
  return element;
}

size_t
mm_value_info_get_data_size (MMValueInfo *value_info)
{
  MMRealValueInfo *rvalue_info = (MMRealValueInfo *)value_info;
  int64_t element;
  g_return_val_if_fail (rvalue_info, 0);

  element = mm_value_info_get_element_count (value_info);
  g_return_val_if_fail (element > 0, 0);

  switch (rvalue_info->dtype)
    {
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT8E4M3FN:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT8E4M3FNUZ:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT8E5M2:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT8E5M2FNUZ:
      return element;
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_BFLOAT16:
      return 2 * element;
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32:
      return 4 * element;
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_COMPLEX64:
      return 8 * element;
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT4:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT4:
      return (element + 1) / 2;
    default:
      g_return_val_if_reached (0);
    }
}
