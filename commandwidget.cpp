#include "commandwidget.h"
#include "ui_commandwidget.h"

#include <QTextCodec>
#include <QTextStream>
#include <iostream>
#include <windows.h>
#include <QListView>
#include <QFile>
#include <QDir>
#include <QIODeviceBase>
#include <QRegExp>
#include <QCollator>
#include <QTextDocument>
#include "commom.h"


void CommandWidget::pushText(const QString& text)
{
    if(text=="")return;
    ui->textEdit->moveCursor(QTextCursor::End);
    ui->textEdit->insertPlainText(text);
}

void CommandWidget::pushRichText(const QString& text)
{
    if(text=="")return;

    ui->textEdit->moveCursor(QTextCursor::End);
    ui->textEdit->insertHtml(text + "<text style=\" color:#ffffff;\">&nbsp;</text>");
}

CommandWidget::CommandWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CommandWidget)
    ,justExcuted("None")
{
    ui->setupUi(this);
    proc = new QProcess;
    proc->start("cmd");
    connect(proc,&QProcess::readyReadStandardOutput,this,&CommandWidget::readyReadStandardOutputSlot);
    connect(proc,&QProcess::readyReadStandardError,this,&CommandWidget::readyReadStandardErrorSlot);
}

int mmax(int a,int b)
{
    return a>b?a:b;
}

void CommandWidget::readyReadStandardOutputSlot()
{
    QByteArray ba = proc->readAll();
    QTextCodec * textCodec = QTextCodec::codecForName("System");
    QString output = textCodec->toUnicode(ba);
    output.replace('\r',"");

    int first_line = output.indexOf('\n');

    QString line = output.sliced(0,mmax(first_line,0));

    if(line.startsWith(justExcuted))
    {
        pushRichText(SKYBLUE_TEXT(line));
        pushText(output.sliced(first_line));

    }
    else {
        pushText(output);
    }

    ui->textEdit->moveCursor(QTextCursor::End);
}

void CommandWidget::readyReadStandardErrorSlot()
{
    QByteArray ba = proc->readAllStandardError();
    QTextCodec* textCodec = QTextCodec::codecForName("System");
    assert(textCodec != nullptr);

    QString output = textCodec->toUnicode(ba);
    if(output.indexOf("warn",0,Qt::CaseSensitivity::CaseInsensitive)!=-1
    || output.indexOf("error",0,Qt::CaseSensitivity::CaseInsensitive)==-1)
    {
        pushRichText(YELLOW_TEXT(output));
    }
    else pushRichText(RED_TEXT(output));
}

QTextEdit*& CommandWidget::getTextEdit()
{
    return ui->textEdit;
}

void CommandWidget::exec(const QString& text)
{
    if(!check_valid(text))return;
    justExcuted=text.trimmed();
    if(text.endsWith('\n'))
        proc->write(text.toLocal8Bit());
    else proc->write(text.toLocal8Bit()+'\n');
}

CommandWidget::~CommandWidget()
{
    delete ui;
}
