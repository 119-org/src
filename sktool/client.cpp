#include "client.h"

ClientWidget::ClientWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_ui.setupUi(this);
    descriptionLabel = new QLabel(tr("This is client tab"));
    addButton = new QPushButton(tr("client test"));
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addWidget(addButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
    setWindowTitle(tr("Client"));
}
