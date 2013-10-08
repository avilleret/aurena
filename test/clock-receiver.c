#ifdef CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gst/gst.h>
#include <gst/net/gstnet.h>

#ifdef G_OS_UNIX
#include <glib-unix.h>
#include <sys/wait.h>
#endif

GMainLoop *loop;
GstClock *sys_clock;

static gboolean
intr_handler (G_GNUC_UNUSED gpointer user_data)
{
  g_print("Exiting.\n");
  g_main_loop_quit(loop);
  return FALSE;
}

gboolean
clock_poll (GstClock *net_clock)
{
  GstClockTime cur_sys_time = gst_clock_get_time (sys_clock);
  GstClockTime cur_net_time = gst_clock_get_time (net_clock);
  g_print("%" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT " %" GST_TIME_FORMAT
      " (%" G_GINT64_FORMAT ")\n",
      cur_sys_time, cur_net_time, GST_TIME_ARGS(cur_net_time),
      GST_CLOCK_DIFF (cur_net_time, cur_sys_time));

  return TRUE;
}

int
main(int argc, char **argv)
{
  guint signal_watch_id;
  guint timeout_id;
  GstClock *net_clock;
  gchar *server;
  gint clock_port;

  gst_init(&argc, &argv);

  if (argc < 2) {
    g_print ("Usage %s <server> <port>\n", argv[0]);
    return 1;
  }

  server = argv[1];
  clock_port = atoi(argv[2]);

  sys_clock = gst_system_clock_obtain ();
  net_clock = gst_net_client_clock_new ("net_clock", server,
        clock_port, 0);
  if (net_clock == NULL) {
    g_print ("Failed to create net clock client for %s:%d\n",
        server, clock_port);
    return 1;
  }

#ifdef G_OS_UNIX
  signal_watch_id =
      g_unix_signal_add (SIGINT, (GSourceFunc) intr_handler, NULL);
#endif

  loop = g_main_loop_new(NULL, FALSE);
  timeout_id = g_timeout_add(100, (GSourceFunc)clock_poll, net_clock);

  g_print("Ready - polling clock server at %s:%d\n", server, clock_port);
  g_main_loop_run(loop);  

  gst_object_unref (net_clock);
  gst_object_unref (sys_clock);
  g_source_remove (signal_watch_id);
  g_source_remove (timeout_id);
  g_main_loop_unref (loop);

  return 0;
}