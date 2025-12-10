#pragma once
#include <QMainWindow>
#include <QTextEdit>
#include <QtWebSockets/QWebSocket>

#include <QPushButton>
#include <QLabel>
#include <QVector>  // 添加QVector包含
#include <QMainWindow>

#include <QLineEdit>

#include <QTableWidget>
#include <QStackedWidget>
#include <QTimer>
#include <seatui/student/book_search.hpp>  // 新增：图书检索引擎

class QComboBox;
class QWidget;
class QStackedWidget;

class StudentWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit StudentWindow(QWidget* parent = nullptr);

signals:
    // 侧边栏"← 返回登录"被点击时发出，交由上层（RoleSelector/Main）处理切回登录
    void backToLoginRequested();

private slots://增加槽位
    // 导航页
    void onGenerate();   // 生成路径
    void onClear();      // 清除
    // 侧边栏"返回登录"
    void onBackToLogin();

    // 页面切换
    void gotoDashboard();
    void gotoNavigation();
    void gotoHeatmap();
    void gotoHelp();     // 已经声明了
    void gotoLive();     // 新增：切换到座位实况页
    void gotoBookSearch(); // 新增：切换到图书查询页（缺少！）
    void onSearchBooks(); // 点击"搜索"的槽

    // 一键求助相关（缺少！）
    void onPickImage();
    void onSubmitHelp();
    void onResetHelp();

private:
    // ===== 通用 =====
    QStackedWidget* pages = nullptr;                 // 右侧多页面容器
    QPushButton *btnDash = nullptr, *btnNav = nullptr, *btnHeat = nullptr;
    QPushButton *btnLive = nullptr;                  // 新增：座位实况按钮
    QPushButton *btnBook = nullptr;                  // 图书查询按钮

    // ===== 导航页控件 =====
    QWidget*     navCanvas = nullptr;               // 地图画布占位（后续绘制路径）
    QComboBox*   destBox   = nullptr;               // 目标书架 A/B/C/D
    QPushButton* btnGen    = nullptr;               // 生成路径
    QPushButton* btnClear  = nullptr;               // 清除
    QLabel*      navStatus = nullptr;               // 状态提示

    // ===== 图书检索页控件 =====
    QWidget      *bookPage = nullptr;  // 页面
    QLineEdit    *bookInput = nullptr; // 关键词输入（作者或书名）
    QPushButton  *bookSearchBtn = nullptr;
    QTableWidget *bookTable = nullptr; // 结果表
    QLabel       *bookHint = nullptr;  // "暂无此书籍"提示

    BookSearchEngine bookEngine;       // 文本解析与检索引擎

    // ===== 构建各页面 =====
    QWidget* buildDashboardPage();                  // 仪表盘主页
    QWidget* buildNavigationPage();                 // 导航页
    QWidget* buildHeatmapPage();                    // 热力图占位页
    QWidget* buildLivePage();                       // 新增：座位实况页
    QWidget* buildHelpPage();                       // 一键求助页
    QWidget* buildBookSearchPage();                 // 图书搜索页（缺少声明！）

    // ===== 座位实况页相关 =====
    QWidget*     livePage = nullptr;                // 座位实况页面容器
    QVector<QLabel*> liveCells_;                    // 4 个小格中右下角状态文本标签

    // 渲染单个座位格子（文字+配色）
    void liveSetCell(const QString& id,             // 座位ID
                     int state,                     // 0=没人 1=有人 2=占座(有物无人)
                     const QString& sinceIso);      // 时间戳

    // —— 一键求助：成员 —— //
    QPushButton *btnHelp = nullptr;
    QTextEdit  *helpText_  = nullptr;
    QLabel     *helpImgPreview_ = nullptr;
    QPushButton *helpPickBtn_ = nullptr;
    QPushButton *helpSubmitBtn_ = nullptr;
    QPushButton *helpResetBtn_ = nullptr;

    QByteArray helpImgBytes_;    // PNG/JPEG 原始字节
    QString    helpImgFilename_; // 原始文件名
    QString    helpImgMime_;     // "image/png" ...

private:
    // --- 新增：番茄钟 UI 控件 ---
    QPushButton* btnPomo = nullptr;      // 侧边栏按钮
    QLabel* pomoTimeLabel = nullptr;     // 倒计时显示
    QPushButton* pomoStartBtn = nullptr; // 开始/暂停按钮
    QLabel* pomoStatusLabel = nullptr;   // 状态文字

    // --- 新增：番茄钟 逻辑变量 ---
    QTimer* pomoTimer = nullptr;         // 定时器
    int pomoRemainingSec = 1500;         // 剩余秒数
    bool isPomoRunning = false;          // 是否正在运行
    bool isPomoWorkState = true;         // 工作还是休息

    // --- 新增：构建页面函数 ---
    QWidget* buildPomodoroPage();

private slots:
    // --- 新增：槽函数 ---
    void gotoPomodoro(); // 切换页面
    void onPomoTick();   // 每秒触发
    void onPomoToggle(); // 开始/暂停
    void onPomoReset();  // 重置

private:
    // —— WS 客户端 —— //
    void initWsClient();
    void wsSend(const QByteArray& utf8Json);
    QWebSocket* ws_ = nullptr;
    bool wsReady_ = false;
};
