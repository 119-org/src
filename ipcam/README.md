IPCAM ARCH
==========

  video device agent         x264 encode agent               network agent       internet
+---------------+   push    +------------------   push     +-----------------+     |
|      buffer   |-------->  |      buffer     | -------->  |     buffer      |     |
+----------/\---+           +-----------------+            +-----------------+     |
|      push||   |           |  ||in     /\    |            |   ||in    /\    |     |
| EQBUF   DQBUF |           |  \/       ||out |            |   \/      ||out |     |
|  ||      /\   |           |     encoder     |            |    protocol     |     |
+--\/------||---+           +-----------------+            +-----------------+     |
| v4l2 driver   |           |   x264 library  |            | protocol library|     |
+---------------+           +-----------------+            +-----------------+     |

