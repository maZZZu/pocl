/* OpenCL runtime library: clCreateImage()

   Copyright (c) 2012 Timo Viitanen, 2013 Ville Korhonen
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/
#include "pocl_cl.h"
#include "pocl_image_util.h"

extern CL_API_ENTRY cl_mem CL_API_CALL
POname(clCreateImage) (cl_context              context,
                       cl_mem_flags            flags,
                       const cl_image_format * image_format,
                       const cl_image_desc *   image_desc, 
                       void *                  host_ptr,
                       cl_int *                errcode_ret) 
CL_API_SUFFIX__VERSION_1_2
{
    cl_mem mem;
    cl_device_id device_id;
    void *device_ptr;
    unsigned i, j;
    cl_uint num_entries = 0;
    cl_image_format *supported_image_formats;
    int size;
    int errcode;
    int row_pitch;
    int slice_pitch;
    int elem_size;
    int channels;
    
    if (context == NULL) 
      {
        errcode = CL_INVALID_CONTEXT;
        goto ERROR;
      }  
    
    if (image_format == NULL)
      {
        errcode = CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
        goto ERROR;
      }

    if (image_desc == NULL)
      {
        errcode = CL_INVALID_IMAGE_DESCRIPTOR;
        goto ERROR;
      }
    
    if (image_desc->num_mip_levels != 0 || image_desc->num_samples != 0)
      POCL_ABORT_UNIMPLEMENTED();
    
    errcode = POname(clGetSupportedImageFormats)
      (context, flags, image_desc->image_type, 0, NULL, &num_entries);
    
    if (errcode != CL_SUCCESS || num_entries == 0)
      goto ERROR;

    supported_image_formats = malloc (num_entries * sizeof(cl_image_format));
    if (supported_image_formats == NULL)
      {
        errcode = CL_OUT_OF_HOST_MEMORY;
        goto ERROR;
      }
    
    errcode = POname(clGetSupportedImageFormats) (context, flags, 
            image_desc->image_type, num_entries, supported_image_formats, NULL);
    
    if (errcode != CL_SUCCESS)
      goto ERROR_CLEAN_DEV;
    
    for (i = 0; i < num_entries; i++)
      {
        if (supported_image_formats[i].image_channel_order == 
            image_format->image_channel_order &&
            supported_image_formats[i].image_channel_data_type ==
            image_format->image_channel_data_type)
          {
            goto TYPE_SUPPORTED;
          }
      }
    
    errcode = CL_INVALID_VALUE;
    goto ERROR_CLEAN_DEV;

TYPE_SUPPORTED:

    /* maybe they are implemented */
    if (image_desc->image_type != CL_MEM_OBJECT_IMAGE2D &&
        image_desc->image_type != CL_MEM_OBJECT_IMAGE3D)
        POCL_ABORT_UNIMPLEMENTED();
    
    pocl_get_image_information (image_format->image_channel_order,
                                image_format->image_channel_data_type, 
                                &channels, &elem_size);
    
    row_pitch = image_desc->image_row_pitch;
    slice_pitch = image_desc->image_slice_pitch;
    
    size = image_desc->image_width * image_desc->image_height * elem_size * 
      channels;
    
    if (host_ptr != NULL && row_pitch == 0)
      {
        row_pitch = image_desc->image_width * elem_size * channels;
      }
    if (host_ptr != NULL && slice_pitch == 0)
      {
        if (image_desc->image_type == CL_MEM_OBJECT_IMAGE3D ||
            image_desc->image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
          {
            slice_pitch = row_pitch * image_desc->image_height;
          }
        if (image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY)
          {
            slice_pitch = image_desc->image_height;
          }
      }

    /* Create buffer and fill in missing parts */
    mem = POname(clCreateBuffer) (context, flags, size, host_ptr, &errcode);

    if (mem == NULL)
      goto ERROR_CLEAN_DEV;
          
    mem->type = image_desc->image_type;
    mem->is_image = CL_TRUE;
    
    mem->image_width = image_desc->image_width;
    mem->image_height = image_desc->image_height;
    mem->image_depth = image_desc->image_depth;
    mem->image_array_size = image_desc->image_array_size;
    mem->image_row_pitch = row_pitch;
    mem->image_slice_pitch = slice_pitch;
    mem->image_channel_data_type = image_format->image_channel_data_type;
    mem->image_channel_order = image_format->image_channel_order;

#if 0
    printf("mem_image_width %d\n", mem->image_width);
    printf("mem_image_height %d\n", mem->image_height);
    printf("mem_image_depth %d\n", mem->image_depth);
    printf("mem_image_array_size %d\n", mem->image_array_size);
    printf("mem_image_row_pitch %d\n", mem->image_row_pitch);
    printf("mem_image_slice_pitch %d\n", mem->image_slice_pitch);
    printf("mem_host_ptr %u\n", mem->mem_host_ptr);
    printf("mem_image_channel_data_type %x \n",mem->image_channel_data_type);
    printf("device_ptrs[0] %x \n \n", mem->device_ptrs[0]);
#endif
    
    if (errcode_ret != NULL)
      *errcode_ret = CL_SUCCESS;
    
    return mem;
    
 ERROR_CLEAN_DEV:
    for (j = 0; j < i; ++j) 
      {
        device_id = context->devices[j];
        device_id->free(device_id->data, 0, mem->device_ptrs[j]);
      }
    
 ERROR:
    if (errcode_ret) 
      {
        *errcode_ret = errcode;
      }
    return NULL;
}
POsym(clCreateImage)
