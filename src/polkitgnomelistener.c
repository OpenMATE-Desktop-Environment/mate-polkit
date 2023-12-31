/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */


#include "config.h"

#include <string.h>
#include <glib/gi18n.h>

#include "polkitgnomelistener.h"
#include "polkitgnomeauthenticator.h"

struct _PolkitGnomeListener
{
  PolkitAgentListener parent_instance;

  /* we support multiple authenticators - they are simply queued up */
  GList *authenticators;

  PolkitGnomeAuthenticator *active_authenticator;
};

struct _PolkitGnomeListenerClass
{
  PolkitAgentListenerClass parent_class;
};

static void polkit_gnome_listener_initiate_authentication (PolkitAgentListener  *listener,
                                                           const gchar          *action_id,
                                                           const gchar          *message,
                                                           const gchar          *icon_name,
                                                           PolkitDetails        *details,
                                                           const gchar          *cookie,
                                                           GList                *identities,
                                                           GCancellable         *cancellable,
                                                           GAsyncReadyCallback   callback,
                                                           gpointer              user_data);

static gboolean polkit_gnome_listener_initiate_authentication_finish (PolkitAgentListener  *listener,
                                                                      GAsyncResult         *res,
                                                                      GError              **error);

G_DEFINE_TYPE (PolkitGnomeListener, polkit_gnome_listener, POLKIT_AGENT_TYPE_LISTENER);

static void
polkit_gnome_listener_init (PolkitGnomeListener *listener)
{
}

static void
polkit_gnome_listener_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (polkit_gnome_listener_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (polkit_gnome_listener_parent_class)->finalize (object);
}

static void
polkit_gnome_listener_class_init (PolkitGnomeListenerClass *klass)
{
  GObjectClass *gobject_class;
  PolkitAgentListenerClass *listener_class;

  gobject_class = G_OBJECT_CLASS (klass);
  listener_class = POLKIT_AGENT_LISTENER_CLASS (klass);

  gobject_class->finalize = polkit_gnome_listener_finalize;

  listener_class->initiate_authentication          = polkit_gnome_listener_initiate_authentication;
  listener_class->initiate_authentication_finish   = polkit_gnome_listener_initiate_authentication_finish;
}

PolkitAgentListener *
polkit_gnome_listener_new (void)
{
  return POLKIT_AGENT_LISTENER (g_object_new (POLKIT_GNOME_TYPE_LISTENER, NULL));
}

typedef struct
{
  PolkitGnomeListener *listener;
  PolkitGnomeAuthenticator *authenticator;

  GSimpleAsyncResult *simple;
  GCancellable *cancellable;

  gulong cancel_id;
} AuthData;

static AuthData *
auth_data_new (PolkitGnomeListener *listener,
               PolkitGnomeAuthenticator *authenticator,
               GSimpleAsyncResult *simple,
               GCancellable *cancellable)
{
  AuthData *data;

  data = g_new0 (AuthData, 1);
  data->listener = g_object_ref (listener);
  data->authenticator = g_object_ref (authenticator);
  data->simple = g_object_ref (simple);
  data->cancellable = g_object_ref (cancellable);
  return data;
}

static void
auth_data_free (AuthData *data)
{
  g_object_unref (data->listener);
  g_object_unref (data->authenticator);
  g_object_unref (data->simple);
  if (data->cancellable != NULL && data->cancel_id > 0)
    g_signal_handler_disconnect (data->cancellable, data->cancel_id);
  g_object_unref (data->cancellable);
  g_free (data);
}

static void
maybe_initiate_next_authenticator (PolkitGnomeListener *listener)
{
  if (listener->active_authenticator == NULL && listener->authenticators != NULL)
    {
      polkit_gnome_authenticator_initiate (POLKIT_GNOME_AUTHENTICATOR (listener->authenticators->data));
      listener->active_authenticator = listener->authenticators->data;
    }
}

static void
authenticator_completed (PolkitGnomeAuthenticator *authenticator,
                         gboolean                  gained_authorization,
                         gboolean                  dismissed,
                         gpointer                  user_data)
{
  AuthData *data = user_data;

  data->listener->authenticators = g_list_remove (data->listener->authenticators, authenticator);
  if (authenticator == data->listener->active_authenticator)
    data->listener->active_authenticator = NULL;

  g_object_unref (authenticator);

  if (dismissed)
    {
      g_simple_async_result_set_error (data->simple,
                                       POLKIT_ERROR,
                                       POLKIT_ERROR_CANCELLED,
                                       _("Authentation dialog was dismissed by the user"));
    }
  g_simple_async_result_complete (data->simple);
  g_object_unref (data->simple);

  maybe_initiate_next_authenticator (data->listener);

  auth_data_free (data);
}

static void
cancelled_cb (GCancellable *cancellable,
              gpointer user_data)
{
  AuthData *data = user_data;

  polkit_gnome_authenticator_cancel (data->authenticator);
}

static void
polkit_gnome_listener_initiate_authentication (PolkitAgentListener  *agent_listener,
                                               const gchar          *action_id,
                                               const gchar          *message,
                                               const gchar          *icon_name,
                                               PolkitDetails        *details,
                                               const gchar          *cookie,
                                               GList                *identities,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data)
{
  PolkitGnomeListener *listener = POLKIT_GNOME_LISTENER (agent_listener);
  GSimpleAsyncResult *simple;
  PolkitGnomeAuthenticator *authenticator;
  AuthData *data;

  simple = g_simple_async_result_new (G_OBJECT (listener),
                                      callback,
                                      user_data,
                                      polkit_gnome_listener_initiate_authentication);

  authenticator = polkit_gnome_authenticator_new (action_id,
                                                  message,
                                                  icon_name,
                                                  details,
                                                  cookie,
                                                  identities);
  if (authenticator == NULL)
    {
      g_simple_async_result_set_error (simple,
                                       POLKIT_ERROR,
                                       POLKIT_ERROR_FAILED,
                                       "Error creating authentication object");
      g_simple_async_result_complete (simple);
      goto out;
    }

  data = auth_data_new (listener, authenticator, simple, cancellable);

  g_signal_connect (authenticator,
                    "completed",
                    G_CALLBACK (authenticator_completed),
                    data);

  if (cancellable != NULL)
    {
      data->cancel_id = g_signal_connect (cancellable,
                                          "cancelled",
                                          G_CALLBACK (cancelled_cb),
                                          data);
    }

  listener->authenticators = g_list_append (listener->authenticators, authenticator);

  maybe_initiate_next_authenticator (listener);

 out:
  ;
}

static gboolean
polkit_gnome_listener_initiate_authentication_finish (PolkitAgentListener  *listener,
                                                      GAsyncResult         *res,
                                                      GError              **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == polkit_gnome_listener_initiate_authentication);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  return TRUE;
}

