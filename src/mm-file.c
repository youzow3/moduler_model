
#include <gio/gio.h>

#include "mm-file.h"

G_DEFINE_QUARK (mm-file-error, mm_file_error);

typedef enum
{
  MM_FILE_VERSION_ALPHA
} MMFileVersion;

typedef enum
{
  MM_FILE_SUBVERSION_ALPHA
} MMFileSubversion;

typedef struct _MMFileHeader
{
  MMFileVersion version;
  MMFileSubversion subversion;
} MMFileHeader;

typedef struct _MMFileHeaderAlpha
{
  uint64_t nvalues;
} MMFileHeaderAlpha;

typedef struct _MMFileHeaderValueAlpha
{
  uint64_t ndim;
  uint64_t input_name_len;
  uint64_t output_name_len;
  uint64_t data_size;
  uint64_t dim_offset;
  uint64_t input_name_offset;
  uint64_t output_name_offset;
  uint64_t data_offset;
} MMFileHeaderValueAlpha;

typedef struct _MMFileHeaderValueDataAlpha
{
  uint64_t *dim;
  gchar *input_name;
  gchar *output_name;
  void *data;
} MMFileHeaderValueDataAlpha;

/*
 * MMFile - utilities for saving Moduler Model stuff
 *
 * Supported:
 *  - MMValue
 *
 * File structure (with current version implementation):
 *  - version header (MMFileHeader)
 *  - version specific header (MMFileHeaderAlpha)
 *  - version specific variable length header (MMFileHeaderValueAlpha)
 *  - data
 */
struct _MMFile
{
  GFile *file;
  GPtrArray *value_array;
  gatomicrefcount ref_count;
};

MMFile *
mm_file_new (const gchar *path)
{
  MMFile *file;

  file = g_new (MMFile, 1);
  file->file = g_file_new_for_path (path);
  file->value_array
      = g_ptr_array_new_with_free_func ((GDestroyNotify)mm_value_unref);
  g_atomic_ref_count_init (&file->ref_count);
  return file;
}

void
mm_file_ref (MMFile *file)
{
  g_return_if_fail (file);
  g_atomic_ref_count_inc (&file->ref_count);
}

void
mm_file_unref (MMFile *file)
{
  g_return_if_fail (file);
  if (!g_atomic_ref_count_dec (&file->ref_count))
    return;

  g_ptr_array_unref (file->value_array);
  g_object_unref (file->file);
  g_free (file);
}

void
mm_file_add_value (MMFile *file, MMValue *value)
{
  g_return_if_fail (file);
  g_return_if_fail (value);

  mm_value_ref (value);
  if (g_ptr_array_find (file->value_array, value, NULL))
    return;
  g_ptr_array_add (file->value_array, value);
}

void
mm_file_remove_value (MMFile *file, MMValue *value)
{
  guint idx;
  g_return_if_fail (file);
  g_return_if_fail (value);

  if (!g_ptr_array_find (file->value_array, value, &idx))
    return;
  g_ptr_array_remove_index (file->value_array, idx);
}

gboolean
mm_file_write (MMFile *file, GError **error)
{
  GFileOutputStream *stream;
  GArray *data = NULL;
  MMFileHeader header;
  MMFileHeaderAlpha header_alpha;
  MMFileHeaderValueAlpha *header_value_alpha = NULL;
  MMValue **valid_values = NULL;
  size_t valid_value_count = 0;
  size_t offset = 0;
  g_return_val_if_fail (file, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);

  stream = g_file_create (
      file->file, G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION,
      NULL, error);
  if (stream == NULL)
    return FALSE;

  data = g_array_sized_new (FALSE, FALSE, sizeof (GOutputVector),
                            3 + 4 * file->value_array->len);

  header.version = MM_FILE_VERSION_ALPHA;
  header.subversion = MM_FILE_SUBVERSION_ALPHA;
  g_array_append_val (data, ((GOutputVector){ &header, sizeof (header) }));

  header_alpha.nvalues = file->value_array->len;
  g_array_append_val (
      data, ((GOutputVector){ &header_alpha, sizeof (header_alpha) }));

  header_value_alpha = g_new (MMFileHeaderValueAlpha, file->value_array->len);
  valid_values = g_new (MMValue *, file->value_array->len);
  for (size_t k = 0; k < file->value_array->len; k++)
    {
      size_t nelements;
      MMValue *v = file->value_array->pdata[k];
      MMFileHeaderValueAlpha *hv = header_value_alpha + valid_value_count;

      nelements = mm_value_info_get_element_count (v->info);
      if (nelements == 0)
        continue;

      hv->ndim = v->info->ndim;
      hv->input_name_len = v->input_name ? strlen (v->input_name) : 0;
      hv->output_name_len = v->output_name ? strlen (v->output_name) : 0;
      hv->data_size = mm_value_info_get_data_size (v->info);

      hv->dim_offset = offset;
      offset += sizeof (int64_t) * hv->ndim;
      hv->input_name_offset = offset;
      offset += hv->input_name_len + 1;
      hv->output_name_offset = offset;
      offset += hv->output_name_len + 1;
      hv->data_offset = offset;
      offset += hv->data_size;

      valid_values[valid_value_count++] = v;
    }

  g_array_append_val (data, ((GOutputVector){ header_value_alpha,
                                              sizeof (MMFileHeaderValueAlpha)
                                                  * valid_value_count }));

  for (size_t k = 0; k < valid_value_count; k++)
    {
      GOutputVector vec_dim;
      GOutputVector vec_input_name;
      GOutputVector vec_output_name;
      GOutputVector vec_data;
      void *tensor_data;
      MMValue *v = valid_values[k];
      MMFileHeaderValueAlpha *hv = header_value_alpha + k;

      vec_dim.buffer = v->info->dim;
      vec_dim.size = sizeof (int64_t) * hv->ndim;
      vec_input_name.buffer = v->input_name;
      vec_input_name.size = hv->input_name_len + 1;
      vec_output_name.buffer = v->input_name;
      vec_output_name.size = hv->output_name_len + 1;
      tensor_data = mm_value_get_data (v, error);
      if (tensor_data == NULL)
        goto on_error;
      vec_data.buffer = tensor_data;
      vec_data.size = mm_value_info_get_data_size (v->info);

      g_array_append_val (data, vec_dim);
      if (v->input_name)
        g_array_append_val (data, vec_input_name);
      if (v->output_name)
        g_array_append_val (data, vec_output_name);
      g_array_append_val (data, vec_data);
    }

  if (!g_output_stream_writev (G_OUTPUT_STREAM (stream),
                               (GOutputVector *)data->data, data->len, NULL,
                               NULL, error))
    goto on_error;

  g_free (valid_values);
  g_free (header_value_alpha);
  g_array_unref (data);
  g_object_unref (stream);
  return TRUE;
on_error:
  g_free (valid_values);
  g_free (header_value_alpha);
  g_array_unref (data);
  g_object_unref (stream);
  return FALSE;
}

static gboolean
mm_file_read_alpha (MMFile *file, GFileInputStream *stream, GError **error)
{
  MMFileHeaderAlpha header;
  MMFileHeaderValueAlpha *header_value = NULL;
  MMFileHeaderValueDataAlpha *header_value_data = NULL;
  goffset base_offset;

  if (!g_input_stream_read (G_INPUT_STREAM (stream), &header, sizeof (header),
                            NULL, error))
    goto on_error;

  header_value = g_new (MMFileHeaderValueAlpha, header.nvalues);
  if (!g_input_stream_read (G_INPUT_STREAM (stream), header_value,
                            sizeof (MMFileHeaderValueAlpha) * header.nvalues,
                            NULL, error))
    goto on_error;

  header_value_data = g_new0 (MMFileHeaderValueDataAlpha, header.nvalues);
  base_offset = g_seekable_tell (G_SEEKABLE (stream));

  for (int64_t k = 0; k < header.nvalues; k++)
    {
      MMFileHeaderValueAlpha *hv = header_value + k;
      MMFileHeaderValueDataAlpha *hvd = header_value_data + k;

      if (hv->input_name_len)
        {
          hvd->input_name = g_new (gchar, hv->input_name_len);

          if (!g_seekable_seek (G_SEEKABLE (stream),
                                base_offset + hv->input_name_offset,
                                G_SEEK_SET, NULL, error))
            goto on_error;

          if (!g_input_stream_read (G_INPUT_STREAM (stream), hvd->input_name,
                                    hv->input_name_len, NULL, error))
            goto on_error;
        }

      if (hv->output_name_len)
        {
          hvd->output_name = g_new (gchar, hv->output_name_len);
          if (!g_seekable_seek (G_SEEKABLE (stream),
                                base_offset + hv->output_name_offset,
                                G_SEEK_SET, NULL, error))
            goto on_error;

          if (!g_input_stream_read (G_INPUT_STREAM (stream), hvd->output_name,
                                    hv->output_name_len, NULL, error))
            goto on_error;
        }
    }

  for (guint k = 0; k < file->value_array->len; k++)
    {
      bool match = FALSE;
      MMValue *v = file->value_array->pdata[k];
      MMFileHeaderValueAlpha *tgt_hv;
      MMFileHeaderValueDataAlpha *tgt_hvd;

      for (int64_t i = 0; i < header.nvalues; i++)
        {
          MMFileHeaderValueDataAlpha *hvd = header_value_data + i;

          match = !g_strcmp0 (v->input_name, hvd->input_name)
                  || !g_strcmp0 (v->output_name, hvd->output_name);
          tgt_hv = header_value + i;
          tgt_hvd = hvd;
        }

      if (!match)
        continue;

      if (tgt_hvd->dim == NULL)
        {
          tgt_hvd->dim = g_new (uint64_t, tgt_hv->ndim);
          if (!g_seekable_seek (G_SEEKABLE (stream),
                                base_offset + tgt_hv->dim_offset, G_SEEK_SET,
                                NULL, error))
            goto on_error;

          if (!g_input_stream_read (G_INPUT_STREAM (stream), tgt_hvd->dim,
                                    sizeof (int64_t) * tgt_hv->ndim, NULL,
                                    error))
            goto on_error;
        }

      if (tgt_hvd->data == NULL)
        {
          tgt_hvd->data = g_malloc (tgt_hv->data_size);
          if (!g_seekable_seek (G_SEEKABLE (stream),
                                base_offset + tgt_hv->data_offset, G_SEEK_SET,
                                NULL, error))
            goto on_error;

          if (!g_input_stream_read (G_INPUT_STREAM (stream), tgt_hvd->data,
                                    tgt_hv->data_size, NULL, error))
            goto on_error;
        }

      memcpy (v->info->dim, tgt_hvd->dim, sizeof (int64_t) * tgt_hv->ndim);

      if (!mm_value_update (v, error))
        goto on_error;
      if (!mm_value_set_data (v, tgt_hvd->data, error))
        goto on_error;
    }

  for (size_t k = 0; k < header.nvalues; k++)
    {
      MMFileHeaderValueDataAlpha *hvd = header_value_data + k;
      g_free (hvd->dim);
      g_free (hvd->input_name);
      g_free (hvd->output_name);
      g_free (hvd->data);
    }
  g_free (header_value_data);
  g_free (header_value);
  return TRUE;
on_error:
  for (size_t k = 0; k < header.nvalues; k++)
    {
      MMFileHeaderValueDataAlpha *hvd = header_value_data + k;
      g_free (hvd->dim);
      g_free (hvd->input_name);
      g_free (hvd->output_name);
      g_free (hvd->data);
    }
  g_free (header_value_data);
  g_free (header_value);
  return FALSE;
}

gboolean
mm_file_read (MMFile *file, GError **error)
{
  GFileInputStream *stream;
  MMFileHeader header;
  g_return_val_if_fail (file, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);

  stream = g_file_read (file->file, NULL, error);
  if (stream == NULL)
    return FALSE;

  if (!g_input_stream_read (G_INPUT_STREAM (stream), &header, sizeof (header),
                            NULL, error))
    goto on_error;

  if ((header.version == MM_FILE_VERSION_ALPHA)
      && (header.subversion == MM_FILE_SUBVERSION_ALPHA))
    {
      if (!mm_file_read_alpha (file, stream, error))
        goto on_error;
    }
  else
    {
      g_set_error (error, MM_FILE_ERROR, MM_FILE_ERROR_VERSION,
                   "Invalid file version.");
      goto on_error;
    }

  g_object_unref (file);
  return TRUE;
on_error:
  g_object_unref (file);
  return FALSE;
}
