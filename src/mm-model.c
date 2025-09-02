
#include "mm-value-info.h"

#include "mm-model.h"

typedef struct _MMRealModel MMRealModel;

struct _MMRealModel
{
  MMModelOptions *options;
  OrtSession *session;
  OrtAllocator *allocator;
  GPtrArray *input_infos;
  GPtrArray *output_infos;
  gatomicrefcount ref_count;
};

MMModel *
mm_model_new (MMModelOptions *options, const char *file_path, GError **error)
{
  MMRealModel *model;
  MMContext *context;
  OrtSession *session = NULL;
  size_t ninputs = 0;
  size_t noutputs = 0;
  OrtTypeInfo *__info = NULL;
  char *__name = NULL;
  OrtStatus *status;

  g_return_val_if_fail (options, NULL);
  g_return_val_if_fail (file_path, NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);
  context = options->context;

  model = g_new0 (MMRealModel, 1);

  status = context->api->CreateSession (context->env, file_path,
                                        options->session_options, &session);
  if (status)
    goto on_ort_error;

  status = context->api->SessionGetInputCount (session, &ninputs);
  if (status)
    goto on_ort_error;

  status = context->api->SessionGetOutputCount (session, &noutputs);
  if (status)
    goto on_ort_error;

  model->input_infos
      = g_ptr_array_new_full (ninputs, (GDestroyNotify)mm_value_info_unref);
  model->output_infos
      = g_ptr_array_new_full (noutputs, (GDestroyNotify)mm_value_info_unref);

  for (size_t k = 0; k < ninputs; k++)
    {
      const OrtTensorTypeAndShapeInfo *tensor_info;
      MMValueInfo *vinfo;

      status = context->api->SessionGetInputName (session, k,
                                                  context->allocator, &__name);
      if (status)
        goto on_ort_error;

      status = context->api->SessionGetInputTypeInfo (session, k, &__info);
      if (status)
        goto on_ort_error;

      status = context->api->CastTypeInfoToTensorInfo (__info, &tensor_info);
      if (status)
        goto on_ort_error;

      vinfo = mm_value_info_new (context, tensor_info, __name, error);
      if (vinfo == NULL)
        goto on_error;
      g_ptr_array_add (model->input_infos, vinfo);

      g_clear_pointer (&__info, context->api->ReleaseTypeInfo);
      context->allocator->Free (context->allocator, __name);
      __name = NULL;
    }

  for (size_t k = 0; k < noutputs; k++)
    {
      const OrtTensorTypeAndShapeInfo *tensor_info;
      MMValueInfo *vinfo;

      status = context->api->SessionGetOutputName (
          session, k, context->allocator, &__name);
      if (status)
        goto on_ort_error;

      status = context->api->SessionGetOutputTypeInfo (session, k, &__info);
      if (status)
        goto on_ort_error;

      status = context->api->CastTypeInfoToTensorInfo (__info, &tensor_info);
      if (status)
        goto on_ort_error;

      vinfo = mm_value_info_new (context, tensor_info, __name, error);
      if (vinfo == NULL)
        goto on_error;
      g_ptr_array_add (model->output_infos, vinfo);

      g_clear_pointer (&__info, context->api->ReleaseTypeInfo);
      context->allocator->Free (context->allocator, __name);
      __name = NULL;
    }

  mm_model_options_ref (options);

  model->options = options;
  model->session = session;
  g_atomic_ref_count_init (&model->ref_count);

  return (MMModel *)model;
on_ort_error:
  mm_context_set_error (context, error, status);
on_error:
  if (__info)
    context->api->ReleaseTypeInfo (__info);
  if (__name)
    context->allocator->Free (context->allocator, __name);
  g_ptr_array_free (model->output_infos, TRUE);
  g_ptr_array_free (model->input_infos, TRUE);
  if (session)
    context->api->ReleaseSession (session);
  g_free (model);
  return NULL;
}

void
mm_model_ref (MMModel *model)
{
  MMRealModel *rmodel = (MMRealModel *)model;
  g_return_if_fail (rmodel);
  g_atomic_ref_count_inc (&rmodel->ref_count);
}

void
mm_model_unref (MMModel *model)
{
  MMRealModel *rmodel = (MMRealModel *)model;
  MMContext *context;

  g_return_if_fail (rmodel);
  if (!g_atomic_ref_count_dec (&rmodel->ref_count))
    return;

  context = model->options->context;

  g_ptr_array_unref (rmodel->input_infos);
  g_ptr_array_unref (rmodel->output_infos);
  context->api->ReleaseSession (rmodel->session);
  mm_model_options_unref (rmodel->options);
  g_free (rmodel);
}

gboolean
mm_model_run (MMModel *model, MMModelInput *input, MMModelOutput *output,
              GError **error)
{
  MMContext *context;
  OrtStatus *status;
  g_return_val_if_fail (model, FALSE);
  g_return_val_if_fail (input, FALSE);
  g_return_val_if_fail (output, FALSE);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), FALSE);

  context = model->options->context;
  status = context->api->Run (model->session, model->options->run_options,
                              input->names, input->values, input->length,
                              output->names, output->length, output->values);
  if (status)
    {
      mm_context_set_error (context, error, status);
      return FALSE;
    }

  return mm_model_output_update_info (output, error);
}
