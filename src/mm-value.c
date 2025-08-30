
#include "mm-value.h"

typedef struct _MMRealValue MMRealValue;

struct _MMRealValue
{
  MMValueInfo *info;
  MMAllocator *allocator;
  OrtValue *value;
  gchar *input_name;
  gchar *output_name;

  MMContext *context;
  MMValue *swap;
  gatomicrefcount ref_count;
};

MMValue *
mm_value_new (MMContext *context, MMValueInfo *info, MMModel *model,
              const char *input_name, const char *output_name, MMValue *swap,
              GError **error)
{
  MMRealValue *value;
  MMRealValue *rswap = (MMRealValue *)swap;

  g_return_val_if_fail (context, NULL);
  g_return_val_if_fail (info, NULL);
  g_return_val_if_fail ((swap == NULL) || (rswap->swap == NULL), NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  value = g_new (MMRealValue, 1);

  mm_context_ref (context);
  if (model)
    mm_model_ref (model);
  if (swap)
    mm_value_ref (swap);

  value->context = context;
  value->info = mm_value_info_copy (info);
  value->value = NULL;
  value->input_name = g_strdup (input_name);
  value->output_name = g_strdup (output_name);
  value->swap = swap;
  g_atomic_ref_count_init (&value->ref_count);

  return (MMValue *)value;
}

void
mm_value_ref (MMValue *value)
{
  MMRealValue *rvalue = (MMRealValue *)value;
  g_return_if_fail (value);
  g_atomic_ref_count_inc (&rvalue->ref_count);
}

void
mm_value_unref (MMValue *value)
{
  MMRealValue *rvalue = (MMRealValue *)value;
  g_return_if_fail (rvalue);
  if (!g_atomic_ref_count_dec (&rvalue->ref_count))
    return;

  if (rvalue->swap)
    mm_value_unref (rvalue->swap);
  if (rvalue->input_name)
    g_free (rvalue->input_name);
  if (rvalue->output_name)
    g_free (rvalue->output_name);
  if (rvalue->value)
    rvalue->context->api->ReleaseValue (rvalue->value);
  mm_value_info_unref (rvalue->info);
  mm_context_unref (rvalue->context);
  g_free (rvalue);
}

gboolean
mm_value_set_dimension (MMValue *value, GHashTable *hash_table, GError **error)
{
  g_return_val_if_fail (value, FALSE);
  g_return_val_if_fail (hash_table, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);

  mm_value_info_set_dimension (value->info, hash_table);
  return mm_value_update (value, error);
}

gboolean
mm_value_set_data (MMValue *value, gpointer data, GError **error)
{
  MMRealValue *rvalue = (MMRealValue *)value;
  const OrtApi *api;
  void *mutable_data;
  size_t data_size;
  OrtStatus *status;
  g_return_val_if_fail (value, FALSE);
  g_return_val_if_fail (data, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);
  g_return_val_if_fail (rvalue->value, FALSE);

  api = rvalue->context->api;
  status = api->GetTensorMutableData (rvalue->value, &mutable_data);
  if (status)
    goto on_error;

  data_size = mm_value_info_get_data_size (rvalue->info);
  g_return_val_if_fail (data_size, FALSE);
  memcpy (mutable_data, data, data_size);

  return TRUE;
on_error:
  mm_context_set_error (rvalue->context, error, status);
  return FALSE;
}

gpointer
mm_value_get_data (MMValue *value, GError **error)
{
  MMRealValue *rvalue = (MMRealValue *)value;
  const OrtApi *api;
  void *data;
  OrtStatus *status;
  g_return_val_if_fail (rvalue, NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  api = rvalue->context->api;
  status = api->GetTensorMutableData (rvalue->value, &data);
  if (status)
    {
      mm_context_set_error (rvalue->context, error, status);
      return NULL;
    }
  return data;
}

gboolean
mm_value_update_info (MMValue *value, GError **error)
{
  MMRealValue *rvalue = (MMRealValue *)value;
  const OrtApi *api;
  OrtTensorTypeAndShapeInfo *tensor_info = NULL;
  size_t ndim = 0;
  OrtStatus *status;
  g_return_val_if_fail (rvalue, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);

  if (rvalue->value == NULL)
    return FALSE;

  api = rvalue->context->api;
  status = api->GetTensorTypeAndShape (rvalue->value, &tensor_info);
  if (status)
    goto on_ort_error;

  status = api->GetDimensionsCount (tensor_info, &ndim);
  if (status)
    goto on_ort_error;
  if (ndim != value->info->ndim)
    goto on_error;

  status = api->GetDimensions (tensor_info, value->info->dim, ndim);
  if (status)
    goto on_ort_error;

  api->ReleaseTensorTypeAndShapeInfo (tensor_info);
  return true;
on_ort_error:
  mm_context_set_error (rvalue->context, error, status);
on_error:
  g_clear_pointer (&tensor_info, api->ReleaseTensorTypeAndShapeInfo);
  return false;
}

gboolean
mm_value_update (MMValue *value, GError **error)
{
  MMRealValue *rvalue = (MMRealValue *)value;
  MMContext *context;
  MMValueInfo *info;
  OrtAllocator *allocator;
  OrtStatus *status;
  g_return_val_if_fail (rvalue, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);

  context = rvalue->context;
  info = rvalue->info;
  allocator = rvalue->allocator->allocator;
  g_clear_pointer (&rvalue->value, context->api->ReleaseValue);

  status = context->api->CreateTensorAsOrtValue (
      allocator, info->dim, info->ndim, info->dtype, &value->value);
  if (status)
    goto on_ort_error;
  return TRUE;
on_ort_error:
  mm_context_set_error (context, error, status);
  return FALSE;
}

void
mm_value_swap (MMValue *value)
{
  MMRealValue *rvalue = (MMRealValue *)value;
  g_return_if_fail (rvalue);

  if (rvalue->swap == NULL)
    return;
  rvalue->context->api->ReleaseValue (rvalue->swap->value);
  rvalue->swap->value = rvalue->value;
  rvalue->value = NULL;
}
