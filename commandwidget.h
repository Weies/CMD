#ifndef COMMANDWIDGET_H
#define COMMANDWIDGET_H

#include <QWidget>
#include <QMainWindow>
#include <QStringListModel>
#include <QProcess>
#include <QTextEdit>


inline bool check_valid(const QString& s)
{
    if(s==""||s=="\n"||s==" "||s=="\r")
        return false;
    return true;
}

namespace Ui {
class CommandWidget;
}

class CommandWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommandWidget(QWidget *parent = nullptr);
    ~CommandWidget();

    void exec(const QString& text);

    void pushText(const QString& s);
    void pushRichText(const QString& s);

    QProcess *proc{nullptr};
    QString justExcuted;

    void readyReadStandardErrorSlot();
    void readyReadStandardOutputSlot();

    QProcess*& getProcess(){return proc;}
    QTextEdit*& getTextEdit();

private:
    Ui::CommandWidget *ui;
};

#endif // COMMANDWIDGET_H

