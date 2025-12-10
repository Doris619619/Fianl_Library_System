
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QHostAddress>
#include <QSet>
#include <QHash>


#include <ws/ws_hub.hpp>
// 前置：数据库单例
#include "../db_core/SeatDatabase.h"


#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>


#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>



#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QHostAddress>
#include <QSet>
#include <QHash>

#include <unordered_map>


#include <ws/ws_hub.hpp>
// 前置：数据库单例
#include "../db_core/SeatDatabase.h"


#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>


#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>


//Qt WebSocket 服务端的构造函数，实现了一个周期性推送数据的功能

WsHub::WsHub(QObject* parent)
    : QObject(parent),
    server_(QStringLiteral("SeatUI-WS"), QWebSocketServer::NonSecureMode, this)
{
    tick_.setInterval(2000); // 2s 定时推送
    connect(&tick_, &QTimer::timeout, this, &WsHub::onTickPush);
}

bool WsHub::start(quint16 port, const QHostAddress& host) {
    if (!server_.listen(host, port)) {
        qWarning() << "[WS] Hub start failed on" << host.toString() << ":" << port;
        return false;
    }
    connect(&server_, &QWebSocketServer::newConnection, this, &WsHub::onNewConnection);
    tick_.start();  // 启动定时器
    return true;
}

void WsHub::onNewConnection() {
    auto* socket = server_.nextPendingConnection();
    if (!socket) return;

    clients_ << socket;

    // 处理连接后的消息
    connect(socket, &QWebSocket::textMessageReceived, this, [this, socket](const QString& message) {
        onSocketText(socket, message);
    });

    // 处理断开连接
    connect(socket, &QWebSocket::disconnected, this, [this, socket] {
        clients_.remove(socket);
        socket->deleteLater();
    });

    
}


/*
void WsHub::onSocketText(QWebSocket* socket, const QString& message) {
    QJsonParseError error;
    auto document = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError || !document.isObject()) return;

    const auto obj = document.object();
    const auto type = obj.value("type").toString();

    if (type == "hello") {
        const auto role = obj.value("role").toString();
        roles_[socket] = role;  // 保存连接角色
    }
}

*/

//实现学生端和admin交互
void WsHub::onSocketText(QWebSocket* socket, const QString& message) {
    QJsonParseError error;
    auto document = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "[WS] Invalid JSON received:" << error.errorString();
        return;
    }

    const auto obj = document.object();
    const auto type = obj.value("type").toString();

    if (type == "hello") {
        const auto role = obj.value("role").toString();
        roles_[socket] = role;  // 保存连接角色

        qDebug() << "[WS] Client connected with role:" << role;

        // 回复确认
        QJsonObject reply;
        reply["type"] = "hello_ack";
        reply["role"] = role;
        reply["status"] = "ok";
        socket->sendTextMessage(QJsonDocument(reply).toJson());
    }
    else if (type == "student_help") {
        // ✅ 转发学生求助消息给管理员端
        qDebug() << "[WS] Forwarding student_help to admin clients";

        bool forwarded = false;
        for (auto* client : clients_) {
            if (roles_.value(client) == "admin") {
                client->sendTextMessage(message);
                forwarded = true;
            }
        }

        if (forwarded) {
            qDebug() << "[WS] student_help forwarded to admin(s)";

            // 可选：给学生端一个确认
            QJsonObject ack;
            ack["type"] = "help_ack";
            ack["status"] = "sent_to_admin";
            ack["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
            socket->sendTextMessage(QJsonDocument(ack).toJson());
        } else {
            qWarning() << "[WS] No admin connected to forward student_help";
        }
    }
    else {
        qDebug() << "[WS] Received unknown message type:" << type;
    }
}

//这个是原先没有修改过的接收字段
/*
void WsHub::onTickPush() {
    // 获取当前座位状态
    auto seatStatusList = SeatDatabase::getInstance().getCurrentSeatStatus();

    // 构造 seat_snapshot 消息
    QJsonObject root;
    root["type"] = "seat_snapshot";
    root["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonArray items;
    for (const auto& seatStatus : seatStatusList) {
        QJsonObject seatObj;
        seatObj["seat_id"] = QString::fromStdString(seatStatus.seat_id);
        seatObj["state"] = QString::fromStdString(seatStatus.state);
        seatObj["since"] = QString::fromStdString(seatStatus.last_update);
        items.append(seatObj);
    }

    root["items"] = items;

    // 广播给所有连接的客户端
    broadcast(QJsonDocument(root).toJson());
}
*/







void WsHub::onTickPush() {
    // 获取当前座位状态
    auto& db = SeatDatabase::getInstance("D:/CSC3002/Library_System/out/seating_system.db");
    const auto seatStatusList = db.getCurrentSeatStatus();
    const auto basicStats     = db.getCurrentBasicStats();

    // 连续异常计数器
    static std::unordered_map<std::string, int> anomaly_streak;
    constexpr int OCCUPIED_STREAK = 6; // 连续 6 次 Anomaly => 占座

    qDebug() << "[WS] tick-raw:"
             << "seats=" << seatStatusList.size()
             << "total=" << basicStats.total_seats
             << "occ="   << basicStats.occupied_seats
             << "anom="  << basicStats.anomaly_seats
             << "rate="  << basicStats.overall_occupancy_rate;

    // 创建 seat_update 消息
    QJsonObject root;
    root["type"]      = "seat_update";
    root["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // 构建基础统计信息
    QJsonObject statsObj;
    statsObj["total_seats"]            = basicStats.total_seats;
    statsObj["occupied_seats"]         = basicStats.occupied_seats;
    statsObj["anomaly_seats"]          = basicStats.anomaly_seats;
    statsObj["overall_occupancy_rate"] = basicStats.overall_occupancy_rate;
    root["stats"] = statsObj;

    // 处理每个座位状态
    QJsonArray seatsArray;

    for (const auto& s : seatStatusList) {
        int& streak = anomaly_streak[s.seat_id];
        if (s.state == "Anomaly") {
            streak++;
        } else {
            streak = 0; // 任何非Anomaly状态重置计数器
        }

        const bool has_person = (s.state == "Seated");
        // 修改：只有连续6次及以上Anomaly才算占座
        const bool occupied = (streak >= OCCUPIED_STREAK);

        QJsonObject seatObj;
        seatObj["id"]    = QString::fromStdString(s.seat_id);
        seatObj["state"] = QString::fromStdString(s.state);

        // 时间戳处理：转换为 UTC ISO 格式
        const QString last = QString::fromStdString(s.last_update);
        QDateTime dt = QDateTime::fromString(last, "yyyy-MM-dd HH:mm:ss");
        if (dt.isValid()) {
            dt.setTimeSpec(Qt::LocalTime);
            seatObj["last_update"] = dt.toUTC().toString(Qt::ISODate);
        } else {
            seatObj["last_update"] = last; // 兜底
        }

        // 添加派生字段
        seatObj["has_person"]     = has_person;
        seatObj["occupied"]       = occupied;  // 只有连续6次及以上Anomaly才为true
        seatObj["anomaly_streak"] = streak;

        seatsArray.append(seatObj);
    }

    // 将 seat_array 添加到消息体
    root["seats"] = seatsArray;

    // 打印调试信息
    if (!seatsArray.isEmpty()) {
        qDebug() << "[WS] sample item:"
                 << QJsonDocument(seatsArray.at(0).toObject()).toJson(QJsonDocument::Compact);
    }

    // 广播消息
    broadcast(QJsonDocument(root).toJson());
}


void WsHub::broadcast(const QString& message, const QString& onlyRole) {
    for (auto* client : clients_) {
        if (!onlyRole.isEmpty()) {
            if (roles_.value(client) != onlyRole) continue;
        }
        client->sendTextMessage(message);
    }
}

// QString WsHub::makeSeatSnapshotJson() {
//     // 返回座位快照的 JSON
//     auto seatStatusList = SeatDatabase::getInstance().getCurrentSeatStatus();

//     QJsonObject root;
//     root["type"] = "seat_snapshot";
//     root["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

//     QJsonArray items;
//     for (const auto& seatStatus : seatStatusList) {
//         QJsonObject seatObj;
//         seatObj["seat_id"] = QString::fromStdString(seatStatus.seat_id);
//         seatObj["state"] = QString::fromStdString(seatStatus.state);
//         seatObj["since"] = QString::fromStdString(seatStatus.last_update);
//         items.append(seatObj);
//     }

//     root["items"] = items;
//     return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
// }


