#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDataStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 连接信号和槽
    connect(ui->postButton, &QPushButton::clicked, this, &MainWindow::onPostButtonClicked);
    connect(ui->replyButton, &QPushButton::clicked, this, &MainWindow::onReplyButtonClicked);

    // 创建网络连接
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);

    socket->connectToHost("127.0.0.1", 1234);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onConnected() {
    requestPosts();
}

void MainWindow::onReadyRead() {
    QByteArray response = socket->readAll();
    QDataStream in(response);

    int responseType;
    in >> responseType;

    if (responseType == 0) { // All posts
        ui->listWidget->clear();

        while (!in.atEnd()) {
            int postId;
            QString postContent;
            in >> postId >> postContent;
            QListWidgetItem *newItem = new QListWidgetItem("Post: " + postContent, ui->listWidget);
            ui->listWidget->addItem(newItem);

            while (true) {
                QString replyContent;
                in >> replyContent;

                if (replyContent.isEmpty()) break;  // 碰到空字符串时表示该帖子的回复结束

                newItem->setText(newItem->text() + "\n  Reply: " + replyContent);
            }
        }
    }
}


void MainWindow::onPostButtonClicked() {
    QString postContent = ui->postEdit->toPlainText();
    if (!postContent.isEmpty()) {
        QByteArray request;
        QDataStream out(&request, QIODevice::WriteOnly);
        out << (int)1 << postContent; // Request type: Post
        socket->write(request);
        socket->flush();
        ui->postEdit->clear();
    }
}

void MainWindow::onReplyButtonClicked() {
    QListWidgetItem *currentItem = ui->listWidget->currentItem();
    if (currentItem) {
        int postId = ui->listWidget->row(currentItem) + 1; // assuming posts are ordered by id
        QString replyContent = ui->replyEdit->toPlainText();
        if (!replyContent.isEmpty()) {
            QByteArray request;
            QDataStream out(&request, QIODevice::WriteOnly);
            out << (int)2 << postId << replyContent; // Request type: Reply
            socket->write(request);
            socket->flush();
            ui->replyEdit->clear();
        }
    }
}

void MainWindow::requestPosts() {
    QByteArray request;
    QDataStream out(&request, QIODevice::WriteOnly);
    out << (int)3; // Request type: Get all posts
    socket->write(request);
    socket->flush();
}

void MainWindow::on_pushButton_clicked()
{
    ui->listWidget->clear();
}

