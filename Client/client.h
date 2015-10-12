#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QMessageBox>
#include <QHostAddress>
#include <QStackedLayout>
#include <QListWidgetItem>
#include <QMenu>
#include <QCloseEvent>

#include "emojiframe.h"
#include "findcontacts.h"
#include "trayicon.h"


namespace Ui {
class Client;
}

class Client : public QMainWindow
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = 0);
    ~Client();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void getMessage();                  //Получение сообщений
    void on_sendMessage_clicked();      //Отправка сообщения
    void on_connect_button_clicked();   //Соединение к серверу
    void show_Error(QAbstractSocket::SocketError errorSocket);  //Их большая часть в коде -_-
    void send_personal_data();          //Отправка сведений о клиенте
    void onDisconnect();                //Отключение
    void sendUserCommand(QString command);
    void on_userSetting_clicked();      //Настройки (-)
    void on_close_setting_button_clicked();
    void whisperOnClick(QListWidgetItem* user);
    void showEmoji();
    void showFindCont();
    void on_pushButton_clicked();
    void on_newContact_Button_clicked();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void showHideWindow();
    void on_actionShowHideWindow_triggered();
    void on_actionExit_triggered();

private:
    Ui::Client *ui;

    TrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QStackedLayout *layout;
    QTcpSocket *tcpSocket;
    quint16 blockSize;
    QString getIP();
    EmojiFrame *frameEmoji;
    findcontacts *findcont;

    bool personDates;
    bool sethide;

};

#endif // CLIENT_H
