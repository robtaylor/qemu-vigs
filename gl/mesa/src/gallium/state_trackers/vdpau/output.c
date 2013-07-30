/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
 * Copyright 2011 Christian König.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <vdpau/vdpau.h>

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_sampler.h"
#include "util/u_format.h"

#include "vdpau_private.h"

/**
 * Create a VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfaceCreate(VdpDevice device,
                         VdpRGBAFormat rgba_format,
                         uint32_t width, uint32_t height,
                         VdpOutputSurface  *surface)
{
   struct pipe_context *pipe;
   struct pipe_resource res_tmpl, *res;
   struct pipe_sampler_view sv_templ;
   struct pipe_surface surf_templ;

   vlVdpOutputSurface *vlsurface = NULL;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating output surface\n");
   if (!(width && height))
      return VDP_STATUS_INVALID_SIZE;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pipe = dev->context->pipe;
   if (!pipe)
      return VDP_STATUS_INVALID_HANDLE;

   vlsurface = CALLOC(1, sizeof(vlVdpOutputSurface));
   if (!vlsurface)
      return VDP_STATUS_RESOURCES;

   vlsurface->device = dev;

   memset(&res_tmpl, 0, sizeof(res_tmpl));

   res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = FormatRGBAToPipe(rgba_format);
   res_tmpl.width0 = width;
   res_tmpl.height0 = height;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   res_tmpl.usage = PIPE_USAGE_STATIC;

   res = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!res) {
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, res, res->format);
   vlsurface->sampler_view = pipe->create_sampler_view(pipe, res, &sv_templ);
   if (!vlsurface->sampler_view) {
      pipe_resource_reference(&res, NULL);
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = res->format;
   surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   vlsurface->surface = pipe->create_surface(pipe, res, &surf_templ);
   if (!vlsurface->surface) {
      pipe_resource_reference(&res, NULL);
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   *surface = vlAddDataHTAB(vlsurface);
   if (*surface == 0) {
      pipe_resource_reference(&res, NULL);
      FREE(dev);
      return VDP_STATUS_ERROR;
   }
   
   pipe_resource_reference(&res, NULL);

   vl_compositor_reset_dirty_area(&vlsurface->dirty_area);

   return VDP_STATUS_OK;
}

/**
 * Destroy a VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
   vlVdpOutputSurface *vlsurface;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying output surface\n");

   vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   pipe_surface_reference(&vlsurface->surface, NULL);
   pipe_sampler_view_reference(&vlsurface->sampler_view, NULL);

   vlRemoveDataHTAB(surface);
   FREE(vlsurface);

   return VDP_STATUS_OK;
}

/**
 * Retrieve the parameters used to create a VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfaceGetParameters(VdpOutputSurface surface,
                                VdpRGBAFormat *rgba_format,
                                uint32_t *width, uint32_t *height)
{
   vlVdpOutputSurface *vlsurface;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Getting output surface parameters\n");
        
   vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   *rgba_format = PipeToFormatRGBA(vlsurface->sampler_view->texture->format);
   *width = vlsurface->sampler_view->texture->width0;
   *height = vlsurface->sampler_view->texture->height0;

   return VDP_STATUS_OK;
}

/**
 * Copy image data from a VdpOutputSurface to application memory in the
 * surface's native format.
 */
VdpStatus
vlVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface,
                                VdpRect const *source_rect,
                                void *const *destination_data,
                                uint32_t const *destination_pitches)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Copy image data from application memory in the surface's native format to
 * a VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfacePutBitsNative(VdpOutputSurface surface,
                                void const *const *source_data,
                                uint32_t const *source_pitches,
                                VdpRect const *destination_rect)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Copy image data from application memory in a specific indexed format to
 * a VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface,
                                 VdpIndexedFormat source_indexed_format,
                                 void const *const *source_data,
                                 uint32_t const *source_pitch,
                                 VdpRect const *destination_rect,
                                 VdpColorTableFormat color_table_format,
                                 void const *color_table)
{
   vlVdpOutputSurface *vlsurface;
   struct pipe_context *context;
   struct vl_compositor *compositor;

   enum pipe_format index_format;
   enum pipe_format colortbl_format;

   struct pipe_resource *res, res_tmpl;
   struct pipe_sampler_view sv_tmpl;
   struct pipe_sampler_view *sv_idx = NULL, *sv_tbl = NULL;

   struct pipe_box box;
   struct pipe_video_rect dst_rect;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Uploading indexed output surface\n");

   vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   context = vlsurface->device->context->pipe;
   compositor = &vlsurface->device->compositor;

   index_format = FormatIndexedToPipe(source_indexed_format);
   if (index_format == PIPE_FORMAT_NONE)
       return VDP_STATUS_INVALID_INDEXED_FORMAT;

   if (!source_data || !source_pitch)
       return VDP_STATUS_INVALID_POINTER;

   colortbl_format = FormatColorTableToPipe(color_table_format);
   if (colortbl_format == PIPE_FORMAT_NONE)
       return VDP_STATUS_INVALID_COLOR_TABLE_FORMAT;

   if (!color_table)
       return VDP_STATUS_INVALID_POINTER;

   memset(&res_tmpl, 0, sizeof(res_tmpl));
   res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = index_format;

   if (destination_rect) {
      res_tmpl.width0 = abs(destination_rect->x0-destination_rect->x1);
      res_tmpl.height0 = abs(destination_rect->y0-destination_rect->y1);
   } else {
      res_tmpl.width0 = vlsurface->surface->texture->width0;
      res_tmpl.height0 = vlsurface->surface->texture->height0;
   }
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.usage = PIPE_USAGE_STAGING;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW;

   res = context->screen->resource_create(context->screen, &res_tmpl);
   if (!res)
      goto error_resource;

   box.x = box.y = box.z = 0;
   box.width = res->width0;
   box.height = res->height0;
   box.depth = res->depth0;

   context->transfer_inline_write(context, res, 0, PIPE_TRANSFER_WRITE, &box,
                                  source_data[0], source_pitch[0],
                                  source_pitch[0] * res->height0);

   memset(&sv_tmpl, 0, sizeof(sv_tmpl));
   u_sampler_view_default_template(&sv_tmpl, res, res->format);

   sv_idx = context->create_sampler_view(context, res, &sv_tmpl);
   pipe_resource_reference(&res, NULL);

   if (!sv_idx)
      goto error_resource;

   memset(&res_tmpl, 0, sizeof(res_tmpl));
   res_tmpl.target = PIPE_TEXTURE_1D;
   res_tmpl.format = colortbl_format;
   res_tmpl.width0 = 1 << util_format_get_component_bits(
      index_format, UTIL_FORMAT_COLORSPACE_RGB, 0);
   res_tmpl.height0 = 1;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.usage = PIPE_USAGE_STAGING;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW;

   res = context->screen->resource_create(context->screen, &res_tmpl);
   if (!res)
      goto error_resource;

   box.x = box.y = box.z = 0;
   box.width = res->width0;
   box.height = res->height0;
   box.depth = res->depth0;

   context->transfer_inline_write(context, res, 0, PIPE_TRANSFER_WRITE, &box, color_table,
                                  util_format_get_stride(colortbl_format, res->width0), 0);

   memset(&sv_tmpl, 0, sizeof(sv_tmpl));
   u_sampler_view_default_template(&sv_tmpl, res, res->format);

   sv_tbl = context->create_sampler_view(context, res, &sv_tmpl);
   pipe_resource_reference(&res, NULL);

   if (!sv_tbl)
      goto error_resource;

   vl_compositor_clear_layers(compositor);
   vl_compositor_set_palette_layer(compositor, 0, sv_idx, sv_tbl, NULL, NULL, false);
   vl_compositor_render(compositor, vlsurface->surface,
                        RectToPipe(destination_rect, &dst_rect), NULL, NULL);

   pipe_sampler_view_reference(&sv_idx, NULL);
   pipe_sampler_view_reference(&sv_tbl, NULL);

   return VDP_STATUS_OK;

error_resource:
   pipe_sampler_view_reference(&sv_idx, NULL);
   pipe_sampler_view_reference(&sv_tbl, NULL);
   return VDP_STATUS_RESOURCES;
}

/**
 * Copy image data from application memory in a specific YCbCr format to
 * a VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface,
                               VdpYCbCrFormat source_ycbcr_format,
                               void const *const *source_data,
                               uint32_t const *source_pitches,
                               VdpRect const *destination_rect,
                               VdpCSCMatrix const *csc_matrix)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

static unsigned
BlendFactorToPipe(VdpOutputSurfaceRenderBlendFactor factor)
{
   switch (factor) {
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO:
      return PIPE_BLENDFACTOR_ZERO;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE:
      return PIPE_BLENDFACTOR_ONE;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_COLOR:
      return PIPE_BLENDFACTOR_SRC_COLOR;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
      return PIPE_BLENDFACTOR_INV_SRC_COLOR;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA:
      return PIPE_BLENDFACTOR_SRC_ALPHA;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
      return PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_ALPHA:
      return PIPE_BLENDFACTOR_DST_ALPHA;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
      return PIPE_BLENDFACTOR_INV_DST_ALPHA;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_COLOR:
      return PIPE_BLENDFACTOR_DST_COLOR;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
      return PIPE_BLENDFACTOR_INV_DST_COLOR;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA_SATURATE:
      return PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_COLOR:
      return PIPE_BLENDFACTOR_CONST_COLOR;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
      return PIPE_BLENDFACTOR_INV_CONST_COLOR;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_ALPHA:
      return PIPE_BLENDFACTOR_CONST_ALPHA;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
      return PIPE_BLENDFACTOR_INV_CONST_ALPHA;
   default:
      assert(0);
      return PIPE_BLENDFACTOR_ONE;
   }
}

static unsigned
BlendEquationToPipe(VdpOutputSurfaceRenderBlendEquation equation)
{
   switch (equation) {
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_SUBTRACT:
      return PIPE_BLEND_SUBTRACT;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_REVERSE_SUBTRACT:
      return PIPE_BLEND_REVERSE_SUBTRACT;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD:
      return PIPE_BLEND_ADD;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MIN:
      return PIPE_BLEND_MIN;
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MAX:
      return PIPE_BLEND_MAX;
   default:
      assert(0);
      return PIPE_BLEND_ADD;
   }
}

static void *
BlenderToPipe(struct pipe_context *context,
              VdpOutputSurfaceRenderBlendState const *blend_state)
{
   struct pipe_blend_state blend;

   memset(&blend, 0, sizeof blend);
   blend.independent_blend_enable = 0;

   if (blend_state) {
      blend.rt[0].blend_enable = 1;
      blend.rt[0].rgb_src_factor = BlendFactorToPipe(blend_state->blend_factor_source_color);
      blend.rt[0].rgb_dst_factor = BlendFactorToPipe(blend_state->blend_factor_destination_color);
      blend.rt[0].alpha_src_factor = BlendFactorToPipe(blend_state->blend_factor_source_alpha);
      blend.rt[0].alpha_dst_factor = BlendFactorToPipe(blend_state->blend_factor_destination_alpha);
      blend.rt[0].rgb_func = BlendEquationToPipe(blend_state->blend_equation_color);
      blend.rt[0].alpha_func = BlendEquationToPipe(blend_state->blend_equation_alpha);
   } else {
      blend.rt[0].blend_enable = 0;
   }

   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.dither = 0;

   return context->create_blend_state(context, &blend);
}

/**
 * Composite a sub-rectangle of a VdpOutputSurface into a sub-rectangle of
 * another VdpOutputSurface; Output Surface object VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                      VdpRect const *destination_rect,
                                      VdpOutputSurface source_surface,
                                      VdpRect const *source_rect,
                                      VdpColor const *colors,
                                      VdpOutputSurfaceRenderBlendState const *blend_state,
                                      uint32_t flags)
{
   vlVdpOutputSurface *dst_vlsurface;
   vlVdpOutputSurface *src_vlsurface;

   struct pipe_context *context;
   struct vl_compositor *compositor;

   struct pipe_video_rect src_rect;
   struct pipe_video_rect dst_rect;

   void *blend;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Composing output surfaces\n");

   dst_vlsurface = vlGetDataHTAB(destination_surface);
   if (!dst_vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   src_vlsurface = vlGetDataHTAB(source_surface);
   if (!src_vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   if (dst_vlsurface->device != src_vlsurface->device)
      return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

   context = dst_vlsurface->device->context->pipe;
   compositor = &dst_vlsurface->device->compositor;

   blend = BlenderToPipe(context, blend_state);

   vl_compositor_clear_layers(compositor);
   vl_compositor_set_layer_blend(compositor, 0, blend, false);
   vl_compositor_set_rgba_layer(compositor, 0, src_vlsurface->sampler_view,
                                RectToPipe(source_rect, &src_rect), NULL);
   vl_compositor_render(compositor, dst_vlsurface->surface,
                        RectToPipe(destination_rect, &dst_rect), NULL, false);

   context->delete_blend_state(context, blend);

   return VDP_STATUS_OK;
}

/**
 * Composite a sub-rectangle of a VdpBitmapSurface into a sub-rectangle of
 * a VdpOutputSurface; Output Surface object VdpOutputSurface.
 */
VdpStatus
vlVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                      VdpRect const *destination_rect,
                                      VdpBitmapSurface source_surface,
                                      VdpRect const *source_rect,
                                      VdpColor const *colors,
                                      VdpOutputSurfaceRenderBlendState const *blend_state,
                                      uint32_t flags)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}
