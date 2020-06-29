#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
    enum MsgType {Msg, UsrEnter, UsrLeft};

public:
    Widget(QWidget *parent, QString name);
    ~Widget();
    //关闭事件
    void closeEvent(QCloseEvent *);

private:
    Ui::Widget *ui;
signals:
    //关闭窗口发送关闭信号
    void closeWidget();
public:
    void sndMsg(MsgType type);
    void usrEnter(QString usrname);//处理新用户的加入
    void usrLeft(QString usrname,QString time);//处理用户离开
    QString getUsr();//获取用户名
    QString getMsg();//获取聊天信息
private:
    QUdpSocket * udpSocket;//udp套接字
    qint16 port;
    QString uName;

    void receiveMessage();//接受UDP消息


};
#endif // WIDGET_H
