#pragma once

#include <glib.h>

#include "mm-value.h"

typedef enum _MMFileError
{
  MM_FILE_ERROR_VERSION = 1,
} MMFileError;

#define MM_FILE_ERROR mm_file_error_quark ()

typedef struct _MMFile MMFile;

MMFile *mm_file_new (const gchar *path);
void mm_file_ref (MMFile *file);
void mm_file_unref (MMFile *file);
void mm_file_add_value (MMFile *file, MMValue *value);
void mm_file_remove_value (MMFile *file, MMValue *value);
gboolean mm_file_write (MMFile *file, GError **error);
gboolean mm_file_read (MMFile *file, GError **error);
