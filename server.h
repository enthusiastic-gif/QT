#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>

class Server : public QTcpServer {
    Q_OBJECT

public:
    Server(QObject *parent = nullptr);
    ~Server();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onClientDisconnected();

private:
    void processRequest(QTcpSocket *clientSocket, const QByteArray &request);
    void handlePostRequest(QTcpSocket *clientSocket, const QString &postContent);
    void handleReplyRequest(QTcpSocket *clientSocket, int postId, const QString &replyContent);
    void sendAllPosts(QTcpSocket *clientSocket);
    //void clearDatabase();

    QSqlDatabase db;
};

#endif // SERVER_H
