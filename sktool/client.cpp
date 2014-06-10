#include "client.h"

ClientWidget::ClientWidget(QWidget *parent)
    : QTabWidget(parent)
{
    descriptionLabel = new QLabel(tr("This is client tab"));
    addButton = new QPushButton(tr("client test"));
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addWidget(addButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
    setWindowTitle(tr("Client"));
}
