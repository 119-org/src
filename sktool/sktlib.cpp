#include <QComboBox>
#include <QToolButton>
#include <QCoreApplication>
#include <QAction>

#include "sktlib.h"

void SocketLib::initNetwork(QComboBox *box)
{
    void *p;
    char str[MAX_ADDR_STRING];
    skt_addr_list_t *tmp;
    if (0 == skt_get_local_list(&tmp, 0)) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
        }
    }

}

