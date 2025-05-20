#include "commandwidget.h"

#include <windows.h>

#include <QCollator>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QIODeviceBase>
#include <QListView>
#include <QRegExp>
#include <QTextCodec>
#include <QTextDocument>
#include <QTextStream>
#include <iostream>

#include "commom.h"
#include "ui_commandwidget.h"

void CommandWidget::pushText(const QString& text)
{
    if (text == "")
        return;
    ui->textEdit->moveCursor(QTextCursor::End);
    ui->textEdit->insertPlainText(text);
}

void CommandWidget::pushRichText(const QString& text)
{
    if (text == "")
        return;

    ui->textEdit->moveCursor(QTextCursor::End);
    ui->textEdit->insertHtml(text + "<text style=\" color:#ffffff;\">&nbsp;</text>");
}

CommandWidget::CommandWidget(QWidget* parent)
    : QWidget(parent)
    , justExcuted("None")
    , ui(new Ui::CommandWidget)
{
    ui->setupUi(this);
    proc = new QProcess;
    proc->start("cmd");
    connect(proc, &QProcess::readyReadStandardOutput, this,
        &CommandWidget::readyReadStandardOutputSlot);
    connect(proc, &QProcess::readyReadStandardError, this,
        &CommandWidget::readyReadStandardErrorSlot);

    //    setLayout(new QHBoxLayout);
}

int mmax(int a, int b) { return a > b ? a : b; }

void CommandWidget::readyReadStandardOutputSlot()
{
    QByteArray ba = proc->readAll();
    QTextCodec* textCodec = QTextCodec::codecForName("System");
    QString output = textCodec->toUnicode(ba).replace('\r', "");

    auto AddCommandLine = [&](const QString& line) {
        pushRichText(SKYBLUE_TEXT(line) + PURPLE_TEXT("        [" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.z") + "] "));
    };

    if (output.startsWith(justExcuted)) {
        int index = output.indexOf('\n');
        if (index == -1)
            AddCommandLine(output);
        else {
            AddCommandLine(output.sliced(0, index));
            pushText(output.sliced(index));
        }
    } else
        pushText(output);

    ui->textEdit->moveCursor(QTextCursor::End);
}

void CommandWidget::readyReadStandardErrorSlot()
{
    QByteArray ba = proc->readAllStandardError();
    QTextCodec* textCodec = QTextCodec::codecForName("System");
    assert(textCodec != nullptr);

    QString output = textCodec->toUnicode(ba);
    if (output.indexOf("warn", 0, Qt::CaseSensitivity::CaseInsensitive) != -1 || output.indexOf("error", 0, Qt::CaseSensitivity::CaseInsensitive) == -1) {
        pushRichText(YELLOW_TEXT(output));
    } else
        pushRichText(RED_TEXT(output));
}

QTextEdit*& CommandWidget::getTextEdit() { return ui->textEdit; }

void CommandWidget::exec(const QString& text)
{
    justExcuted = text.trimmed();
    if (proc->state() != QProcess::ProcessState::Running) {
        pushRichText(RED_TEXT("\n\n ***************** Process ended, unable to exec command, please click Restart button. ***************** \n\n"));
        ui->textEdit->moveCursor(QTextCursor::End);
        return;
    }
    if (!check_valid(justExcuted))
        return;
    if (justExcuted.endsWith('\n'))
        proc->write(justExcuted.toLocal8Bit());
    else
        proc->write(justExcuted.toLocal8Bit() + '\n');
}

CommandWidget::~CommandWidget() { delete ui; }
