#pragma once

#include <QMainWindow>
#include <QList>
#include <QVector>
#include <QtWebSockets/QWebSocket>

class QTabWidget;
class QTableWidget;
class QLabel;
class QWidget;
class QWebSocketServer;
class QWebSocket;

#include <QJsonObject>

class AdminWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit AdminWindow(QWidget* parent = nullptr);

private: // —— 页面构建 —— //
    QWidget* buildOverviewPage();
    QWidget* buildHelpCenterPage();
    QWidget* buildHeatmapPage();
    QWidget* buildStatsPage();
    QWidget* buildTimelinePage();

    // 新增：占座监控页
    QWidget* buildSeatMonitorPage();


private:
    void initWsClient();  // 初始化 WebSocket 客户端


private: // —— 逻辑/工具 —— //
    void appendHelpRow(const QString& when,
                       const QString& user,
                       const QString& text,
                       const QPixmap& thumb,
                       const QByteArray& rawImgBase64,
                       const QString& mime);
    void initWsServer();

    // 新增：占座监控相关
    //void initWsClient();   // 初始化并连接到 ws://127.0.0.1:12345
    void setSeatCell(const QString& id, int state, const QString& sinceIso);
    void onSeatEventJson(const QJsonObject& o);     // 单条 seat_event
    void onSeatSnapshotJson(const QJsonObject& o);  // 批量 seat_snapshot

private: // —— 成员 —— //
    QTabWidget*    tabs_ = nullptr;

    // 求助中心
    QTableWidget*  helpTable_ = nullptr;


    /*
    QWebSocketServer* wsServer_ = nullptr;
    QList<QWebSocket*> wsClients_;
    */

    // 占座监控
    QVector<QLabel*>  seatCells_;     // 2×2 网格中右下角的状态标签
    QTableWidget*     seatTable_ = nullptr; // 当前状态表
    QWidget*          seatPage_  = nullptr; // 页面根（可用于整体刷新/样式）

    // 新增：订阅座位快照的 WS 客户端（连接 ws_service）
    QWebSocket* ws_ = nullptr;
    bool wsReady_ = false;

private slots:
    void onHelpArrived(const QByteArray& utf8Json);
};
