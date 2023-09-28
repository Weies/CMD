#include "MainWindow.h"
#include "ui_MainWindow.h"

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
#include <QTextEdit>
#include <QMouseEvent>
#include <QEventPoint>
#include <QStandardItem>
#include <QBrush>
#include <QPainter>
#include "ui_commandwidget.h"
#include "commom.h"


const QString MainWindow::SCMD_DIR = "c:/scmd";
const QString MainWindow::SETUP_FILE = "c:/scmd/setup.txt";
const QString MainWindow::SETUP_FILE_COPY = "c:/scmd/setup_copy.txt";
const char* SETUP_HEAD ="# SCMD setup file.\n\n";

#pragma execution_character_set("UTF-8")


bool MainWindow::check_level1(const QModelIndex& index)
{
    return index.isValid() && !index.parent().isValid();
}

QString cutTabTitle(const QString& title,int head,int tail)
{
    if(title.size()<head+tail+3)
        return title;
    return title.sliced(0,head)+"..."+title.sliced(title.size()-tail);
}

void MainWindow::loadConfigFile()
{
    QDir d;
    if(d.exists(SCMD_DIR))
    {
        QFile f(SETUP_FILE);
        if(!f.exists(SETUP_FILE))
        {
            f.setFileName(SETUP_FILE_COPY);
            if(!f.exists(SETUP_FILE_COPY))
            {
                QTextEdit* edit = ui->cmd_wdiget->getTextEdit();
                edit->moveCursor(QTextCursor::Start);
                edit->append("Found no configuration file, skipped initialization.\n");
                return;
            }
        }
        f.open(Read);
        QString str = f.readAll();
        str.replace("\r","");
        QStringList list = str.split('\n');
        for(auto& line: list)
        {
            if(line.startsWith("#"))
                continue;
            if(line.startsWith("recent_commands"))
            {
                QString command_lines = line.sliced(line.indexOf('[')+1,line.lastIndexOf(']')-line.indexOf('[')-1);
                QStringList command_lines_list =command_lines.split(',');
                for(QString& command: command_lines_list)
                {
                    addInstructions(command);
                }
            }
            else if(line.startsWith("expanded_headers"))
            {
                QString headers = line.sliced(line.indexOf('[')+1,line.lastIndexOf(']')-line.indexOf('[')-1);
                expandedHeaders = headers.split(',');
            }
            else if(line.startsWith("geometry"))
            {
                QString geo = line.sliced(line.indexOf('[')+1,line.lastIndexOf(']')-line.indexOf('[')-1);
                int a[4];
                sscanf(geo.toLocal8Bit(),"%d %d %d %d",a , a+1, a+2, a+3);
                setGeometry(QRect(a[0] , a[1], a[2], a[3]));
            }
        }
        f.close();
    }
}

void MainWindow::dumpConfigFile()
{
    QDir d;
    if(!d.exists(SCMD_DIR))
        d.mkdir(SCMD_DIR);

    QFile f(SETUP_FILE);
    f.open(Write);
    f.write(SETUP_HEAD);

    f.write("recent_commands=[");
    for(const QString& inst:commands)
    {
        if(!check_valid(inst))
            continue;
        f.write(inst.toLocal8Bit()+',');
    }
    f.write("]\n\n");

    readExpandedHeaders();
    f.write("expanded_headers=[");
    for(const QString& inst:expandedHeaders)
    {
        if(!check_valid(inst))
            continue;
        f.write(inst.toLocal8Bit()+',');
    }
    f.write("]\n\n");

    f.write("geometry=[");
    const auto& r = this->geometry();
    QString geo = fmt("{} {} {} {}",r.x(),r.y(),r.width(),r.height());
    f.write(geo.toLocal8Bit());
    f.write("]\n\n");

    f.close();

    // get output and save to a copy file.
    f.open(Read);
    QString config = f.readAll();
    f.close();
    f.setFileName(SCMD_DIR+"/setup_copy_"+QDateTime::currentDateTime().toString("hh.mm.ss dd.MM.yyyy")+".txt");
    f.open(Write);
    f.write(config.toLocal8Bit());
    f.close();
}

void MainWindow::setupNewTab(QToolButton* btn,CommandWidget* cmd)
{
    btn->setText("cmd");
    btn->setToolTip("cmd");
    tabs.push_back(btn);
    widgets.push_back(cmd);
    btn->setMinimumSize({60,20});
    btn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
    if(cmd!=ui->cmd_wdiget)
        ui->horizontalLayout->insertWidget(tabs.size()-1,btn);
    btn->setToolTip("cmd");
    connect(btn,&QToolButton::clicked,[this,btn](){selectTab(this->tabs.indexOf(btn));});
}

void MainWindow::selectTab(int bar_index)
{
    static int last_select = -1;
    if(bar_index==last_select||bar_index>widgets.size())
        return;

    if(last_select!=-1)
    {
        widgets[last_select]->hide();
        tabs[last_select]->setProperty("class","rest_tab");
        tabs[last_select]->style()->polish(tabs[last_select]);
        Update_Style(tabs[last_select]);
    }
    curTab = tabs[bar_index];
    curTab->setProperty("class","active_tab");
    Update_Style(curTab);
    curWidget = widgets[bar_index];
    curWidget->setParent(ui->cmd_window);
    curWidget->show();

    last_select=bar_index;
    onResize(this->size());
}


void MainWindow::onAddTab()
{
    QToolButton* btn = new QToolButton(this);
    CommandWidget* cmd = new CommandWidget(this);
    setupNewTab(btn,cmd);
    selectTab(tabs.size()-1);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    model = new QStandardItemModel();

    connect(ui->add_bar,&QPushButton::clicked,this,&MainWindow::onAddTab);

    setupNewTab(ui->cmd_bar,ui->cmd_wdiget);
    selectTab(0);

    connect(ui->lineEdit, &QLineEdit::returnPressed,this,&MainWindow::onEditDone);

    connect(ui->clear_button,&QPushButton::clicked,this,&MainWindow::onClear);
    connect(ui->excute_button,&QPushButton::clicked,this,&MainWindow::onExcuteButton);
    connect(ui->restart_button,&QPushButton::clicked,this,&MainWindow::onRestart);

    connect(ui->delete_button,&QPushButton::clicked,this,&MainWindow::onDelete);
    connect(ui->move_up_button,&QPushButton::clicked,this,&MainWindow::onMoveUp);
    connect(ui->move_down_button,&QPushButton::clicked,this,&MainWindow::onMoveDown);
    connect(ui->sort_button,&QPushButton::clicked,this,&MainWindow::onSort);

    connect(ui->treeView,&QListView::doubleClicked,this,&MainWindow::onDoubleClick);
    connect(ui->treeView,&QListView::clicked,this,&MainWindow::onClick);
    // connect(ui->treeView,&QTreeView::expanded,[this](){readExpandedHeaders();});

    loadConfigFile();

    ui->treeView->setModel(model);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->add_bar->setIcon(QIcon(":/imgs/add"));

    this->setWindowFlags(Qt::FramelessWindowHint);

    ui->min_btn->setIcon(style()->standardPixmap(QStyle::SP_TitleBarMinButton));
    ui->max_btn->setIcon(style()->standardPixmap(QStyle::SP_TitleBarMaxButton));
    ui->close_btn ->setIcon(style()->standardPixmap(QStyle::SP_TitleBarCloseButton));
    ui->stay_top_btn ->setIcon(QIcon(":/imgs/stay_top"));

    connect(ui->close_btn,&QToolButton::clicked,this,&QMainWindow::close);
    connect(ui->min_btn,&QToolButton::clicked,this,&QMainWindow::showMinimized);
    connect(ui->max_btn,&QToolButton::clicked,[this](){
        if(this->isMaximized())
            this->showNormal();
        else this->showMaximized();
    });
    connect(ui->stay_top_btn,&QToolButton::clicked,[this](){
        static bool stay_top = false;
        if (!stay_top)
            // a bad experience that when switched stay on top, the window will disapear.
            this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
        else this->setWindowFlags(this->windowFlags() ^ Qt::WindowStaysOnTopHint);
        stay_top = !stay_top;
        this->show();
    });
    this->setWindowIcon(QIcon(":/imgs/icon"));
    expandHeaders();

}
void MainWindow::onResize(const QSize& size)
{
    auto& s = size;
    float cmd_window_w = s.width()*840.0/1134;

    ui->cmd_window->resize(cmd_window_w,s.height()-228);
    curWidget->resize(cmd_window_w,s.height()-228);
    curWidget->getTextEdit()->resize(cmd_window_w,s.height()-228);

    ui->treeView->resize(s.width()-ui->cmd_window->width()-32,s.height()-267);
    ui->treeView->move(ui->cmd_window->pos()+QPoint{10+ui->cmd_window->width(),0});
    ui->lineEdit->move({10,s.height()-118});
    ui->lineEdit->resize(s.width()-22,ui->lineEdit->height());
    ui->btn_frame->move({s.width()-474,s.height()-72});
    ui->top_widget->resize(s.width(),ui->top_widget->height());
    ui->frame_2->resize(s.width()*261/1134,ui->frame_2->height());
    ui->frame_2->move(cmd_window_w + 20,s.height()-(778-620));
    ui->tab_widget->resize(s.width()-22 ,ui->tab_widget->height());
    ui->horizontalLayout_2->setStretchFactor(ui->move_down_button,1);
    ui->horizontalSpacer_2->setGeometry(QRect(0,0,600,10));
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

//    QPainter painter(this);
//    painter.drawPixmap(rect(),QPixmap(":/imgs/bkg"),QRect());
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    onResize(e->size());
}

void MainWindow::updateView()
{
    readExpandedHeaders();
    model->clear();
    headers.clear();
    auto copy = commands;
    commands.clear();
    for(auto& command : copy)
    {
        addInstructions(command);
    }
    expandHeaders();
}

void MainWindow::mouseMoveEvent(QMouseEvent* e)
{
    if(ui->top_widget->underMouse()&&e->type()==QMouseEvent::MouseMove)
    {
        this->move(e->point(0).globalPosition().toPoint()-press_p);
    }
}

void MainWindow:: mousePressEvent(QMouseEvent* e)
{
    press_p = e->point(0).pressPosition().toPoint();
}

MainWindow::~MainWindow()
{
    dumpConfigFile();
    delete ui;
    delete model;
}

void MainWindow::addInstructions(const QString& _instruction)
{
    if(!check_valid(_instruction)||commands.indexOf(_instruction)!=-1)
    {
        return;
    }
    QString instruction = _instruction.trimmed();
    if(!instruction.startsWith("bat ") && instruction.endsWith(".bat"))
    {
        instruction = "bat "+ instruction;
    }
    QString head = instruction.split(' ')[0];
    if(!headers.count(head))
    {
        headers.append(head);
        QStandardItem* item = new QStandardItem(head);
        QBrush b;
        b.setColor(QColor(255,0,255));
        item->setBackground(b);
        item->setIcon(QIcon(":/imgs/folder"));

        item->appendRow(new QStandardItem(instruction));
        model->appendRow({item});
    }
    else
    {
        int idx = headers.indexOf(head);
        auto* ptr= model->item(idx,0);
        ptr->appendRow(new QStandardItem(instruction));
    }
    commands.append(instruction);
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
    for(auto id= ui->treeView->indexAt({0,0});id.isValid();id = ui->treeView->indexBelow(id))
    {
        if(ui->treeView->isExpanded(id))
        {
            expandedHeaders << model->itemFromIndex(id)->text();
        }
    }
}
void MainWindow::expandHeaders()
{
    for(auto id= ui->treeView->indexAt({0,0});id.isValid();id = ui->treeView->indexBelow(id))
    {
        const auto& header = model->itemFromIndex(id)->text();
        if(expandedHeaders.indexOf(header)!=-1)
        {
                ui->treeView->setExpanded(id,true);
        }
    }
}

void  MainWindow::onRestart()
{
    QProcess*& proc = curWidget->getProcess();
    curWidget->getTextEdit()->clear();
    if(proc&& proc->state()!=proc->NotRunning)
    {
        proc->terminate();
    }
    delete proc;
    proc = new QProcess;
    proc->start("cmd");

    curTab->setText("cmd");
    curTab->setToolTip("cmd");
    connect(proc,&QProcess::readyReadStandardOutput,curWidget,&CommandWidget::readyReadStandardOutputSlot);
    connect(proc,&QProcess::readyReadStandardError,curWidget,&CommandWidget::readyReadStandardErrorSlot);
}

void  MainWindow::onClear()
{
    curWidget->getTextEdit()->clear();
}

void  MainWindow::onDelete()
{
    auto index = ui->treeView->currentIndex();
    const QString& text = model->itemFromIndex(index)->text();
    if(!index.isValid())
        return;
    if(check_level1(index))
    {
        auto copy = commands;
        for(auto& command : commands)
        {
            if(command.startsWith(text))
            {
                copy.remove(copy.indexOf(command));
            }
        }
        commands = copy;
    }
    else
    {
        commands.removeAt(commands.indexOf(text));
    }
    updateView();
    dumpConfigFile();
}

void  MainWindow::onSort()
{
    QCollator co;
    std::sort(commands.begin(),commands.end(),co);
    updateView();
    dumpConfigFile();
}

void  MainWindow::onMoveDown()
{
    QModelIndex index = ui->treeView->currentIndex();
    if(!index.isValid()||check_level1(index)) return;

    QString text = model->itemFromIndex(index)->text();
    int the_index = commands.indexOf(text);
    if(index.isValid()&&the_index<commands.size()-1)
    {
        commands.swapItemsAt(the_index,the_index+1);

        dumpConfigFile();
        updateView();
        ui->treeView->setCurrentIndex(ui->treeView->indexBelow(index));
    }
}

void  MainWindow::onMoveUp()
{
    auto index = ui->treeView->currentIndex();
    if(!index.isValid()|| check_level1(index)) return;

    const QString& text = model->itemFromIndex(index)->text();
    int the_index = commands.indexOf(text);
    if(index.isValid()&&the_index>0)
    {
        commands.swapItemsAt(the_index,the_index-1);
        updateView();
        dumpConfigFile();
        ui->treeView->setCurrentIndex(ui->treeView->indexAbove(index));
    }
}

void MainWindow::exec(const QString& cmd)
{
    if(!check_valid(cmd))return;
    justExcuted=cmd;

    curTab->setText(cutTabTitle(cmd,12,10));
    curTab->setToolTip(cmd);
    if(!commands.count(cmd))
    {
        readExpandedHeaders();
        addInstructions(cmd);
        expandHeaders();
        dumpConfigFile();
        updateView();
    }
    if(cmd=="cls")
    {
        getCurrentEdit()->clear();
        curWidget->exec(cmd);
    }
    else if (cmd.startsWith("bat "))
    {
        curWidget->exec(cmd.sliced(4));
    }
    else curWidget->exec(cmd);
}


void MainWindow::onEditDone()
{
    const QString& str = ui->lineEdit->text().trimmed();
    if(str=="")return;
    ui->lineEdit->setText("");

    exec(str);
}

void MainWindow::onDoubleClick(const QModelIndex& index)
{
    if(!check_level1(index))
    {
        exec(model->itemFromIndex(index)->text());
    }
}

void MainWindow::onClick(const QModelIndex& index)
{
    if(!check_level1(index))
    {
        ui->lineEdit->setText(model->itemFromIndex(index)->text());
    }
}
