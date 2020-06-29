#include "widget.h"
#include "ui_widget.h"
#include <QDataStream>
#include <QMessageBox>
#include <QDateTime>
#include <QColorDialog>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

Widget::Widget(QWidget *parent, QString name)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    //初始化
    udpSocket = new QUdpSocket(this);
    //用户名获取
    uName = name;
    //端口号
    this->port = 9999;
    //绑定端口号 1 绑定端口号 2 绑定模式 共享地址|断开重连 一般这两个一起使用
    udpSocket->bind(this->port,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    //发送新用户进入
    sndMsg(UsrEnter);

    //点击发送按钮发送消息
    connect(ui->sendBtn,&QPushButton::clicked,[=](){
        sndMsg(Msg);
    });
    //监听别人发送的数据
    connect(udpSocket,&QUdpSocket::readyRead,this,&Widget::receiveMessage);
    //点击退出按钮 实现关闭窗口
    connect(ui->exitBtn,&QPushButton::clicked,[=](){
        this->close();
    });

    //////////////////////////辅助功能/////////////////////////////
    //字体
    connect(ui->fontCbs,&QFontComboBox::currentFontChanged,[=](const QFont &font){
        ui->msgTxtEdit->setCurrentFont(font);
        ui->msgTxtEdit->setFocus();
    });

    //字号
    void (QComboBox:: *size)(const QString &text) = &QComboBox::currentIndexChanged;
    connect(ui->sizeCbx,size,[=](const QString &text){
        ui->msgTxtEdit->setFontPointSize(text.toDouble());
        ui->msgTxtEdit->setFocus();

    });

    //加粗
    connect(ui->boldTBtn,&QToolButton::clicked,[=](bool isCheck){


        if(isCheck)
        {
           ui->msgTxtEdit->setFontWeight(QFont::Bold);
        }
        else {
            ui->msgTxtEdit->setFontWeight(QFont::Normal);
        }

    });

    //倾斜
    connect(ui->italicTBtn,&QToolButton::clicked,[=](bool check){

        ui->msgTxtEdit->setFontItalic(check);
    });

    //下划线
    connect(ui->underlineTBtn,&QToolButton::clicked,[=](bool check){
        ui->msgTxtEdit->setFontUnderline(check);
    });

    //字体颜色
    connect(ui->colorTBtn,&QToolButton::clicked,[=](){
        QColor color = QColorDialog::getColor(Qt::red);
        ui->msgTxtEdit->setTextColor(color);
    });

    //清空聊天记录
    connect(ui->clearTBtn,&QToolButton::clicked,[=](){
        ui->msgBrowser->clear();
    });

    //保存聊天记录
    connect(ui->saveTBtn,&QToolButton::clicked,[=](){
       if(ui->msgBrowser->document()->isEmpty())
       {
           QMessageBox::warning(this,"警告","聊天不能为空");
           return;
       }

       else {
           QString path = QFileDialog::getSaveFileName(this,"保存文件","聊天记录","(*.txt)");
           if (path.isEmpty())
           {
               QMessageBox::warning(this,"警告","路径不能为空");
               return;
           }
           QFile file(path);
           //打开模式加换行
           file.open(QIODevice::WriteOnly | QIODevice::Text); //text表换行
           QTextStream stream(&file);
           stream << ui->msgBrowser->toPlainText();
           file.close();
       }




    });

}

Widget::~Widget()
{
    delete ui;
}

void Widget::closeEvent(QCloseEvent *e)
{
    emit this->closeWidget();
    sndMsg(UsrLeft);
    //断开套接字
    udpSocket->close();
    udpSocket->destroyed();
    QWidget::closeEvent(e);
}

void Widget::sndMsg(Widget::MsgType type)
{
    //发送消息分为三种类型
    //发送的数据 做分段处理 第一段 类型 第二段 用户名 第三段 具体内容

    QByteArray array;
    QDataStream stream(&array,QIODevice::WriteOnly);
    stream << type << getUsr();//第一段 第二段 内容添加到流中
    switch (type) {
    case Msg:
        if(ui->msgTxtEdit->toPlainText() == "")
        {
            QMessageBox::warning(this,"警告","发送内容为空");
            return;
        }
        //第三段数据 基体说的话
        stream << getMsg();
        break;
    case UsrEnter:
        break;
    case UsrLeft:
        break;

    }
    //书写报文 广播发送
    udpSocket->writeDatagram(array,QHostAddress::Broadcast,port);
}

void Widget::usrEnter(QString usrName)
{
    //1.更新右侧tableWidget
    bool isEmpty = ui->usrTblWidget->findItems(usrName,Qt::MatchExactly).isEmpty();
    if(isEmpty)
    {
        QTableWidgetItem * usr = new QTableWidgetItem(usrName);
        //插入行
        ui->usrTblWidget->insertRow(0);
        ui->usrTblWidget->setItem(0,0,usr);

        //2.追加聊天记录
        ui->msgBrowser->setTextColor(Qt::gray);
        ui->msgBrowser->append(QString("%1上线了").arg(usrName));

        //3.在线人数更新
        ui->usrNumLbl->setText(QString("在线人数：%1人").arg(ui->usrTblWidget->rowCount()));
        //4.把自身信息广播除去
        sndMsg(UsrEnter);
    }

}

void Widget::usrLeft(QString usrname, QString time)
{
    //1.更新右侧tablewidget
    bool isEmpty = ui->usrTblWidget->findItems(usrname,Qt::MatchExactly).isEmpty();
    if(!isEmpty)
    {
        int row = ui->usrTblWidget->findItems(usrname,Qt::MatchExactly).first()->row();
        ui->usrTblWidget->removeRow(row);
        //2.追加聊天记录
        ui->msgBrowser->setTextColor(Qt::gray);
        ui->msgBrowser->append(QString("%1于%2离开").arg(usrname).arg(time));
        //3.在线人数更新
        ui->usrNumLbl->setText(QString("在线用户：%1人").arg(ui->usrTblWidget->rowCount()));

    }
}

QString Widget::getUsr()
{
    return this->uName;
}

QString Widget::getMsg()
{
    QString str = ui->msgTxtEdit->toHtml();
    ui->msgTxtEdit->clear();
    ui->msgTxtEdit->setFocus();
    return str;
}

void Widget::receiveMessage()
{
    //拿到数据报文
    //获取长度
    qint64 size = udpSocket->pendingDatagramSize();
    QByteArray array = QByteArray(size,0);
    udpSocket->readDatagram(array.data(),size);

    //解析数据  第一段 类型  第二段 用户名 第三代 内容
    QDataStream stream (&array,QIODevice::ReadOnly);
    int msgType;
    QString usrName;
    QString msg;
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    stream >> msgType;
    switch (msgType) {
    case Msg://普通聊天
        stream >> usrName >> msg;
        //追加聊天记录
        ui->msgBrowser->setTextColor(Qt::blue);
        ui->msgBrowser->append("[" + usrName + "]" + time);
        ui->msgBrowser->append(msg);
        break;
    case UsrEnter:
    {
        stream >> usrName;
        usrEnter(usrName);
    }
    case UsrLeft:
        stream >> usrName;
        usrLeft(usrName,time);
        break;
    default:
        break;
    }
}
