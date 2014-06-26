#ifndef CLIENT_H
#define CLIENT_H

#include <QtGui>
#include <QWidget>
#include <QTabWidget>
#include "ui_client.h"

class ClientWidget : public QTabWidget
{
    Q_OBJECT

public:
    ClientWidget(QWidget *parent = 0);

private slots:

private:

    QLabel *descriptionLabel;
    QPushButton *addButton;
    QVBoxLayout *mainLayout;
    Ui::ClientWidget m_ui;
 
 
};

#endif
