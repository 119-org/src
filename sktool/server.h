#ifndef SERVER_H
#define SERVER_H

#include <QtGui>
#include <QWidget>
#include "ui_server.h"

class ServerWidget : public QTabWidget
{
    Q_OBJECT

public:
    ServerWidget(QWidget *parent = 0);

private slots:

private:

    QLabel *descriptionLabel;
    QPushButton *addButton;
    QVBoxLayout *mainLayout;
    Ui::ServerWidget m_ui;
    
};

#endif
