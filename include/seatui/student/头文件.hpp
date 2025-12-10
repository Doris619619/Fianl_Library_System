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

#include <seatui/student/book_search.hpp>  // 新增：图书检索引擎



class QComboBox;
class QPushButton;
class QLabel;
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
    void gotoBookSearch();  // 缺少这个声明
    void onSearchBooks();              // 点击“搜索”的槽

private:
    // ===== 通用 =====
    QStackedWidget* pages = nullptr;                 // 右侧多页面容器
    QPushButton *btnDash = nullptr, *btnNav = nullptr, *btnHeat = nullptr;
    QPushButton *btnLive = nullptr;                  // 新增：座位实况按钮
    

    // ===== 导航页控件 =====
    QWidget*     navCanvas = nullptr;               // 地图画布占位（后续绘制路径）
    QComboBox*   destBox   = nullptr;               // 目标书架 A/B/C/D
    QPushButton* btnGen    = nullptr;               // 生成路径
    QPushButton* btnClear  = nullptr;               // 清除
    QLabel*      navStatus = nullptr;               // 状态提示

    // ===== 图书检索页控件 =====（新增）
    QPushButton  *btnBook = nullptr;   // 侧栏按钮：📚 图书查询
    QWidget      *bookPage = nullptr;  // 页面
    QLineEdit    *bookInput = nullptr; // 关键词输入（作者或书名）
    QPushButton  *bookSearchBtn = nullptr;
    QTableWidget *bookTable = nullptr; // 结果表
    QLabel       *bookHint = nullptr;  // “暂无此书籍”提示

    BookSearchEngine bookEngine;       // 文本解析与检索引擎


    // ===== 构建各页面 =====
    QWidget* buildDashboardPage();                  // 仪表盘主页
    QWidget* buildNavigationPage();                 // 导航页
    QWidget* buildHeatmapPage();                    // 热力图占位页
    QWidget* buildLivePage();                       // 新增：座位实况页
    QWidget *buildHelpPage();
    QWidget* buildBookSearchPage();    // 构建该页面

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

    // —— 一键求助：槽函数 —— //
    void onPickImage();
    void onSubmitHelp();
    void onResetHelp();

private:
    // —— WS 客户端 —— //
    void initWsClient();
    void wsSend(const QByteArray& utf8Json);
    QWebSocket* ws_ = nullptr;
    bool wsReady_ = false;
};


