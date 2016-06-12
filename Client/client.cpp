#include "client.h"
#include "ui_client.h"
#include <QDebug>
#include <QPixmap>
#include <QImage>
#include <QIcon>
#include <QFileDialog>
#include <QString>
#include <QPainter>
#include <time.h>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

#define DEFAULT_LANGUAGE QString("EN_Language")


QString gl_fname;

Client::Client(QWidget *parent) : QMainWindow(parent), download_path("(C:/...)"), personDates(false),ui(new Ui::Client)
{
    srand((time(NULL)));
    ui->setupUi(this);


    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);

    frameEmoji      = new EmojiFrame();
    emojiMan        = new EmojiManager();
    notice          = new Notification();
    authorization   = new Authorization();
    trayIcon        = new TrayIcon(this);
    stackchat       = new QStackedWidget;
    trayIconMenu    = new QMenu(this);
    tcpSocket       = new QTcpSocket(this);
    rsaCrypt        = new RSACrypt;
    set_language    = new XML_Language();


    QColor defaulcolor(Qt::white);
    colorchat = defaulcolor.name();
    ui->RB_sendEnter->setChecked(true);
    ui->userList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->stackedWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->userList->setItemDelegate(new UserListDelegate(ui->userList));
    ui->search_list->setItemDelegate(new UserListDelegate(ui->search_list));
    ui->label_6->setText(QDir::homePath() + "/Whisper/");
    ui->widget_2->hide();
    ui->search_list->hide();
    ui->glass_button->raise();
    ui->glass_button->hide();
    set_default_Language();

    nextBlockSize = 0;
    FriendCount = 0;

    trayIconMenu->addAction(ui->actionShowHideWindow);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(ui->actionExit);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->hide();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    connect(authorization, SIGNAL(sendData(QString, QString, QString)), this, SLOT(recieveData(QString, QString, QString)));
    connect(frameEmoji, SIGNAL(sendEmoji(QString)), this, SLOT(insertEmoticon(QString)));

    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(getMessage()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(show_Error(QAbstractSocket::SocketError)));
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(send_personal_data()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(onDisconnect()));

    connect(ui->userSetting_button, SIGNAL(clicked()), this, SLOT(on_userSetting_clicked()));
    connect(ui->close_setting_button_2, SIGNAL(clicked()), this, SLOT(on_close_setting_button_clicked()));
    connect(ui->userList_3, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(whisperOnClick(QListWidgetItem*)));
    connect(ui->userList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(whisperOnClickUsers(QListWidgetItem*)));
    connect(ui->userList, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(whisperOnClickSelectUsers(QListWidgetItem*)));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->stackedWidget_2, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(showContextMenuForChat(const QPoint &)));
    connect(ui->userList, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(showContextMenuForWidget(const QPoint &)));

}

void Client::keyReleaseEvent(QKeyEvent *event)
{
    if(ui->RB_sendEnter->isChecked())
    {
        switch(event->key()) {
        case Qt::Key_Escape:
            // Обработка нажатия Esc
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if(ui->widget_2->isHidden() && ui->RB_sendEnter->isChecked())
                on_sendMessage_clicked();
            break;
            // Обработчики других клавиш
        }
    }

    if(ui->RB_send_CEnter->isChecked())
        if(event->modifiers()==Qt::ControlModifier)
            if(event->key() == Qt::Key_Return)
                if(ui->widget_2->isHidden() && ui->RB_send_CEnter->isChecked())
                    on_sendMessage_clicked();
}

void Client::recieveData(QString str, QString pas, QString pubKey)
{
    if(str!="" && pas!="")
    {
        name = str;
        name.replace(" ", "_");
        ui->usernameEdit->setText(name);

        QDir keyDir = QDir::homePath() + "/WhisperServer/Whisper Close Key/";
        QLineEdit m_ptxtMask("*.txt");
        QStringList listFiles = keyDir.entryList(m_ptxtMask.text().split(" "), QDir::Files);
        myPublicKey = pubKey;

        for(int i=0; i<listFiles.size(); i++)
            if(listFiles.at(i)==name+".txt")
            {
                QFile myKey(QDir::homePath() + "/WhisperServer/Whisper Close Key/" + name + ".txt");
                myKey.open(QIODevice::ReadOnly);
                myPrivateKey = myKey.readAll();
                break;
            }
        qDebug() << "Закрытый ключ: " << myPrivateKey;
        QString hostname = "127.0.0.1";
        quint32 port = 55155;
        tcpSocket->abort();
        tcpSocket->connectToHost(hostname, port);

        this->show();
        QPropertyAnimation* animation = new QPropertyAnimation(this, "windowOpacity");
        animation->setDuration(1500);
        animation->setStartValue(0);
        animation->setEndValue(1);
        animation->start();

        trayIcon->show();
    }
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

void Client::on_sendMessage_clicked()
{
    QRegExp rx("<a [^>]*name=\"([^\"]+)\"[^>]*>");      // Регулярные выражения для парсинга смайликов в сообщении
    QString str = ui->editText->text();                 // Получение сообщения для отправки
    QString message = str;


    QStringList list;
    QString encodemsg;

    list.clear();
    int pos = 0;

    while ((pos = rx.indexIn(str, pos)) != -1) {
        list << rx.cap(1);
        pos += rx.matchedLength();
    }

    for (int i = 0; i < list.count(); i++) {
        QString emojiNumber = list.at(i);
        QString searchTag = QString("<a name=\"%1\"></a>").arg(emojiNumber);
        QString replTag = emojiMan->getEmojiSymbolFromNumber(emojiNumber);
        str = str.replace(searchTag, replTag);
    }

    QTextDocument doc;
    doc.setHtml(ui->textEdit1->toPlainText());
    doc.toPlainText();

    ui->editText->clear();
    blockSize = 0;

    if (!message.isEmpty() && vec.size()!=0)              //Отправка сообщений
    {
        if(ui->ChBox_PSound->isChecked())
            QSound::play(":/new/prefix1/Resource/to.wav");

        QString RecName = vec.at(ui->userList->currentRow())->data(Qt::DisplayRole).toString();
        QStringList lst = myPublicKey.split(" ", QString::SkipEmptyParts);
        QString MyMsg = rsaCrypt->encodeText(message, lst.at(0).toInt(), lst.at(1).toInt());

        for(int i=0; i<pubFriendKey.size(); i++)
        {
            if(RecName==pubFriendKey.at(i).first)
            {
                QString user_key = pubFriendKey.at(i).second;
                qDebug() << "Ключ друга: " << pubFriendKey.at(i).first << user_key;
                QStringList _key = user_key.split(" ", QString::SkipEmptyParts);
                encodemsg = rsaCrypt->encodeText(message, _key.at(0).toInt(), _key.at(1).toInt());
                qDebug() << "Отправил: " << encodemsg;
                break;
            }
        }

        QString new_mes = "/msg " + vec.at(ui->userList->currentRow())->data(Qt::DisplayRole).toString() + " " + encodemsg;
        sendUserCommand(new_mes, MyMsg);
    }
}

void Client::insertEmoticon(QString symbol)
{
    QTextCursor cursor(ui->textEdit1->textCursor());

    if (!cursor.isNull()) {
        cursor.insertHtml(symbol);
    }

    QTextCursor cursor2(ui->textEdit1->document()->find(symbol));

    QString emojiNumber = emojiMan->getEmojiNumberFromSymbol(symbol);
    QString binDir = QCoreApplication::applicationDirPath();
    QString dataDir = binDir;
    dataDir = QDir::cleanPath(dataDir + "/");

    QDir iconsLocation(dataDir + "/data/icons");


    QString s = QString("%1/x-%2.png")
            .arg(iconsLocation.absolutePath())
            .arg(emojiNumber);

    if (!cursor2.isNull()) {
        QString imgTag = QString("<img src=\"%1\" id=\"%2\" />")
                .arg(s)
                .arg(emojiNumber);

        cursor2.insertHtml(imgTag);
    }
}

void Client::AddUser_Chat(QString _username, QString _sex, QList<QPair<QString, QString>> lst, int count)
{
    int rand_avatar;
    QString sex = _sex;

    // В зависимости от пола рандомно устаналиваю аватар из списка.

    if(sex=="Man")
        rand_avatar = rand()%18+1;
    else if(sex=="Woman")
        rand_avatar = rand()%13+19;
    else if(sex=="Unknown")
        rand_avatar = rand()%20+32;

    QIcon pic(":/Avatars/Resource/Avatars/"+QString::number(rand_avatar)+".jpg");


    // Проверка на повторное добавление человека в список - выходим.
    if (!vec.empty())
        for(int i=0; i<vec.size(); i++)
            if(vec.at(i)->data(Qt::DisplayRole)==_username)
                return;

    //Создание Списка на каждую страницу стека, для отдельного отображения чат переписки и через делегат управляю свойством Item'a

    QListWidgetItem *item = new QListWidgetItem();
    QListWidget *chatlist = new QListWidget();
    chatlist->setItemDelegate(new ChatListDelegate(chatlist, colorchat));
    ui->stackedWidget_2->addWidget(chatlist);

    // Добавление нового пользователя через Поиск.
    if (count==-1 || count == -2)
    {
        item->setData(Qt::DisplayRole, _username);
        item->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));
        if (count==-2)
        {   QListWidgetItem *item2 = new QListWidgetItem();
            item->setData(Qt::UserRole + 1, lan_dict.value("Ad_User"));
            item2->setData(Qt::UserRole + 1, "FROM");
            item2->setData(Qt::DisplayRole, lan_dict.value("Invate_msg"));
            item2->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));
            chatlist->addItem(item2);
        }
        else
            item->setData(Qt::UserRole + 1, "New User");
        item->setData(Qt::DecorationRole, pic);

        vec.push_back(item);
        chatvec.push_back(chatlist);
        ui->userList->addItem(item);
        ui->userList->scrollToBottom();

        if(count==-2)
        {
            ui->start_textBrowser->hide();
            ui->stackedWidget_2->show();
            ui->stackedWidget_2->setCurrentIndex(chatvec.size()-1);
            ui->userList->clearSelection();
            vec.at(vec.size()-1)->setSelected(true);
            whisperOnClickUsers(vec.at(vec.size()-1));
        }
        return;
    }

    // Добавление пользователя(друга) из БД
    item->setData(Qt::DisplayRole, _username);
    item->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));
    item->setData(Qt::DecorationRole, pic);
    vec.push_back(item);
    chatvec.push_back(chatlist);

    // Загрузка Вашей переписки.
    for(int i=0; i<lst.size(); i++)
    {
        QListWidgetItem *item2 = new QListWidgetItem();
        QString msg = lst.at(i).first;
        QString timeStr = (lst.at(i).first);
        timeStr.remove(14, lst.at(i).first.size());

        msg.remove(0,14);
        QString TFile = msg.left(4);

        if(lst.at(i).second.left(1) == "F")
        {
            QStringList lstfrom = myPrivateKey.split(" ", QString::SkipEmptyParts);

            if(msg.left(1)=="F")
            {

            }
            else
                msg = rsaCrypt->decodeText(msg, lstfrom.at(0).toInt(), lstfrom.at(1).toInt());

            if(TFile=="FILE")
            {
                msg.remove(0,5);
                QString fsize = lst.at(i).second;
                QIcon pic(":/new/prefix1/Resource/sendfiles.png");
                item2->setData(Qt::ToolTipRole, fsize.remove(0,5) +  "  " +timeStr);
                item2->setData(Qt::UserRole + 1, "FROMF");
                item2->setData(Qt::DecorationRole, pic);
                item->setData(Qt::UserRole + 1, lan_dict.value("File") + ": " + msg);
            }
            else
            {
                item2->setData(Qt::UserRole + 1, "FROM");
                item2->setData(Qt::ToolTipRole, timeStr);
                item->setData(Qt::UserRole + 1, msg);
            }

            item2->setData(Qt::DisplayRole, msg);
            item->setData(Qt::ToolTipRole, timeStr);

        }
        else if (lst.at(i).second.left(1) == "T")
        {


            if(msg.left(1)=="F")
            {

            }
            else{
                QStringList _lst = myPrivateKey.split(" ", QString::SkipEmptyParts);
                msg = rsaCrypt->decodeText(msg, _lst.at(0).toInt(), _lst.at(1).toInt());
            }


            if(TFile=="FILE")
            {
                msg.remove(0,5);
                QString fsize = lst.at(i).second;
                QIcon pic(":/new/prefix1/Resource/sendfiles.png");
                item2->setData(Qt::ToolTipRole, fsize.remove(0,3) + "  " + timeStr);
                item2->setData(Qt::UserRole + 1, "TOF");
                item2->setData(Qt::DecorationRole, pic);
            }
            else
            {
                item2->setData(Qt::UserRole + 1, "TO");
                item2->setData(Qt::ToolTipRole, timeStr);
            }

            item2->setData(Qt::DisplayRole, msg);
        }

        chatvec.at(count)->addItem(item2);
    }
    chatvec.at(count)->scrollToBottom();
    ui->userList->clearSelection();
    ui->stackedWidget_2->hide();
    ui->userList->addItem(item);

}

void Client::getMessage_UserList(PairStringList &PublicFriendKey, PairStringList &FriendList, ChatListVector &chatList)
{
    for(int i=0; i<PublicFriendKey.size(); i++)
        pubFriendKey.push_back(qMakePair(PublicFriendKey.at(i).first, PublicFriendKey.at(i).second));

    for(int i=0; i<FriendList.size(); i++)
        AddUser_Chat(FriendList.at(i).first, FriendList.at(i).second, chatList.at(i).second, i);
}

void Client::getMessage()
{
    QString message;
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_5_4);

    if (!nextBlockSize) {
        if (quint32(tcpSocket->bytesAvailable()) < sizeof(quint32)) {
            return;
        }
        in >> nextBlockSize;
    }
    if (tcpSocket->bytesAvailable() < nextBlockSize) {
        return;
    }

    in >> message;
    QStringList commandList;
    QString fromname;

    enum class COMMAND { NONE, USERLIST, FINDUSER, INVITE, GETFILE, USINFO};
    COMMAND cmd = COMMAND::NONE;

    if (message=="FRLST")
        cmd = COMMAND::USERLIST;

    else if(message=="_GetFILE_")
        cmd = COMMAND::GETFILE;

    else if(message=="_USINFO_")
        cmd = COMMAND::USINFO;

    else
    {
        commandList = message.split(" ", QString::SkipEmptyParts);
        if(commandList.at(1)==name)
            return;
        fromname = commandList.at(1);

        QStringRef checkCmd(&message, 0, 5);        //Создание строки из заданного диапозона пришедшего сообщения
        if (checkCmd == "_FIN_")
            cmd = COMMAND::FINDUSER;
        if (checkCmd == "_INV_")
            cmd = COMMAND::INVITE;
    }

    switch (cmd)
    {

    case COMMAND::USERLIST:
    {
        PairStringList PublicFriendKey;
        PairStringList FriendList;
        ChatListVector chatList;

        in >> PublicFriendKey >> FriendList >> chatList;
        getMessage_UserList(PublicFriendKey, FriendList, chatList);
        break;
    }

    case COMMAND::FINDUSER:
    {
        QString find_user = commandList.at(1);

        if(find_user=="OKFIN" && gl_fname!=name)
        {
            QString pubKey = commandList.at(3) + " " + commandList.at(4);
            pubFriendKey.push_back(qMakePair(gl_fname, pubKey));
            QList <QPair<QString, QString> > a;
            AddUser_Chat(gl_fname, commandList.at(2), a , -1);
            findcont->~findcontacts();
            on_glass_button_clicked();
        }
        else
            findcont->SetErrorLayout(1);
        //qDebug() << "Поиск:(Ключ друга) " << pubFriendKey;
        break;
    }

    case COMMAND::INVITE:
    {
        QList <QPair<QString, QString> > a;
        QString pubKey = commandList.at(3) + " " + commandList.at(4);
        pubFriendKey.push_back(qMakePair(commandList.at(1), pubKey));
        AddUser_Chat(commandList.at(1), commandList.at(2), a , -2);
        //qDebug() << "Инвайт:(Ключ друга) " << pubFriendKey;
        break;
    }

    case COMMAND::USINFO:
    {
        QStringList *usData = new QStringList;
        QStringList UData;
        in >> UData;
        for(int i=0; i<4; i++)
            usData->push_back(UData.at(i));

        users_info = new UsersGroupInfo(this, usData);
        users_info->setGeometry(this->x()+365, this->y()+70, 330, 480);
        setGlass();
        users_info->show();
        break;
    }

    case COMMAND::GETFILE:
    {
        QByteArray buffer;
        QString filename;
        qint64 fileSize;

        QString dirDownloads = ui->label_6->text();
        QDir(dirDownloads).mkdir(dirDownloads);

        in >> fromname >> filename >> fileSize;

        QThread::sleep(2);
        forever
        {
            if (!nextBlockSize) {

                if (quint32(tcpSocket->bytesAvailable()) < sizeof(quint32)) {
                    break;
                }
                in >> nextBlockSize;
            }
            in >> buffer;
            if (tcpSocket->bytesAvailable() < nextBlockSize) {
                break;
            }
        }

        QFile receiveFile(dirDownloads + filename);
        receiveFile.open(QIODevice::ReadWrite);
        receiveFile.write(buffer);
        receiveFile.close();
        buffer.clear();

        QIcon pic(":/new/prefix1/Resource/sendfiles.png");
        if (!vec.empty())
        {
            for(int i=0; i<vec.size(); i++)
                if(vec.at(i)->data(Qt::DisplayRole)==fromname)
                {
                    QListWidgetItem *item = new QListWidgetItem();
                    item->setData(Qt::UserRole + 1, "FROMF");
                    item->setData(Qt::DisplayRole, filename);
                    item->setData(Qt::ToolTipRole, QString::number((float)fileSize/1024, 'f', 2)  + " KB  " +  QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));
                    item->setData(Qt::DecorationRole, pic);
                    vec.at(i)->setData(Qt::UserRole + 1, lan_dict.value("File") + ": " + filename);
                    vec.at(i)->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));

                    if(!this->isVisible() && ui->ChBox_Notif->isChecked())
                    {
                        notice->setPopupText(lan_dict.value("notice_msg") + " (" + fromname + "):\n" + lan_dict.value("File") + ": " + filename);
                        notice->show();
                    }

                    chatvec.at(i)->addItem(item);
                    chatvec.at(i)->scrollToBottom();
                    vec.at(i)->setSelected(true);
                    whisperOnClickUsers(vec.at(i));
                    ui->start_textBrowser->hide();
                    ui->stackedWidget_2->show();
                    ui->stackedWidget_2->setCurrentIndex(i);

                    break;
                }
        }
        nextBlockSize = 0;

        break;
    }
    default:                                                // Получение обычного текстового сообщения. Звук и добавление в ЧатЛист

        if(QStringRef(&message, 0, 3)=="*To")
        {
            QString myMsg = message.remove(0, 6+fromname.size());
            QStringList lst = myPrivateKey.split(" ", QString::SkipEmptyParts);
            myMsg = rsaCrypt->decodeText(myMsg, lst.at(0).toInt(), lst.at(1).toInt());

            QListWidgetItem *item = new QListWidgetItem();
            item->setData(Qt::UserRole + 1, "TO");
            item->setData(Qt::DisplayRole, myMsg);
            item->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));

            chatvec.at(ui->stackedWidget_2->currentIndex())->addItem(item);
            chatvec.at(ui->stackedWidget_2->currentIndex())->scrollToBottom();

        }
        else
        {
            if(ui->ChBox_PSound->isChecked())
                QSound::play(":/new/prefix1/Resource/from.wav");

            fromname.chop(1);
            if (!vec.empty())
            {
                for(int i=0; i<vec.size(); i++)
                    if(vec.at(i)->data(Qt::DisplayRole)==fromname)
                    {
                        QString decodmsg = message.remove(0, 9+fromname.size());
                        qDebug() << "Принял: " << decodmsg;
                        QStringList lst = myPrivateKey.split(" ", QString::SkipEmptyParts);
                        decodmsg = rsaCrypt->decodeText(decodmsg, lst.at(0).toInt(), lst.at(1).toInt());

                        QListWidgetItem *item = new QListWidgetItem();
                        item->setData(Qt::UserRole + 1, "FROM");
                        item->setData(Qt::DisplayRole, decodmsg);
                        item->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));

                        vec.at(i)->setData(Qt::UserRole + 1, decodmsg);
                        vec.at(i)->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));

                        if(!this->isVisible())
                        {
                            notice->setPopupText(lan_dict.value("notice_msg") + " (" + fromname + "):\n" + decodmsg);
                            notice->show();
                        }
                        chatvec.at(i)->addItem(item);
                        chatvec.at(i)->scrollToBottom();
                        vec.at(i)->setSelected(true);
                        whisperOnClickUsers(vec.at(i));
                        ui->start_textBrowser->hide();
                        ui->stackedWidget_2->show();
                        ui->stackedWidget_2->setCurrentIndex(i);

                        break;
                    }
                // Если пользователь не из списка (удален ранее), добавить заного в список.
                // Сделать попозже. Добавить в текущей беседе и вновь в БД на сервере.

                //QList <QPair<QString, QString> > a;
                //AddUser_Chat(fromname, commandList.at(2), a , -1);
                //qDebug() << fromname << message;
            }
        }
    }
}


void Client::whisperOnClickUsers(QListWidgetItem* user)
{
    ui->editText->clear();
    ui->editText->setFocus();
}

void Client::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() && Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void Client::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void Client::show_Error(QAbstractSocket::SocketError errorSocket)
{
    switch (errorSocket)
    {
    case QAbstractSocket::RemoteHostClosedError:
        QMessageBox::information(this, tr("W-H-I-S-P-E-R"),
                                 lan_dict.value("Disconnected_1"));
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("W-H-I-S-P-E-R"),
                               lan_dict.value("ServerNF"));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("W-H-I-S-P-E-R"),
                                 lan_dict.value("Disconnected_2"));
        break;
    default:
        QMessageBox::information(this, tr("W-H-I-S-P-E-R"),
                                 lan_dict.value("Occurred_err") + QString("%1.").arg(tcpSocket->errorString()));
    }
}

void Client::send_personal_data()
{
    if(!personDates)
    {
        personDates = true;

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);

        QString command = "_USR_";
        QString UserName = ui->usernameEdit->text();
        out << quint32(0) << QTime::currentTime() << command << UserName ;

        tcpSocket->write(block);
        authorization->close();
    }
}

void Client::onDisconnect()
{
    personDates = false;
}

void Client::sendUserCommand(QString Command, QString myMessage)
{
    QByteArray msg;
    QDataStream out(&msg, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);

    out << quint32(0) << QTime::currentTime() << QString("_UCD_") << Command << myMessage;
    tcpSocket->write(msg);
}

void Client::on_userSetting_clicked()
{
    ui->widget_2->show();
}

void Client::on_close_setting_button_clicked()
{
    ui->userList_3->clearFocus();
    ui->userList_3->clearSelection();
    ui->stackedWidget->setCurrentIndex(0);
    ui->widget_2->hide();
}

void Client::whisperOnClick(QListWidgetItem* User)
{
    QStringList lst_options;
    lst_options = lan_dict.value("userList_3").split(',');

    QString section = User->text();
    if (section == lst_options.at(0))
        ui->stackedWidget->setCurrentIndex(0);
    else if(section==lst_options.at(1))
        ui->stackedWidget->setCurrentIndex(1);
    else if (section == lst_options.at(2))
        ui->stackedWidget->setCurrentIndex(2);
    else
        ui->stackedWidget->setCurrentIndex(3);
}

void Client::on_pushButton_clicked()
{
    showEmoji();
}

void Client::showEmoji()
{
    QPoint p = QCursor::pos();
    frameEmoji->setGeometry(p.x()-250, p.y() -300, 300, 250);
    frameEmoji->show();

}

void Client::setGlass()
{
    QPropertyAnimation* animation = new QPropertyAnimation(ui->glass_button);
    QGraphicsOpacityEffect* grEffect = new QGraphicsOpacityEffect(ui->glass_button);
    ui->glass_button->setGraphicsEffect(grEffect);

    animation->setTargetObject(grEffect);
    animation->setPropertyName("opacity");
    animation->setDuration(1000);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start();
    ui->glass_button->show();
}

void Client::showFindCont()
{
    QPoint p = QCursor::pos();
    findcont->setGeometry(p.x()-200, p.y()-230, 310, 350);
    findcont->show();

    connect(findcont, SIGNAL(findUsers(QString)), this, SLOT(findtoserv(QString)));
}

void Client::findtoserv(QString name_user)
{
    gl_fname = name_user;
    static bool tmp = true;

    if(name==gl_fname)
    {
        findcont->SetErrorLayout(0);
        return;
    }

    for(int i=0; i<vec.size(); i++)
        if (vec.at(i)->data(Qt::DisplayRole)== gl_fname)
        {
            findcont->SetErrorLayout(2);
            return;
        }

    if(tmp)
    {
        QByteArray msg;
        QDataStream out(&msg, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);

        out << quint32(0) << QTime::currentTime() << QString("_FND_") << name_user << name;
        tcpSocket->write(msg);
        tmp=false;
    }
    else    // Костылек :)
        tmp=true;
}

void Client::on_newContact_Button_clicked()
{
    choice_window = new ChoiceCreate(this);
    choice_window->set_lang(lan_dict);
    choice_window->setGeometry(this->x()+359, this->y()+130, 343, 220);
    setGlass();
    choice_window->show();
    connect(choice_window, SIGNAL(choice(QString)), this, SLOT(choice_Window(QString)));

}

void Client::choice_Window(QString str)
{
    if(str == "close")
        on_glass_button_clicked();

    else if(str == "newContact")
    {
        findcont = new findcontacts();
        findcont->set_lang(lan_dict);
        showFindCont();
    }
    else if (str == "newGroup")
    {
        create_group = new CreateGroup(this);
        create_group->set_lang(lan_dict);
        showCreateGroup();
    }
}

void Client::showCreateGroup()
{
    QPoint p = QCursor::pos();
    create_group->setGeometry(this->x()+330, this->y()+130, 400, 230);
    create_group->show();

    connect(create_group, SIGNAL(GroupSignal(QString, QString, QString, QString)), SLOT(getCreateGroupSig(QString, QString, QString, QString)));
}

void Client::getCreateGroupSig(QString state, QString group_name, QString group_descr, QString path_avatar)
{

    if(state=="Close"){
        on_glass_button_clicked();

    }
    else if(state == "Create")
    {
        selectContacts = new SelectContacts(this, ui->userList);
        selectContacts->set_lang(lan_dict);
        selectContacts->setGeometry(this->x()+350, this->y()+40, 361, 540);
        selectContacts->show();

        groupData.push_back(group_name);
        groupData.push_back(group_descr);
        groupData.push_back(path_avatar);

        connect(selectContacts, SIGNAL(SelectUsersSignal(QStringList, QString)), SLOT(addGroup_toList(QStringList, QString)));
    }
}

void Client::addGroup_toList(QStringList userList, QString state)
{
    if(state == "Cancel")
    {
        if(!groupData.isEmpty())
            groupData.clear();
        on_glass_button_clicked();
    }

    else if(state == "Create")
    {
        QListWidgetItem *item = new QListWidgetItem();
        QListWidget *chatlist = new QListWidget();
        chatlist->setItemDelegate(new ChatListDelegate(chatlist, colorchat));
        ui->stackedWidget_2->addWidget(chatlist);

        QIcon pic(":/new/prefix1/Resource/group-1.png");
        item->setData(Qt::DisplayRole, groupData.at(0));
        item->setData(Qt::StatusTipRole, "group");
        item->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));
        item->setData(Qt::UserRole + 1, groupData.at(1));
        item->setData(Qt::DecorationRole, pic);

        QListWidgetItem *item2 = new QListWidgetItem();
        item2->setData(Qt::UserRole + 1, "TO");
        item2->setData(Qt::DisplayRole, lan_dict.value("NewGroup_msg") + " \"" + groupData.at(0) + "\"");
        item2->setData(Qt::ToolTipRole, QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));
        chatlist->addItem(item2);

        vec.push_back(item);
        chatvec.push_back(chatlist);
        ui->userList->addItem(item);
        ui->userList->scrollToBottom();

        ui->start_textBrowser->hide();
        ui->stackedWidget_2->show();
        ui->stackedWidget_2->setCurrentIndex(chatvec.size()-1);
        ui->userList->clearSelection();
        vec.at(vec.size()-1)->setSelected(true);
        whisperOnClickUsers(vec.at(vec.size()-1));
        on_glass_button_clicked();

        QByteArray msg;
        QDataStream out(&msg, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);

        out << quint32(0) << QTime::currentTime() << QString("_NEWGROUP_") << groupData.at(0) << groupData.at(1) <<  userList;
        tcpSocket->write(msg);

    }
}

void Client::on_actionShowHideWindow_triggered()
{
    showHideWindow();
}

void Client::on_actionExit_triggered()
{
    QApplication::exit();
}

void Client::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        showHideWindow();
        break;
    case QSystemTrayIcon::Unknown:
        break;
    case QSystemTrayIcon::Context:
        break;
    case QSystemTrayIcon::DoubleClick:
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        break;
    }
}

void Client::showHideWindow()
{
    if (this->isVisible()) {
        hide();
        ui->actionShowHideWindow->setText(tr("Restore"));
    }
    else {
        show();
        ui->actionShowHideWindow->setText(tr("Hide"));
    }
}

void Client::closeEvent(QCloseEvent *event)
{
    showHideWindow();
    event->ignore();
}


void Client::on_PB_SelColor_clicked()
{
    QColor color = QColorDialog::getColor(Qt::black, this, "Text Color",  QColorDialog::DontUseNativeDialog);
    colorchat = color.name();

    QPixmap pixmap(16,16);
    QPainter painter;
    painter.begin(&pixmap);
    painter.drawRect(0,0,16,16);
    painter.fillRect(0,0,16,16,QBrush(QColor(color)));
    painter.end();
    ui->imageLabel->setPixmap(pixmap);
    ui->chat_back_lab->setPixmap(pixmap);
}

void Client::on_PB_LoadFileBackground_clicked()
{
    QString files = QFileDialog::getOpenFileName(this, lan_dict.value("SelectImages"), "" , tr("Images (*.jpg *jpeg *.png)"));

    if(QString::compare(files, QString())!=0)
    {
        QImage image;
        bool vol = image.load(files);

        if(vol)
        {
            ui->chat_back_lab->setPixmap(QPixmap::fromImage(image));
            ui->imageLabel->setPixmap(QPixmap::fromImage(image));
        }
        else
        {
            //Error
        }
    }
}

void Client::on_radioButton_2_clicked()
{
    QString files = QFileDialog::getOpenFileName(this, lan_dict.value("SelectImages"), "" , tr("Images (*.jpg *jpeg *.png)"));

    if(QString::compare(files, QString())!=0)
    {
        QImage image;
        bool vol = image.load(files);

        if(vol)
        {
            ui->avatar_label->setPixmap(QPixmap::fromImage(image));
            ui->avatar_label->setFixedSize(120,120);
        }
        else
        {
            //Error
        }
    }
}

void Client::on_radioButton_clicked()
{
    QImage image(":/Avatars/Resource/Avatars/5.jpg");
    ui->avatar_label->setPixmap(QPixmap::fromImage(image));
    ui->avatar_label->setFixedSize(120,120);
}

void Client::on_Download_path_PB_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, lan_dict.value("SelectImages"), "");

    if(QString::compare(path, QString())!=0)
    {
        download_path = path;
        ui->label_6->setText(path);
    }
    else
        ui->label_6->setText(download_path);
}


void Client::on_pushButton_2_clicked()
{
    if(ui->stackedWidget_2->isHidden())
        return;

    QString filePatch = QFileDialog::getOpenFileName(this, lan_dict.value("fileWindow"), "", QObject::trUtf8("(*.*)"));

    if (filePatch.isEmpty())
        return;

    QByteArray  arrBlock;
    qint64 fileSize;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);

    sendFile = new QFile(filePatch);
    sendFile->open(QFile::ReadOnly);

    QFileInfo fi(filePatch);
    QString fileName = fi.fileName();

    fileSize = fi.size();

    QByteArray buffer = sendFile->readAll();
    QString receiver_name = vec.at(ui->userList->currentRow())->data(Qt::DisplayRole).toString();

    out << quint32(0) << QTime::currentTime() << QString("_FILE_") << receiver_name
        << fileName << fileSize << buffer;

    out.device()->seek(0);
    out << quint32(arrBlock.size() - sizeof(quint32));

    tcpSocket->write(arrBlock);

    QIcon pic(":/new/prefix1/Resource/sendfiles.png");
    int pos;
    QListWidgetItem *item = new QListWidgetItem();
    item->setData(Qt::UserRole + 1, "TOF");

    if(fileName.size()>55)
    {
        for(int i=fileName.size()-1; i>=0; i--)
            if(fileName.at(i) == '.')
            {
                pos = i;
                //fileName.remove(28, pos);
                QString newfilename = fileName.left(50)+"...."+fileName.right(fileName.size()-pos);
                fileName = newfilename;
                break;
            }
    }

    item->setData(Qt::DisplayRole, fileName);
    item->setData(Qt::ToolTipRole, QString::number((float)fileSize/1024, 'f', 2)  + " KB  " + QDateTime::currentDateTime().toString("dd.MM.yy hh:mm"));
    item->setData(Qt::DecorationRole, pic);
    chatvec.at(ui->stackedWidget_2->currentIndex())->addItem(item);
    chatvec.at(ui->stackedWidget_2->currentIndex())->scrollToBottom();
}


void Client::on_userList_clicked(const QModelIndex &index)
{
    if (!vec.empty())
    {
        ui->start_textBrowser->hide();
        ui->stackedWidget_2->show();
        ui->stackedWidget_2->setCurrentIndex(index.row());
    }
}

void Client::on_pushButton_3_clicked()
{
    this->close();
}

void Client::on_pushButton_5_clicked()
{
    aboutdialog = new AboutDialog(this);
    aboutdialog->setGeometry(this->x()+340, this->y()+70, 380, 480);
    setGlass();
    aboutdialog->show();
}

void Client::clearHistory()
{
    // Очистить историю.
    // Пока сеансово, добавть подтверждение и запрос в БД на очистку переписки.
    conf_message = new ConfirmWindow(this, lan_dict.value("conf_message_del_hist"));
    conf_message->set_lang(lan_dict);
    conf_message->setGeometry(this->x()+355, this->y()+100, 350, 170);
    setGlass();
    conf_message->show();

    connect(conf_message, SIGNAL(response(QString)), this, SLOT(clearCurHistory(QString)));
}

void Client::clearCurHistory(QString cmd)
{
    if(cmd=="OK")
    {
        chatvec.at(ui->stackedWidget_2->currentIndex())->clear();

        QByteArray msg;
        QDataStream out(&msg, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);

        QString to = vec.at(ui->userList->currentRow())->data(Qt::DisplayRole).toString();
        out << quint32(0) << QTime::currentTime() << QString("_CLNHISTORY_") << name << to;
        tcpSocket->write(msg);
    }
    on_glass_button_clicked();
    conf_message->~ConfirmWindow();
}

void Client::clearCurUser(QString cmd)
{
    if(cmd=="OK")
    {
        // Если есть контакты в списке, удалить нужного.
        if(!ui->userList->size().isEmpty())
        {
            QByteArray msg;
            QDataStream out(&msg, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_4);

            QString delfriend = vec.at(ui->userList->currentRow())->data(Qt::DisplayRole).toString();
            out << quint32(0) << QTime::currentTime() << QString("_DELFRIEND_") << name << delfriend;
            tcpSocket->write(msg);

            //Вытаскивание виджета из Стека Виджетов >_<
            QWidget* widget = ui->stackedWidget_2->widget(ui->userList->currentRow());
            // и удалить
            ui->stackedWidget_2->removeWidget(widget);

            // Очистить вектора
            vec.erase(vec.begin()+ui->userList->currentRow());
            chatvec.erase(chatvec.begin()+ui->userList->currentRow());
            ui->userList->removeItemWidget(ui->userList->takeItem(ui->userList->currentRow()));
            update();
        }
    }

    on_glass_button_clicked();
    conf_message->~ConfirmWindow();

}
void Client::ClearSelect()
{
    // Очистить выделенные сообщения
    chatvec.at(ui->stackedWidget_2->currentIndex())->clearSelection();
}

void Client::showContextMenuForChat(const QPoint &pos)
{
    // Контекстное меню для чата.

    if(ui->stackedWidget_2->count()==0 || ui->stackedWidget->isHidden() || chatvec.at(ui->stackedWidget_2->currentIndex())->count()==0)
        return;

    QPoint newPos = pos;
    newPos.setX(pos.x()+385);
    newPos.setY(pos.y()+30);

    QMenu * menu = new QMenu(this);
    QAction * deleteDevice = new QAction(lan_dict.value("ContextClearHistory"), this);
    QAction * delSelect = NULL;
    menu->addAction(deleteDevice);

    // Если есть выделенные сообщения, добавить в меню возможность скипнуть.
    if(ui->stackedWidget_2->count()!=0)
        if(!chatvec.at(ui->stackedWidget_2->currentIndex())->selectedItems().isEmpty())
        {
            delSelect = new QAction(lan_dict.value("ContextClearSelection"), this);
            menu->addAction(delSelect);
        }

    menu->popup(mapToGlobal(newPos));
    connect(delSelect, SIGNAL(triggered()), this, SLOT(ClearSelect())); // Обработчик удаления записи
    connect(deleteDevice, SIGNAL(triggered()), this, SLOT(clearHistory()));
}

void Client::showContextMenuForWidget(const QPoint &pos)
{
    // Контекстное меню для списка контактов.

    if(ui->userList->count()==0 || ui->userList->selectedItems().isEmpty())
        return;

    QPoint newPos = pos;
    newPos.setX(pos.x());
    newPos.setY(pos.y()+45);

    QMenu * menu = new QMenu(this);
    QAction * deleteDevice = new QAction(lan_dict.value("ContextDelUser"), this);
    connect(deleteDevice, SIGNAL(triggered()), this, SLOT(DeleteUser())); // Обработчик удаления записи
    menu->addAction(deleteDevice);
    menu->popup(mapToGlobal(newPos));
}
void Client::set_default_Language()
{
    if(!lan_dict.isEmpty())
        lan_dict.clear();

    lan_dict = set_language->parseXML(DEFAULT_LANGUAGE);
    set_lang();
    authorization->set_lang(lan_dict);
}

void Client::set_lang()
{
    ui->language->setText(lan_dict.value(ui->language->objectName()));
    ui->down_path->setText(lan_dict.value(ui->down_path->objectName()));
    ui->RB_sendEnter->setText(lan_dict.value(ui->RB_sendEnter->objectName()));
    ui->RB_send_CEnter->setText(lan_dict.value(ui->RB_send_CEnter->objectName()));
    ui->ChBox_Notif->setText(lan_dict.value(ui->ChBox_Notif->objectName()));
    ui->ChBox_PSound->setText(lan_dict.value(ui->ChBox_PSound->objectName()));
    ui->background->setText(lan_dict.value(ui->background->objectName()));
    ui->PB_SelColor->setText(lan_dict.value(ui->PB_SelColor->objectName()));
    ui->PB_LoadFileBackground->setText(lan_dict.value(ui->PB_LoadFileBackground->objectName()));
    ui->username_label->setText(lan_dict.value(ui->username_label->objectName()));
    ui->radioButton->setText(lan_dict.value(ui->radioButton->objectName()));
    ui->radioButton_2->setText(lan_dict.value(ui->radioButton_2->objectName()));
    ui->groupBox->setTitle(lan_dict.value(ui->groupBox->objectName()));
    ui->search_line_edit->setPlaceholderText(lan_dict.value(ui->search_line_edit->objectName()));
    ui->editText->setPlaceholderText(lan_dict.value(ui->editText->objectName()));
    ui->start_sys->setText(lan_dict.value(ui->start_sys->objectName()));
    //ui->start_textBrowser->setText(lan_dict.value(ui->start_textBrowser->objectName()));

    QStringList lst_setting;
    lst_setting = lan_dict.value("userList_3").split(',');

    for (int i=0; i<lst_setting.size(); i++)
        ui->userList_3->item(i)->setText(lst_setting[i]);

}

void Client::DeleteUser()
{
    // И запрос на сервер в БД, удалить из таблицы.
    // Хз что пока делать на стороне удаляемого, вдруг он будет писать, а его уже нет =(((

    conf_message = new ConfirmWindow(this, lan_dict.value("conf_message_del_user"));
    conf_message->set_lang(lan_dict);
    conf_message->setGeometry(this->x()+355, this->y()+100, 350, 170);
    setGlass();
    conf_message->show();

    connect(conf_message, SIGNAL(response(QString)), this, SLOT(clearCurUser(QString)));
}


// Поиск по контакт - листу. Вызывается при изменении текст. поля.
void Client::on_search_line_edit_textChanged(const QString &arg1)
{
    // Если запрос пуст, удаление предыдщуих результатов поиска + скрытие виджета.
    if(arg1.isEmpty()){
        ui->search_list->clear();
        ui->search_list->hide();
        return;
    }


    // Если список контактов не пуст, нахожу вхождение поиск-запроса в имена друзей.
    if(!vec.isEmpty())
    {
        ui->search_list->show();
        ui->search_list->clear();
        for(int i=0; i<vec.size(); i++)
            if(vec.at(i)->data(Qt::DisplayRole).toString().contains(arg1, Qt::CaseInsensitive))
            {
                // Во временный список добавляю элементы, которые удовлетворяют поиску(вхождение подстроки)
                // Переписать: не копировать все элементы, а брать их по ссылке.
                QListWidgetItem *item = new QListWidgetItem();
                item->setData(Qt::DisplayRole, vec.at(i)->data(Qt::DisplayRole).toString());
                item->setData(Qt::ToolTipRole, vec.at(i)->data(Qt::ToolTipRole).toString());
                item->setData(Qt::UserRole + 1, vec.at(i)->data(Qt::UserRole + 1).toString());
                item->setData(Qt::DecorationRole, vec.at(i)->data(Qt::DecorationRole));
                ui->search_list->addItem(item);
            }
    }
}

// Выбор контакта из поискового запроса (списка)
void Client::on_search_list_clicked(const QModelIndex &index)
{
    for(int i=0; i<vec.size(); i++)
    {
        if(vec.at(i)->data(Qt::DisplayRole).toString() == index.data(Qt::DisplayRole).toString())
        {
            // Переключение на выбранного человека/чат. Очищение запроса поиска + скрытие результата.
            ui->start_textBrowser->hide();
            ui->stackedWidget_2->show();
            ui->stackedWidget_2->setCurrentIndex(i);
            ui->userList->setCurrentRow(i);
            ui->search_line_edit->clear();
            ui->search_line_edit->clearFocus();
            ui->search_list->hide();
            ui->search_list->clear();
            return;
        }
    }
}


void Client::on_glass_button_clicked()
{
    ui->glass_button->hide();
}

void Client::on_info_user_button_clicked()
{
    if(ui->stackedWidget_2->count()==0 || ui->stackedWidget->isHidden())
        return;

    QByteArray msg;
    QDataStream out(&msg, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);

    QString name_user = vec.at(ui->userList->currentRow())->data(Qt::DisplayRole).toString();
    out << quint32(0) << QTime::currentTime() << QString("_USERINFO_") << name_user;
    tcpSocket->write(msg);
}

void Client::on_select_language_box_currentIndexChanged(int index)
{
    lan_dict.clear();

    switch (index) {
    case 0:
    {
        lan_dict = set_language->parseXML(DEFAULT_LANGUAGE);
        break;
    }
    case 1:
    {
        lan_dict = set_language->parseXML("FR_Language");
        break;
    }
    case 2:
    {
        lan_dict = set_language->parseXML("DE_Language");
        break;
    }
    case 3:
    {
        lan_dict = set_language->parseXML("RU_Language");
        break;
    }
    }

    set_lang();
}


Client::~Client()
{
    delete ui;
}

