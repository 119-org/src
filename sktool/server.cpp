#include "server.h"

ServerWidget::ServerWidget(QWidget *parent)
    : QTabWidget(parent)
{
    descriptionLabel = new QLabel(tr("This is server tab"));
    addButton = new QPushButton(tr("server test"));
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addWidget(addButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
    setWindowTitle(tr("Server"));
}
