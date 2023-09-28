#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QProcess>
#include <QToolButton>
#include <QList>
#include <QWidget>
#include <commandwidget.h>
#include <QResizeEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void onAddTab();

    QToolButton* add_tab_button;

    CommandWidget* curWidget{nullptr};
    QToolButton* curTab{nullptr};
    QList<QToolButton*> tabs;
    QList<CommandWidget*> widgets;

    void exec(const QString& text);

    void onEditDone();
    void onDoubleClick(const QModelIndex& index);
    void onClick(const QModelIndex& index);
    void onExcuteButton();
    void onClear();
    void onRestart();
    void onDelete();
    void onMoveUp();
    void onMoveDown();
    void onSort();

    void setupNewTab(QToolButton* btn,CommandWidget* cmd);

    void selectTab(int bar_index);
    void updateView();

    void loadConfigFile();
    void dumpConfigFile();

    void addInstructions(const QString& intruction);

    void onResize(const QSize& size);

    bool check_level1(const QModelIndex& index);

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);

    QStringList commands;
    QStringList headers;// command headers
    QStandardItemModel *model{nullptr};

    static const QString SCMD_DIR;
    static const QString SETUP_FILE;
    static const QString SETUP_FILE_COPY;
    QString justExcuted;
    QStringList expandedHeaders;

    QProcess*& getCurrentProc();
    QTextEdit*& getCurrentEdit(){ return curWidget->getTextEdit();}

    void readExpandedHeaders();
    void expandHeaders();

    virtual void paintEvent(QPaintEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

    void onSwichToNewTab();

    QPoint press_p;
private:

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
