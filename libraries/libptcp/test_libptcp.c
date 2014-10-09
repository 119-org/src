#include "libptcp.h"

static void adjust_clock(ptcp_socket_t *ps)
{
#if 0
    uint64_t timeout = 0;

    if (ptcp_get_next_clock(ps, &timeout)) {
        timeout -= get_monotonic_time () / 1000;
//    g_debug ("Socket %p: Adjusting clock to %" G_GUINT64_FORMAT " ms", sock, timeout);
    if (sock == left) {
      if (left_clock != 0)
         g_source_remove (left_clock);
      left_clock = g_timeout_add (timeout, notify_clock, sock);
    } else {
      if (right_clock != 0)
         g_source_remove (right_clock);
      right_clock = g_timeout_add (timeout, notify_clock, sock);
    }
  } else {
    g_debug ("Socket %p should be destroyed, it's done", sock);
    if (sock == left)
      left_closed = TRUE;
    else
      right_closed = TRUE;
    if (left_closed && right_closed)
      g_main_loop_quit (mainloop);
  }
#endif
}

void on_opened(ptcp_socket_t *p, void *data)
{

}

void on_readable(ptcp_socket_t *p, void *data)
{

}

void on_writable(ptcp_socket_t *p, void *data)
{

}

void on_closed(ptcp_socket_t *p, uint32_t error, void *data)
{

}

ptcp_write_result_t write(ptcp_socket_t *p, const char *buf, uint32_t len, void *data)
{

}

int server()
{

}

int client()
{
    ptcp_callbacks_t cbs = {
        NULL, on_opened, on_readable, on_writable, on_closed, write
    };
    ptcp_socket_t *ps = ptcp_create(&cbs);
    if (ps == NULL) {
        printf("error!\n");
    }
    ptcp_notify_mtu(ps, 1496);
    ptcp_connect(ps);
    adjust_clock(ps);


}

int main(int argc, char **argv)
{
    return 0;
}
