#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QProcess>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <QToolButton>
#include <QWidget>
#include <commandwidget.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

using CommandMap = std::unordered_map<QString, int>;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void onAddTab();

    QToolButton* add_tab_button;

    CommandWidget* curWidget { nullptr };
    QToolButton* curTab { nullptr };
    QList<QToolButton*> tabs;
    QList<CommandWidget*> widgets;

    void exec(const QString& text);

    void onEditDone();
    void onDoubleClick(const QModelIndex& index);
    void onClick(const QModelIndex& index);
    void onExcuteButton();
    void onClear();
    void onRestart();
    void onTerminate();
    void onDelete();
    void onMoveUp();
    void onMoveDown();
    void onSort();

    void setupNewTab(QToolButton* btn, CommandWidget* cmd);

    void selectTab(int bar_index);
    void updateView();

    void loadConfigFile();
    void dumpConfigFile();

    void addInstructions(const QString& intruction, int count = 0);

    void onResize(const QSize& size);

    void setTitle(const QString& title);

    bool check_level1(const QModelIndex& index);

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);

    //    QStringList commands;
    CommandMap commandMap;
    QStringList commands;
    QStringList headers; // command headers
    QStandardItemModel* model { nullptr };

    int last_select_tab = -1;

    QString CONFIG_DIR;
    QString SETUP_FILE;
    QString STYLE_FILE;

    QStringList expandedHeaders;

    QProcess*& getCurrentProc();
    QTextEdit*& getCurrentEdit() { return curWidget->getTextEdit(); }

    void readExpandedHeaders();
    void expandHeaders();

    virtual void paintEvent(QPaintEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    void onSwichToNewTab();

    QPoint press_p;

private:
    Ui::MainWindow* ui;
};
#endif // MAINWINDOW_H
