
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012 James McLaughlin et al.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "common.h"
#include "pump.h"

lw_pump lw_pump_new (const lw_pumpdef * def)
{
   lw_pump ctx = calloc (sizeof (*ctx) + def->outer_size, 1);
   
   if (!ctx)
      return 0;

   ctx->def = def;

   return ctx;
}

void * lw_pump_outer (lw_pump pump)
{
   return pump + 1;
}

lw_pump lw_pump_inner (void * outer)
{
   return ((lw_pump) outer) - 1;
}

void lw_pump_delete (lw_pump ctx)
{
   if (!ctx)
      return;

   ctx->def->cleanup (ctx);

   free (ctx);
}

void lw_pump_add_user (lw_pump ctx)
{
   ++ ctx->use_count;
}

void lw_pump_remove_user (lw_pump ctx)
{
   -- ctx->use_count;
}

lw_bool lw_pump_in_use (lw_pump ctx)
{
   return ctx->use_count > 0;
}  

void lw_pump_post (lw_pump ctx, void * proc, void * param)
{
   ctx->def->post (ctx, proc, param);
}

#ifdef _WIN32

   lw_pump_watch lw_pump_add (lw_pump ctx, HANDLE handle,
                              void * tag, lw_pump_callback callback)
   {
      return ctx->def->add (ctx, handle, tag, callback);
   }

   void lw_pump_update_callbacks (lw_pump ctx, lw_pump_watch watch,
                                  void * tag, lw_pump_callback callback)
   {
      ctx->def->update_callbacks (ctx, watch, tag, callback);
   }

#else

   lw_pump_watch lw_pump_add (lw_pump ctx, int fd, void * tag,
                              lw_pump_callback on_read_ready,
                              lw_pump_callback on_write_ready,
                              lw_bool edge_triggered)
   {
      return ctx->def->add (ctx, fd, tag, on_read_ready,
                            on_write_ready, edge_triggered);
   }

   void lw_pump_update_callbacks (lw_pump ctx, lw_pump_watch watch, void * tag,
                                  lw_pump_callback on_read_ready,
                                  lw_pump_callback on_write_ready,
                                  lw_bool edge_triggered)
   {
      ctx->def->update_callbacks (ctx, watch, tag, on_read_ready,
                                  on_write_ready, edge_triggered);
   }

#endif

void lw_pump_remove (lw_pump ctx, lw_pump_watch watch)
{
   ctx->def->remove (ctx, watch);
}

