
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QHostAddress>
#include <QSet>
#include <QHash>
#include <ws/ws_hub.hpp>
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
#include <unordered_map>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>


//Qt WebSocket Constructor Function

WsHub::WsHub(QObject* parent)
    : QObject(parent),
    server_(QStringLiteral("SeatUI-WS"), QWebSocketServer::NonSecureMode, this)//
{
    // Initialize the server_ member with parameters:
    // 1. Server name: "SeatUI-WS"
    // 2. Security mode: non-secure mode (no encryption)
    // 3. Parent object: this (current WsHub instance)
    tick_.setInterval(2000); // Set the timer interval to 2 seconds for periodic message pushing.
    connect(&tick_, &QTimer::timeout, this, &WsHub::onTickPush);
    //Whenever the tick_ (a QTimer object) times out, it automatically calls the onTickPush slot function in the current object (this, which is an instance of the WsHub class).
}

//Start server
bool WsHub::start(quint16 port, const QHostAddress& host) {//port: Port number，host: IP address to listen on
    if (!server_.listen(host, port)) {
        qWarning() << "[WS] Hub start failed on" << host.toString() << ":" << port;
        return false;
    }
    connect(&server_, &QWebSocketServer::newConnection, this, &WsHub::onNewConnection);
    tick_.start();  // Start the timer启动定时器
    return true;
}

/**

@brief Handles new WebSocket connections.

This function is triggered when a new client connects to the WebSocket server.

It performs the following steps:

Retrieves the newly connected socket object and adds it to the client list clients_.

Sets up the logic for processing incoming text messages (by connecting to the onSocketText() function via signals and slots).

Configures cleanup operations for when the client disconnects, including removing the connection from the client list and safely deleting the socket object.
*/


void WsHub::onNewConnection() {
    auto* socket = server_.nextPendingConnection();
    if (!socket) return;

    clients_ << socket;//equivalent to clients_.append(socket);//Add the socket to the clients_ container.

    connect(socket, &QWebSocket::textMessageReceived, this, [this, socket](const QString& message) {
        onSocketText(socket, message);
    });

    // Handle disconnection
    connect(socket, &QWebSocket::disconnected, this, [this, socket] {
        clients_.remove(socket);
        socket->deleteLater();
    });    
}

// Implement the interaction between the student client and admin.
void WsHub::onSocketText(QWebSocket* socket, const QString& message) {
    QJsonParseError error;
    auto document = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        // If JSON parsing fails or the message format is not an object, log a warning and return.
        qWarning() << "[WS] Invalid JSON received:" << error.errorString();
        return;
    }

    const auto obj = document.object();// Convert the JSON document into a JSON object for easier subsequent access.
    const auto type = obj.value("type").toString();

    if (type == "hello") {
        const auto role = obj.value("role").toString();
        roles_[socket] = role;

        qDebug() << "[WS] Client connected with role:" << role;

        // 回复确认
        QJsonObject reply;
        reply["type"] = "hello_ack";
        reply["role"] = role;
        reply["status"] = "ok";
        socket->sendTextMessage(QJsonDocument(reply).toJson());
    }
    else if (type == "student_help") {// If the message type is "student_help", it indicates that the student is requesting assistance.

        qDebug() << "[WS] Forwarding student_help to admin clients";
        bool forwarded = false;
        for (auto* client : clients_) {
            if (roles_.value(client) == "admin") {
                client->sendTextMessage(message);
                forwarded = true;
            }
        }

        if (forwarded) {// If successfully forwarded to the administrator, notify the student client.
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

    else {// If the message type is not "hello" or "student_help", log the unknown message type.
        qDebug() << "[WS] Received unknown message type:" << type;
    }
}



// Process the received JSON-formatted message and perform different actions
//based on the message type (e.g., "hello" or "student_help").
void WsHub::onTickPush() {

    auto& db = SeatDatabase::getInstance("D:/CSC3002/Library_System/out/seating_system.db");
    const auto seatStatusList = db.getCurrentSeatStatus();
    const auto basicStats     = db.getCurrentBasicStats();

    // 连续异常计数器
    static std::unordered_map<std::string, int> anomaly_streak;
    constexpr int OCCUPIED_STREAK = 6;

    qDebug() << "[WS] tick-raw:"
             << "seats=" << seatStatusList.size()
             << "total=" << basicStats.total_seats
             << "occ="   << basicStats.occupied_seats
             << "anom="  << basicStats.anomaly_seats
             << "rate="  << basicStats.overall_occupancy_rate;

    // Create a seat_update message
    QJsonObject root;//root is a QJsonObject instance. QJsonObject is a class provided by Qt for representing JSON objects.
    root["type"]      = "seat_update";
    root["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    //statsObj is another QJsonObject instance, specifically used to store statistical information related to seats.
    // 构建基础统计信息
    QJsonObject statsObj;
    statsObj["total_seats"]            = basicStats.total_seats;
    statsObj["occupied_seats"]         = basicStats.occupied_seats;
    statsObj["anomaly_seats"]          = basicStats.anomaly_seats;
    statsObj["overall_occupancy_rate"] = basicStats.overall_occupancy_rate;
    root["stats"] = statsObj;

    // 处理每个座位状态
    // seatsArray is a QJsonArray object used to store JSON information for all seats.
    QJsonArray seatsArray;

    for (const auto& s : seatStatusList) {

        int& streak = anomaly_streak[s.seat_id];// Store the exception count for each seat
        if (s.state == "Anomaly") {
            streak++;
        } else {
            streak = 0;
        }

        const bool has_person = (s.state == "Seated");
        const bool occupied = (streak >= OCCUPIED_STREAK);
        //Determine whether a seat is "occupied" based on the exception count.

        QJsonObject seatObj;//Store the information of a single seat.
        seatObj["id"]    = QString::fromStdString(s.seat_id);
        seatObj["state"] = QString::fromStdString(s.state);

        // Timestamp processing: Convert to UTC ISO format
        const QString last = QString::fromStdString(s.last_update);
        QDateTime dt = QDateTime::fromString(last, "yyyy-MM-dd HH:mm:ss");
        if (dt.isValid()) {
            dt.setTimeSpec(Qt::LocalTime);
            seatObj["last_update"] = dt.toUTC().toString(Qt::ISODate);
        } else {
            seatObj["last_update"] = last;
        }

        seatObj["has_person"]     = has_person;
        seatObj["occupied"]       = occupied;
        seatObj["anomaly_streak"] = streak;

        seatsArray.append(seatObj);
    }

    // 将 seat_array 添加到消息体
    root["seats"] = seatsArray;

    if (!seatsArray.isEmpty()) {
        qDebug() << "[WS] sample item:"
                 << QJsonDocument(seatsArray.at(0).toObject()).toJson(QJsonDocument::Compact);
    }

    // Broadcast the message
    broadcast(QJsonDocument(root).toJson());
}

//Send a message to all connected clients.
void WsHub::broadcast(const QString& message, const QString& onlyRole) {
    for (auto* client : clients_) {
        if (!onlyRole.isEmpty()) {
            if (roles_.value(client) != onlyRole) continue;
        }
        client->sendTextMessage(message);// If the condition is met, send a message to the current client.
    }
}

