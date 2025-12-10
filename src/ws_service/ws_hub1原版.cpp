#include <ws/ws_hub.hpp>
#include "../db_core/SeatDatabase.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>

static int toStateNum(const std::string& s){
    if (s == "Seated")  return 1;
    if (s == "Anomaly") return 2;
    return 0; // Unseated/其他
}

WsHub::WsHub(QObject* parent):QObject(parent){
    server_ = new QWebSocketServer(QStringLiteral("SeatWS"), QWebSocketServer::NonSecureMode, this);
    tick_ = new QTimer(this);
    tick_->setInterval(2000); // 2s
    connect(tick_, &QTimer::timeout, this, &WsHub::onTickPush);
}

bool WsHub::start(quint16 port, const QHostAddress& host){
    if (!server_->listen(host, port)) return false;
    connect(server_, &QWebSocketServer::newConnection, this, &WsHub::onNewConnection);
    tick_->start();
    return true;
}

void WsHub::onNewConnection(){
    auto* c = server_->nextPendingConnection();
    clients_.insert(c);
    connect(c, &QWebSocket::disconnected, this, &WsHub::onClientDisconnected);
}

void WsHub::onClientDisconnected(){
    auto* c = qobject_cast<QWebSocket*>(sender());
    if (!c) return;
    clients_.remove(c);
    c->deleteLater();
}

void WsHub::onTickPush(){
    const QByteArray payload = makeSeatSnapshotJson();
    for (auto* c : clients_){
        c->sendTextMessage(QString::fromUtf8(payload));
    }
}

QByteArray WsHub::makeSeatSnapshotJson() const{
    using namespace std;
    auto& db = SeatDatabase::getInstance();                     // 单例
    const auto vec = db.getCurrentSeatStatus();                 // seat_id/state/last_update（每座位最新） :contentReference[oaicite:7]{index=7}

    QJsonObject root;
    root["type"] = QStringLiteral("seat_snapshot");
    root["ts"]   = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonArray items;
    for (const auto& it : vec){
        QJsonObject o;
        o["seat_id"] = QString::fromStdString(it.seat_id);
        o["state"]   = toStateNum(it.state);
        o["since"]   = QString::fromStdString(it.last_update);
        items.push_back(o);
    }
    root["items"] = items;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}
