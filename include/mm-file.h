#pragma once

#include <glib.h>

#include "mm-value.h"

G_BEGIN_DECLS

typedef enum _MMFileError
{
  MM_FILE_ERROR_VERSION = 1,
} MMFileError;

#define MM_FILE_ERROR mm_file_error_quark ()

/*
 * MMFile
 * Used to save/load Moduler-Model data. Currently supports MMValue only.
 */
typedef struct _MMFile MMFile;

MMFile *mm_file_new (const gchar *path);
void mm_file_ref (MMFile *file);
void mm_file_unref (MMFile *file);
/* Adds value to save/load */
void mm_file_add_value (MMFile *file, MMValue *value);
/* Remove value from file if value is added. */
void mm_file_remove_value (MMFile *file, MMValue *value);
/* Writes holding data to path given at mm_file_new() */
gboolean mm_file_write (MMFile *file, GError **error);
/*
 * Reads data from path given at mm_file_new().
 * You should add data (for example, MMValue) before call this function.
 * Only necessary data will be loaded, and other data will be discarded.
 */
gboolean mm_file_read (MMFile *file, GError **error);

G_END_DECLS
