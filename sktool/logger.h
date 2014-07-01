#ifndef LOGGER_H
#define LOGGER_H

#include <QTreeView>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QMenu>

class Logger : public QObject
{
    Q_OBJECT

public:
    Logger(QObject *parent = 0);
    ~Logger();

	void init(QTreeWidget* o, QPlainTextEdit* d);

	void clear();
	void output(const QString& title, const QString& info);
	void output(const char* buf, quint32 len);

signals:
	void clearLog();

public slots:
	void output(const QString& info);
	void output(const QString& title, const char* buf, quint32 len);

private slots:
	void ctxmenu(const QPoint& pos);
	void copy();
	void syncOutput();

private:
	void pack();
	QTreeWidgetItem* appendLogEntry(QTreeWidgetItem* p, const QString& t);

private:
    QMenu m_cmlog;
    QMenu m_cmtxt;
    QTreeWidget* m_treeOut;
    QPlainTextEdit* m_textOut;
};


#endif
