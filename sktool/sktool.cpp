#include <QtGui>
#include <QWidget>

#include "sktool.h"

Sktool::Sktool()
{
    QWidget* pnl = new QWidget(&m_wnd);
    m_wnd.setCentralWidget(pnl);
    QTabWidget* tab = new QTabWidget(pnl);
    serverWidget = new ServerWidget;
    clientWidget = new ClientWidget;

    fileMenu = menuBar()->addMenu(tr("&File"));
    helpMenu = menuBar()->addMenu(tr("&Help"));

    tab->addTab(serverWidget, serverWidget->windowTitle());
    tab->addTab(clientWidget, clientWidget->windowTitle());
    tab->setCurrentIndex(0);

    QLayout* lay = new QVBoxLayout(pnl);
    lay->setSpacing(5);
    lay->setContentsMargins(5, 5, 5, 5);
    lay->addWidget(tab);

    setWindowTitle(tr("Socket Tool"));
}

void Sktool::show()
{
    m_wnd.show();
}
