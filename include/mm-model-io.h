#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

typedef struct _MMModelIO MMModelIO;
typedef struct _MMModelInput MMModelInput;
typedef struct _MMModelOutput MMModelOutput;

struct _MMModelInput
{
  const char *const *names;
  const OrtValue *const *values;
  size_t length;
};

struct _MMModelOutput
{
  const char *const *names;
  OrtValue **values;
  size_t length;
};

MMModelIO *mm_model_io_new (GPtrArray *value_array, bool is_input);
void mm_model_io_ref (MMModelIO *model_io);
void mm_model_io_unref (MMModelIO *model_io);
gboolean mm_model_io_set_dimension (MMModelIO *model_io,
                                    GHashTable *hash_table, GError **error);
/* Update values from MMValue */
void mm_model_io_update (MMModelIO *model_io);
/* Update MMValue from values */
gboolean mm_model_io_update_info (MMModelIO *model_io, GError **error);

#define mm_model_input_new(value_array)                                       \
  (MMModelInput *)mm_model_io_new (value_array, TRUE)
#define mm_model_input_ref(model_input)                                       \
  mm_model_io_ref ((MMModelIO *)model_input)
#define mm_model_input_unref(model_input)                                     \
  mm_model_io_unref ((MMModelIO *)model_input)
#define mm_model_input_set_dimension(model_input, hash_table, error)          \
  mm_model_io_set_dimension ((MMModelIO *)model_input, hash_table, error)
#define mm_model_input_update(model_input)                                    \
  mm_model_io_update ((MMModelIO *)model_input)
#define mm_model_input_update_info(model_input, error)                        \
  mm_model_io_update_info ((MMModelIO *)model_input, error)

#define mm_model_output_new(value_array)                                      \
  (MMModelOutput *)mm_model_io_new (value_array, FALSE)
#define mm_model_output_ref(model_output)                                     \
  mm_model_io_ref ((MMModelIO *)model_output)
#define mm_model_output_unref(model_output)                                   \
  mm_model_io_unref ((MMModelIO *)model_output)
#define mm_model_output_set_dimension(model_output, hash_table, error)        \
  mm_model_io_set_dimension ((MMModelIO *)model_output, hash_table, error)
#define mm_model_output_update(model_output)                                  \
  mm_model_io_update ((MMModelIO *)model_output)
#define mm_model_output_update_info(model_output, error)                      \
  mm_model_io_update_info ((MMModelIO *)model_output, error)
