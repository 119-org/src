#include <stdio.h>
#include <assert.h>

long filesize(FILE *fd)
{
    long curpos, length;
    curpos = ftell(fd);
    fseek(fd, 0L, SEEK_END);
    length = ftell(fd);
    fseek(fd, curpos, SEEK_SET);
    return length;
}

int file_send(char *name)
{
    int len, flen;
    char buf[1024] = {0};
    FILE *fd = fopen(name, "r");
    assert(fd);
    flen = filesize(fd);
    printf("file %s length is %u\n", name, flen);

#if 0
    while (1) {
        len = fread(buf, 1, sizeof(buf), fd);
        if (len == 0) {
          assert(feof(fd));
//          xfer_close();
          break;
        } else {
          wlen = xfer_send(buf, len);
          total += wlen;
          if (wlen < len) {
            fseek(fd, wlen - len, SEEK_CUR);
            assert (!feof(fd));
            break;
          }
        }
    }
#endif
}

int file_recv(char *name)
{
    int fd;

}



int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("./test -send / -recv filename\n");
        return 0;
    }
    if (!strcmp(argv[1], "-send"))
        file_send(argv[2]);
    if (!strcmp(argv[1], "-recv"))
        file_recv(argv[2]);
    return 0;
}
