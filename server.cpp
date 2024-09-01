#include "server.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDataStream>

Server::Server(QObject *parent) : QTcpServer(parent) {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("forum.db");

    if (!db.open()) {
        qCritical() << "Unable to open database:" << db.lastError();
        return;
    }

    //clearDatabase();

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS posts (id INTEGER PRIMARY KEY AUTOINCREMENT, content TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS replies (id INTEGER PRIMARY KEY AUTOINCREMENT, post_id INTEGER, content TEXT)");

    if (!listen(QHostAddress::Any, 1234)) {
        qCritical() << "Unable to start server:" << errorString();
    } else {
        qDebug() << "Server started on port" << serverPort();
    }

}

Server::~Server() {
    db.close();
}

/*void Server::clearDatabase() {
    QSqlQuery query;
    QSqlDatabase db = QSqlDatabase::database();  // 获取当前数据库连接

    // 开始事务
    if (!db.transaction()) {
        qWarning() << "Failed to start transaction:" << db.lastError().text();
        return;
    }

    // 逐步删除记录，按照外键关系的依赖顺序
    // 1. 删除依赖表（子表）的记录
    if (!query.exec("DELETE FROM replies")) {
        qWarning() << "Failed to delete from replies:" << query.lastError().text();
        db.rollback();  // 回滚事务
        return;
    }

    // 2. 删除主表的记录
    if (!query.exec("DELETE FROM posts")) {
        qWarning() << "Failed to delete from posts:" << query.lastError().text();
        db.rollback();  // 回滚事务
        return;
    }

    // 提交事务
    if (!db.commit()) {
        qWarning() << "Failed to commit transaction:" << db.lastError().text();
        return;
    }

    qDebug() << "Database cleared successfully.";
}
*/
void Server::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onClientDisconnected);

    qDebug() << "Client connected:" << socketDescriptor;
}

void Server::onReadyRead() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    QByteArray request = clientSocket->readAll();

    processRequest(clientSocket, request);
}



void Server::onClientDisconnected() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    clientSocket->deleteLater();
    qDebug() << "Client disconnected";
}

void Server::processRequest(QTcpSocket *clientSocket, const QByteArray &request) {
    QDataStream in(request);
    int requestType;
    in >> requestType;

    if (requestType == 1) { // Post
        QString postContent;
        in >> postContent;
        handlePostRequest(clientSocket, postContent);
    } else if (requestType == 2) { // Reply
        int postId;
        QString replyContent;
        in >> postId >> replyContent;
        handleReplyRequest(clientSocket, postId, replyContent);
    } else if (requestType == 3) { // Get all posts
        sendAllPosts(clientSocket);
    }
}

void Server::handlePostRequest(QTcpSocket *clientSocket, const QString &postContent) {
    QSqlQuery query;
    query.prepare("INSERT INTO posts (content) VALUES (:content)");
    query.bindValue(":content", postContent);
    if (!query.exec()) {
        qCritical() << "Failed to insert post:" << query.lastError();
        return;
    }
    sendAllPosts(clientSocket);
}

void Server::handleReplyRequest(QTcpSocket *clientSocket, int postId, const QString &replyContent) {
    QSqlQuery query;
    query.prepare("INSERT INTO replies (post_id, content) VALUES (:post_id, :content)");
    query.bindValue(":post_id", postId);
    query.bindValue(":content", replyContent);
    if (!query.exec()) {
        qCritical() << "Failed to insert reply:" << query.lastError();
        return;
    }
    sendAllPosts(clientSocket);
}


void Server::sendAllPosts(QTcpSocket *clientSocket) {
    QSqlQuery query;
    query.exec("SELECT id, content FROM posts");

    QByteArray response;
    QDataStream out(&response, QIODevice::WriteOnly);
    out << (int)0; // Response type: all posts

    while (query.next()) {
        int postId = query.value(0).toInt();
        QString postContent = query.value(1).toString();
        out << postId << postContent;

        // 获取每个帖子的回复
        QSqlQuery replyQuery;
        replyQuery.prepare("SELECT content FROM replies WHERE post_id = :post_id");
        replyQuery.bindValue(":post_id", postId);
        replyQuery.exec();

        while (replyQuery.next()) {
            QString replyContent = replyQuery.value(0).toString();
            out << replyContent;
        }

        // 标记回复结束
        out << QString(); // 添加一个空字符串作为回复结束标记
    }

    clientSocket->write(response);
    clientSocket->flush();
}
