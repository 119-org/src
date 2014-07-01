#include <QDate>
#include <QDir>
#include <QTextStream>
#include <QKeySequence>
#include <QApplication>
#include <QClipboard>
#include <QTreeWidget>
#include <QPlainTextEdit>

#include "logger.h"

#define SET_MAX_LOGITM  100
#define SET_MAX_LOGTRM  30

Logger::Logger(QObject *parent)
: QObject(parent),m_treeOut(0),m_textOut(0)
{
}

Logger::~Logger()
{
}

void Logger::init(QTreeWidget* o, QPlainTextEdit* d)
{
    m_cmlog.clear();
    m_cmtxt.clear();

    if (m_treeOut)
        m_treeOut->disconnect(this);

    if (m_textOut)
        m_textOut->disconnect(this);

    m_treeOut = o;
    m_textOut = d;

    if (m_treeOut && m_textOut) {
        QList<QKeySequence> ks;
        ks << QKeySequence(Qt::CTRL + Qt::Key_D);

        QAction* copy = new QAction(tr("Copy"), this);
        copy->setShortcuts(QKeySequence::Copy);
        connect(copy, SIGNAL(triggered()), this, SLOT(copy()));

        QAction* clear = new QAction(tr("Clear"), this);
        clear->setShortcuts(ks);
        connect(clear, SIGNAL(triggered()), this, SIGNAL(clearLog()));

        QAction* all = new QAction(tr("Select All"), this);
        all->setShortcuts(QKeySequence::SelectAll);
        connect(all, SIGNAL(triggered()), m_textOut, SLOT(selectAll()));

        m_cmlog.addAction(copy);
        m_cmlog.addSeparator();
        m_cmlog.addAction(clear);

        m_cmtxt.addAction(copy);
        m_cmtxt.addSeparator();
        m_cmtxt.addAction(all);

        QPalette pal = m_textOut->palette();
        pal.setBrush(QPalette::Base, m_treeOut->palette().brush(QPalette::Window));
        m_textOut->setPalette(pal);

        m_treeOut->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_treeOut, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ctxmenu(const QPoint&)));
        connect(m_treeOut, SIGNAL(itemSelectionChanged()), this, SLOT(syncOutput()));

        m_textOut->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_textOut, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ctxmenu(const QPoint&)));
    }
}

void Logger::syncOutput()
{
    QList<QTreeWidgetItem*> list = m_treeOut->selectedItems();
    if (!list.isEmpty())
        m_textOut->setPlainText(list.first()->text(0));
    else
        m_textOut->clear();
}

void Logger::ctxmenu(const QPoint& pos)
{
    if (sender() == (QObject*)m_treeOut)
        m_cmlog.exec(m_treeOut->mapToGlobal(pos));
    else
        m_cmtxt.exec(m_textOut->mapToGlobal(pos));
}

void Logger::copy()
{
    if (sender() == (QObject*)m_treeOut) {
        QList<QTreeWidgetItem*> list = m_treeOut->selectedItems();
        if (!list.isEmpty())
            QApplication::clipboard()->setText(list.first()->text(0));
    } else {
        m_textOut->copy();
    }
}

void Logger::clear()
{
    m_treeOut->clear();
    m_textOut->clear();
}

void Logger::output(const QString& info)
{
    output("MSG", info);
}

void Logger::output(const char* buf, uint len)
{
    output("DAT", buf, len);
}

void Logger::pack()
{
    if (m_treeOut->topLevelItemCount() > SET_MAX_LOGITM)
        m_treeOut->model()->removeRows(0, SET_MAX_LOGTRM);
    m_treeOut->scrollToBottom();
}

QTreeWidgetItem* Logger::appendLogEntry(QTreeWidgetItem* p, const QString& t)
{
	QTreeWidgetItem* res = new QTreeWidgetItem(p);
	if (res)
	{
		res->setText(0, t);

		if (p)
		{
			p->addChild(res);
		}
		else
		{
			m_treeOut->addTopLevelItem(res);
			m_textOut->setPlainText(t);
		}
	}
	return res;
}

void Logger::output(const QString& title, const QString& info)
{
	QTreeWidgetItem* it = new QTreeWidgetItem(0);
	if (!it) return;

	QString lab(QTime::currentTime().toString("HH:mm:ss "));
	
	lab += title;
	lab += ' ';
	lab += info;

	appendLogEntry(0, lab);

	pack();

	lab += '\n';
	lab += '\n';

}

QString bin2ascii(const char* buf, uint len)
{
	char* tmp = new char[len + 1];

	const char* s = buf;
	const char* e = buf + len;
	char* d = tmp;

	if (s && d)
	{
		while (s < e)
		{
			char c = *s++;
			*d++ = isprint((uchar)c) ? c : '.';
		}

		*d = 0;
	}

	QString res(tmp);

	delete tmp;

	return res;
}

const char* hextab = "0123456789ABCDEF";
QString bin2hex(const char* buf, uint len)
{
	char* tmp = new char[len * 3 + 3];

	const char* s = buf;
	const char* e = buf + len;
	char* d = tmp;

	if (s && d)
	{
		*d++ = '[';

		while (s < e)
		{
			char c = *s++;
			*d++ = hextab[(c>>4)&0xF];
			*d++ = hextab[c & 0xF];
			*d++ = ' ';
		}

		*d++ = ']';
		*d = 0;
	}

	QString res(tmp);

	delete tmp;

	return res;
}

void Logger::output(const QString& title, const char* buf, quint32 len)
{
    QString lab(QTime::currentTime().toString("HH:mm:ss "));
    QTextStream out(&lab);

    out << title << " <" << len << "> " << bin2ascii(buf, len);

    QString hex = bin2hex(buf, len);

    QTreeWidgetItem* it = appendLogEntry(0, lab);

    if (it) {
        appendLogEntry(it, hex);
        pack();
    }
    out << '\n' << hex << '\n' << '\n';
}
