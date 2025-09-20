#pragma once

#include <glib.h>
#include <onnxruntime_c_api.h>

G_BEGIN_DECLS

/*
 * MMModelIO
 * Base structure representing model input/output values and names.
 */
typedef struct _MMModelIO MMModelIO;
/*
 * MMModelInput
 * Same as MMModelIO, but it is for model input.
 */
typedef struct _MMModelInput MMModelInput;
/*
 * MMModelOutput
 * Same as MMModelIO, but it is for model output.
 */
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
/*
 * Set dynamic dimension. hash_table should be (str, int64_t)
 * This function calls mm_model_io_update() internally.
 */
gboolean mm_model_io_set_dimension (MMModelIO *model_io,
                                    GHashTable *hash_table, GError **error);
/* Update values according to MMValue array. */
void mm_model_io_update (MMModelIO *model_io);
/* Update internal MMValue array according to values */
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

G_END_DECLS
