
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
mm_model_io_new (GPtrArray *value_array)
{
  MMModelIO *model_io;

  g_return_val_if_fail (value_array, NULL);

  model_io = g_new (MMModelIO, 1);

  g_ptr_array_ref (value_array);
  model_io->value_array = value_array;
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
mm_model_io_update (MMModelIO *model_io, bool is_input)
{
  GPtrArray *value_array;
  char **names;
  OrtValue **values;
  size_t length = 0;

  g_return_val_if_fail (model_io, FALSE);
  value_array = model_io->value_array;

  for (size_t k = 0; k < value_array->len; k++)
    {
      if (is_input && ((MMValue *)value_array->pdata[k])->input_name)
        length++;
      if (!is_input && ((MMValue *)value_array->pdata[k])->output_name)
        length++;
    }

  names = g_realloc (model_io->names, length * sizeof (char *));
  model_io->names = names;
  values = g_realloc (model_io->values, length * sizeof (OrtValue *));
  model_io->values = values;

  model_io->length = length;

  mm_model_io_update_fast (model_io, is_input);

  return TRUE;
}

void
mm_model_io_update_fast (MMModelIO *model_io, bool is_input)
{
  GPtrArray *value_array;
  g_return_if_fail (model_io);

  value_array = model_io->value_array;
  for (size_t i = 0, j = 0; (i < model_io->length) && (j < value_array->len);
       j++)
    {
      MMValue *v = (MMValue *)value_array->pdata[j];
      if (is_input && v->input_name)
        {
          model_io->names[i] = v->input_name;
          model_io->values[i++] = v->value;
        }
      if (!is_input && v->output_name)
        {
          model_io->names[i] = v->output_name;
          model_io->values[i++] = v->value;
        }
    }
}
