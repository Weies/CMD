#include "MainWindow.h"
#include "QMenuBar"
#include "QMessageBox"
#include "commom.h"
#include "ui_MainWindow.h"
#include "ui_commandwidget.h"
#include <QBrush>
#include <QCollator>
#include <QDir>
#include <QEventPoint>
#include <QFile>
#include <QIODeviceBase>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QRegExp>
#include <QStandardItem>
#include <QTextCodec>
#include <QTextEdit>
#include <QTextStream>
#include <iostream>
#include <windows.h>

#include <QIODeviceBase>
#define Read QIODeviceBase::OpenModeFlag::ReadOnly
#define Write QIODeviceBase::OpenModeFlag::WriteOnly

QString dir(const QString& file)
{
    return QCoreApplication::applicationDirPath() + '/' + file;
}

const char* SETUP_HEAD = "# QCMD setup file.\n\n";

bool MainWindow::check_level1(const QModelIndex& index)
{
    return index.isValid() && !index.parent().isValid();
}

QString cutTabTitle(const QString& title, int head, int tail)
{
    if (title.size() < head + tail + 3)
        return title;
    return title.sliced(0, head) + "..." + title.sliced(title.size() - tail);
}

void write_field(const QString& name, const QString& value, QFile& f)
{
    QString head = name + "=[" + value + "]\n\n";
    f.write(head.toUtf8());
}

QString dumpStringList(const QStringList& s, char delima = ',')
{
    QString r;
    for (auto& str : s) {
        r += str + delima;
    }
    if (r.size())
        r.resize(r.size() - 1);
    return r;
}

QString dumpCommandMap(CommandMap s, const QStringList& cs, char delima = ',')
{
    QString r = "\n";
    for (auto& str : cs) {
        r += str + "^" + QString::number(s[str]) + '\n';
    }
    r += '\n';

    if (r.size())
        r.resize(r.size() - 1);
    return r;
}

void MainWindow::loadConfigFile()
{
    QDir d;
    if (d.exists(CONFIG_DIR)) {
        QFile f(SETUP_FILE);
        f.open(Read);
        if (!f.isOpen()) {
            QTextEdit* edit = ui->cmd_widget->getTextEdit();
            edit->moveCursor(QTextCursor::Start);
            edit->append("Found no configuration file, skipped initialization.\n");
            return;
        }

        QString text = f.readAll();

        auto _get_content = [](const QString& s) -> QString { return s.sliced(s.indexOf('[') + 1, s.indexOf(']') - s.indexOf('[') - 1); };

        while (!text.isEmpty()) {
            auto lineEnd = text.indexOf('\n');
            QString line = text.sliced(0, lineEnd);
            if (check_valid(line) && !line.startsWith("#")) {
                QString content = _get_content(text);
                lineEnd = text.indexOf(']');

                if (line.startsWith("recent_commands")) {
                    QStringList command_lines_list = content.split('\n');
                    for (QString& command : command_lines_list) {
                        if (!check_valid(command))
                            continue;

                        QStringList p = command.split('^');
                        commands.append(p[0].trimmed());
                        if (p.size() > 1)
                            commandMap[p[0].trimmed()] = p[1].trimmed().toInt();
                    }
                } else if (line.startsWith("expanded_headers")) {
                    expandedHeaders = content.split(',');
                } else if (line.startsWith("geometry")) {
                    int a[4] = { 0, 0, 600, 400 };
                    sscanf(content.toLocal8Bit(), "%d %d %d %d", a, a + 1, a + 2, a + 3);
                    setGeometry(QRect(a[0], a[1], a[2], a[3]));
                } else if (line.startsWith("window_title")) {
                    auto str = ui->title_label->toHtml();
                    ui->title_label->setHtml(str.replace("qCMD", content));
                } else {
                    lineEnd = line.size();
                }
            }
            text = text.sliced(lineEnd + 1).trimmed();
        }
        f.close();
    } else {
        d.mkdir(CONFIG_DIR);
    }

    updateView();
}

void MainWindow::dumpConfigFile()
{
    QDir d;
    if (!d.exists(CONFIG_DIR))
        d.mkdir(CONFIG_DIR);
    if (!d.exists(CONFIG_DIR + "/backup"))
        d.mkdir(CONFIG_DIR + "/backup");

    QFile f(SETUP_FILE);
    f.open(Write);
    f.write(SETUP_HEAD);

    const auto& r = this->normalGeometry();
    write_field("geometry", fmt("{} {} {} {}", r.x(), r.y(), r.width(), r.height()), f);

    write_field("window_title", ui->title_label->toPlainText(), f);

    write_field("recent_commands", dumpCommandMap(commandMap, commands, ','), f);

    readExpandedHeaders();
    write_field("expanded_headers", dumpStringList(expandedHeaders, ','), f);

    f.close();

    // get output and save to a copy file.
    f.open(Read);
    QString config = f.readAll();
    f.close();
    f.setFileName(CONFIG_DIR + "/backup/setup_copy_" + QDateTime::currentDateTime().toString("hh.mm.ss dd.MM.yyyy") + ".txt");
    f.open(Write);
    f.write(config.toUtf8());
    f.close();
}

void MainWindow::setupNewTab(QToolButton* btn, CommandWidget* cmd)
{
    btn->setText("cmd");
    btn->setToolTip("cmd");
    btn->setMinimumSize({ 60, 20 });
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    tabs.push_back(btn);
    widgets.push_back(cmd);

    if (cmd != ui->cmd_widget) {
        ui->tab_layout->insertWidget(tabs.size() - 1, btn);
    }
    connect(btn, &QToolButton::clicked, [this, btn]() { selectTab(this->tabs.indexOf(btn)); });
}

void MainWindow::selectTab(int bar_index)
{
    if (bar_index == last_select_tab || bar_index > widgets.size())
        return;

    curTab = tabs[bar_index];
    curTab->setProperty("class", "active_tab");
    Update_Style(curTab);

    curWidget = widgets[bar_index];

    if (last_select_tab != -1) {
        ui->horizontalLayout_3->removeWidget(widgets[last_select_tab]);
        widgets[last_select_tab]->hide();
        tabs[last_select_tab]->setProperty("class", "rest_tab");
        Update_Style(tabs[last_select_tab]);

        ui->horizontalLayout_3->insertWidget(0, curWidget);
        ui->horizontalLayout_3->setStretch(0, 7);
        ui->horizontalLayout_3->setStretch(1, 3);
    }

    curWidget->show();

    last_select_tab = bar_index;
    onResize(this->size());
}

void MainWindow::onAddTab()
{
    QToolButton* btn = new QToolButton(this);
    CommandWidget* cmd = new CommandWidget(this);
    setupNewTab(btn, cmd);
    selectTab(tabs.size() - 1);
}

void loadStyle(const QString& file, MainWindow* win)
{
    QFile f(file);

    if (f.exists()) {
        f.open(Read);
        QString style = f.readAll();
        win->setStyleSheet(style);
    } else {
        (new QMessageBox(QMessageBox::Icon::Warning, "Failed load stylesheet", "Failed load stylesheet,file: " + file + " not exist.\n"))->show();
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    CONFIG_DIR = dir("config");
    SETUP_FILE = dir("config/setup.txt");
    STYLE_FILE = dir("config/style.css");

    model = new QStandardItemModel();

    connect(ui->add_bar, &QPushButton::clicked, this, &MainWindow::onAddTab);

    setupNewTab(ui->cmd_bar, ui->cmd_widget);
    selectTab(0);

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::onEditDone);

    connect(ui->clear_button, &QPushButton::clicked, this, &MainWindow::onClear);
    connect(ui->excute_button, &QPushButton::clicked, this, &MainWindow::onExcuteButton);
    connect(ui->restart_button, &QPushButton::clicked, this, &MainWindow::onRestart);
    //    connect(ui->terminate_button,&QPushButton::clicked,this,&MainWindow::onTerminate);

    connect(ui->delete_button, &QPushButton::clicked, this, &MainWindow::onDelete);
    connect(ui->move_up_button, &QPushButton::clicked, this, &MainWindow::onMoveUp);
    connect(ui->move_down_button, &QPushButton::clicked, this, &MainWindow::onMoveDown);
    connect(ui->sort_button, &QPushButton::clicked, this, &MainWindow::onSort);

    connect(ui->treeView, &QListView::doubleClicked, this, &MainWindow::onDoubleClick);
    connect(ui->treeView, &QListView::clicked, this, &MainWindow::onClick);
    // connect(ui->treeView,&QTreeView::expanded,[this](){readExpandedHeaders();});

    ui->treeView->setModel(model);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->add_bar->setIcon(QIcon(":/imgs/add"));

    this->setWindowFlags(Qt::FramelessWindowHint);

    ui->min_btn->setIcon(style()->standardPixmap(QStyle::SP_TitleBarMinButton));
    ui->max_btn->setIcon(style()->standardPixmap(QStyle::SP_TitleBarMaxButton));
    ui->close_btn->setIcon(style()->standardPixmap(QStyle::SP_TitleBarCloseButton));
    ui->stay_top_btn->setIcon(QIcon(":/imgs/stay_top"));

    connect(ui->close_btn, &QToolButton::clicked, this, &QMainWindow::close);
    connect(ui->min_btn, &QToolButton::clicked, this, &QMainWindow::showMinimized);
    connect(ui->max_btn, &QToolButton::clicked, [this]() {
        if (this->isMaximized())
            this->showNormal();
        else
            this->showMaximized();
    });
    connect(ui->stay_top_btn, &QToolButton::clicked, [this]() {
        static bool stay_top = false;
        if (!stay_top)
            // a bad experience that when switched stay on top, the window will disapear.
            this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
        else
            this->setWindowFlags(this->windowFlags() ^ Qt::WindowStaysOnTopHint);
        stay_top = !stay_top;
        this->show();
    });
    this->setWindowIcon(QIcon(":/imgs/icon"));

    loadConfigFile();
    loadStyle(STYLE_FILE, this);
    expandHeaders();
}
void MainWindow::onResize(const QSize& size)
{
}
void MainWindow::setTitle(const QString& title)
{
    ui->title_label->setText(title);
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QMainWindow::paintEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* e)
{
    onResize(e->size());
}

void MainWindow::updateView()
{
    readExpandedHeaders();
    model->clear();
    headers.clear();
    auto copyCommands = commands;
    auto copyCommandMap = commandMap;
    commands.clear();
    commandMap.clear();

    headers.append("*HOT*");
    QStandardItem* item = new QStandardItem(headers[0]);

    QBrush b;
    b.setColor(QColor(255, 0, 255));
    item->setBackground(b);
    item->setIcon(QIcon(":/imgs/favo.png"));

    std::map<int, QStringList> topInstMap;
    for (auto& [str, count] : copyCommandMap)
        topInstMap[count].append(str);

    const int tops = 18;
    QStringList topInstructions;
    for (auto it = topInstMap.rbegin(); it != topInstMap.rend(); ++it) {
        if (topInstructions.size() > tops || it->first == 0)
            break;
        topInstructions.append(it->second);
    }
    topInstructions.resize(std::min(tops, (int)topInstructions.size()));
    std::sort(topInstructions.begin(), topInstructions.end());
    for (auto& inst : topInstructions)
        item->appendRow(new QStandardItem(inst));

    model->appendRow(item);

    for (auto& command : copyCommands) {
        addInstructions(command, copyCommandMap[command]);
    }
    expandHeaders();
}

void MainWindow::mouseMoveEvent(QMouseEvent* e)
{
    if (ui->top_widget->underMouse() && e->type() == QMouseEvent::MouseMove) {
        this->move(e->point(0).globalPosition().toPoint() - press_p);
    }
}

void MainWindow::mousePressEvent(QMouseEvent* e)
{
    press_p = e->point(0).pressPosition().toPoint();
}

MainWindow::~MainWindow()
{
    dumpConfigFile();
    delete ui;
    delete model;
}

void MainWindow::addInstructions(const QString& _instruction, int count)
{
    QString instruction = _instruction.trimmed();
    if (!check_valid(instruction) || commandMap.count(instruction)) {
        return;
    }

    if (!instruction.startsWith("bat ") && instruction.endsWith(".bat")) {
        instruction = "bat " + instruction;
    }
    QString head = instruction.split(' ')[0];
    if (headers.indexOf(head) == -1) {
        headers.append(head);
        QStandardItem* item = new QStandardItem(head);
        QBrush b;
        b.setColor(QColor(255, 0, 255));
        item->setBackground(b);
        item->setIcon(QIcon(":/imgs/folder"));

        item->appendRow(new QStandardItem(instruction));
        model->appendRow(item);
    } else {
        int idx = headers.indexOf(head);
        auto* ptr = model->item(idx, 0);
        ptr->appendRow(new QStandardItem(instruction));
    }
    commandMap[instruction] = count;
    commands.push_back(instruction);
}

void MainWindow::onExcuteButton()
{
    const QString& str = ui->lineEdit->text();
    ui->lineEdit->setText("");
    exec(str);
}

void MainWindow::readExpandedHeaders()
{
    expandedHeaders.clear();
    for (auto id = ui->treeView->indexAt({ 0, 0 }); id.isValid(); id = ui->treeView->indexBelow(id)) {
        if (ui->treeView->isExpanded(id)) {
            expandedHeaders << model->itemFromIndex(id)->text();
        }
    }
}
void MainWindow::expandHeaders()
{
    for (auto id = ui->treeView->indexAt({ 0, 0 }); id.isValid(); id = ui->treeView->indexBelow(id)) {
        const auto& header = model->itemFromIndex(id)->text();
        if (expandedHeaders.indexOf(header) != -1) {
            ui->treeView->setExpanded(id, true);
        }
    }
}

void MainWindow::onRestart()
{
    QProcess*& proc = curWidget->getProcess();
    curWidget->getTextEdit()->clear();

    delete proc;

    proc = new QProcess;
    proc->start("cmd");

    curTab->setText("cmd");
    curTab->setToolTip("cmd");
    connect(proc, &QProcess::readyReadStandardOutput, curWidget, &CommandWidget::readyReadStandardOutputSlot);
    connect(proc, &QProcess::readyReadStandardError, curWidget, &CommandWidget::readyReadStandardErrorSlot);
}

void MainWindow::onTerminate()
{
    curWidget->getProcess()->write("\0x03\n", 3);
}

void MainWindow::onClear()
{
    curWidget->getTextEdit()->clear();
}

void MainWindow::onDelete()
{
    auto index = ui->treeView->currentIndex();
    const QString& text = model->itemFromIndex(index)->text();
    if (!index.isValid())
        return;

    if (check_level1(index)) {
        auto copy = commands;
        for (auto& command : copy) {
            if (command.startsWith(text)) {
                copy.remove(copy.indexOf(command));
                commandMap.erase(commandMap.find(command));
            }
        }
        commands = copy;
    } else {
        commands.remove(commands.indexOf(text));
        commandMap.erase(commandMap.find(text));
    }
    updateView();
    dumpConfigFile();
}

void MainWindow::onSort()
{
    QCollator co;
    std::sort(commands.begin(), commands.end(), co);
    updateView();
    dumpConfigFile();
}

void MainWindow::onMoveDown()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid() || check_level1(index))
        return;

    QString text = model->itemFromIndex(index)->text();
    int the_index = commands.indexOf(text);
    if (index.isValid() && the_index < commands.size() - 1) {
        commands.swapItemsAt(the_index, the_index + 1);

        dumpConfigFile();
        updateView();
        ui->treeView->setCurrentIndex(ui->treeView->indexBelow(index));
    }
}

void MainWindow::onMoveUp()
{
    auto index = ui->treeView->currentIndex();
    if (!index.isValid() || check_level1(index))
        return;

    const QString& text = model->itemFromIndex(index)->text();
    int the_index = commands.indexOf(text);
    if (index.isValid() && the_index > 0) {
        commands.swapItemsAt(the_index, the_index - 1);
        updateView();
        dumpConfigFile();
        ui->treeView->setCurrentIndex(ui->treeView->indexAbove(index));
    }
}

void MainWindow::exec(const QString& _cmd)
{
    QString cmd = _cmd.trimmed();
    if (!check_valid(cmd))
        return;

    curTab->setText(cutTabTitle(cmd, 12, 10));
    curTab->setToolTip(cmd);
    if (!commandMap.count(cmd)) {
        commands.push_back(cmd);
        commandMap[cmd] = 1;
        updateView();
        dumpConfigFile();
    } else {
        commandMap[cmd] += 1;
    }

    if (cmd == "cls") {
        getCurrentEdit()->clear();
        curWidget->exec(cmd);
    } else if (cmd.startsWith("bat ")) {
        curWidget->exec(cmd.sliced(4));
    } else
        curWidget->exec(cmd);
}

void MainWindow::onEditDone()
{
    const QString& str = ui->lineEdit->text().trimmed();
    if (str == "")
        return;
    ui->lineEdit->setText("");

    exec(str);
}

void MainWindow::onDoubleClick(const QModelIndex& index)
{
    if (!check_level1(index)) {
        exec(model->itemFromIndex(index)->text());
    }
}

void MainWindow::onClick(const QModelIndex& index)
{
    if (!check_level1(index)) {
        ui->lineEdit->setText(model->itemFromIndex(index)->text());
    }
}
