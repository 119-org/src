#ifndef CLIENT_H
#define CLIENT_H

#include <QtGui>
#include <QWidget>
#include <QTabWidget>

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
 
 
};

#endif
