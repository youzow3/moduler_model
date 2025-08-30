
#include "mm-model-io.h"
#include "mm-value.h"

struct _MMModelIO
{
  char **names;
  OrtValue **values;
  size_t length;
  GPtrArray *value_array;
  gatomicrefcount ref_count;
};

MMModelIO *
mm_model_io_new (GPtrArray *value_array, bool is_input)
{
  MMModelIO *model_io;
  GStrvBuilder *name_builder;

  g_return_val_if_fail (value_array, NULL);

  model_io = g_new (MMModelIO, 1);

  model_io->value_array = g_ptr_array_new_full (
      value_array->len, (GDestroyNotify)mm_value_unref);
  name_builder = g_strv_builder_new ();
  for (size_t k = 0; k < value_array->len; k++)
    {
      MMValue *v = value_array->pdata[k];
      if (is_input && v->input_name)
        {
          mm_value_ref (v);
          g_ptr_array_add (model_io->value_array, v);
          g_strv_builder_add (name_builder, v->input_name);
        }
      if (!is_input && v->output_name)
        {
          mm_value_ref (v);
          g_ptr_array_add (model_io->value_array, v);
          g_strv_builder_add (name_builder, v->output_name);
        }
    }

  g_ptr_array_set_size (model_io->value_array, model_io->value_array->len);
  model_io->names = g_strv_builder_end (name_builder);
  g_strv_builder_unref (name_builder);
  model_io->length = model_io->value_array->len;
  model_io->values = g_new0 (OrtValue *, model_io->value_array->len);
  g_atomic_ref_count_init (&model_io->ref_count);
  return model_io;
}

void
mm_model_io_ref (MMModelIO *model_io)
{
  g_return_if_fail (model_io);
  g_atomic_ref_count_inc (&model_io->ref_count);
}

void
mm_model_io_unref (MMModelIO *model_io)
{
  g_return_if_fail (model_io);
  if (!g_atomic_ref_count_dec (&model_io->ref_count))
    return;

  g_free (model_io->values);
  g_free (model_io->names);
  g_ptr_array_unref (model_io->value_array);
  g_free (model_io);
}

gboolean
mm_model_io_set_dimension (MMModelIO *model_io, GHashTable *hash_table,
                           GError **error)
{
  g_return_val_if_fail (model_io, FALSE);
  g_return_val_if_fail (hash_table, FALSE);

  for (size_t k = 0; k < model_io->value_array->len; k++)
    {
      MMValue *v = model_io->value_array->pdata[k];
      if (!mm_value_set_dimension (v, hash_table, error))
        goto on_error;
    }

  mm_model_io_update (model_io);
  return TRUE;
on_error:
  return FALSE;
}

void
mm_model_io_update (MMModelIO *model_io)
{
  g_return_if_fail (model_io);

  for (size_t k = 0; k < model_io->value_array->len; k++)
    {
      MMValue *v = model_io->value_array->pdata[k];
      model_io->values[k] = v->value;
    }
}
