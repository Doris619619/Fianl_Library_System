

// main.cpp
#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QGuiApplication>
#include <QScreen>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QFile>

#include <seatui/launcher/login_window.hpp>
#include "ws/ws_hub.hpp"
#include <QHostAddress>

// 高分屏与位图设置（需在 QApplication 前设置）
static void initHiDpi() {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
}

// 全局样式（卡片风 + 主按钮 + 弹框与之统一）
static void applyGlobalStyle(QApplication& app) {
    // 字体（Windows 建议雅黑）
    QFont f("Microsoft YaHei UI");
    f.setPointSize(10);
    app.setFont(f);

    // 统一主题 QSS
    const char* qss = R"(
        QWidget { font-family: "Microsoft YaHei UI"; font-size: 10pt; }
        QMainWindow { background: #f5f7fa; }

        /* 卡片容器 */
        QFrame#card {
            background: #ffffff;
            border-radius: 16px;
            border: 1px solid #e6e8eb;
        }

        /* 标签提示色 */
        QLabel[hint="true"] { color: #8a8f98; }

        /* 行编辑 */
        QLineEdit {
            background: #ffffff;
            border: 1px solid #d7dbe3;
            border-radius: 10px;
            padding: 8px 12px;
            selection-background-color: #0ea5e9;
            selection-color: #ffffff;
        }
        QLineEdit:focus {
            border: 1px solid #0ea5e9;
            box-shadow: 0 0 0 3px rgba(14,165,233,0.15);
        }

        /* 主按钮 */
        QPushButton[type="primary"] {
            background: #0ea5e9;
            color: #ffffff;
            border: none;
            border-radius: 10px;
            padding: 10px 16px;
        }
        QPushButton[type="primary"]:hover { background: #0284c7; }
        QPushButton[type="primary"]:pressed { background: #0369a1; }

        /* 大卡片按钮（角色选择） */
        QPushButton[class="cardBtn"] {
            background: #ffffff;
            border: 1px solid #d7dbe3;
            border-radius: 14px;
            padding: 14px 18px;
            text-align: left;
        }
        QPushButton[class="cardBtn"]:hover {
            border-color: #0ea5e9;
            box-shadow: 0 4px 16px rgba(2,132,199,0.15);
        }

        /* 顶部条 */
        QFrame#header { background: #ffffff; border-bottom: 1px solid #e6e8eb; }

        /* —— 弹框与卡片风一致 —— */
        QMessageBox {
            background: #ffffff;
            border: 1px solid #e6e8eb;
            border-radius: 12px;
        }
        QMessageBox QLabel { color: #1f2937; min-width: 280px; }
        QMessageBox QPushButton {
            min-height: 32px;
            padding: 6px 12px;
            border-radius: 8px;
            border: 1px solid #d7dbe3;
            background: #ffffff;
        }
        QMessageBox QPushButton:hover {
            border-color: #0ea5e9;
            box-shadow: 0 2px 8px rgba(2,132,199,0.15);
        }
        QMessageBox QPushButton[type="primary"] {
            background: #0ea5e9; color: #ffffff; border: none;
        }
        QMessageBox QPushButton[type="primary"]:hover { background: #0284c7; }
        QMessageBox QPushButton[type="primary"]:pressed { background: #0369a1; }
    )";
    app.setStyleSheet(qss);
}

int main(int argc, char *argv[]) {
    initHiDpi();
    QApplication app(argc, argv);

    // ==== 第一步：确保数据库路径存在 ====
    QMessageBox::information(nullptr, "启动状态", "1. 检查数据库路径...");

    // 获取当前exe目录
    QString exeDir = QCoreApplication::applicationDirPath();
    qDebug() << "exe目录:" << exeDir;

    // 构建数据库路径：../output/seating_system.db
    QDir dir(exeDir);
    if (!dir.cdUp()) {
        QMessageBox::critical(nullptr, "错误", "无法访问上级目录");
        return -1;
    }

    QString outputDir = dir.path() + "/output";
    QString dbPath = outputDir + "/seating_system.db";

    qDebug() << "数据库目录:" << outputDir;
    qDebug() << "数据库文件:" << dbPath;

    // 创建output目录
    if (!QDir().mkpath(outputDir)) {
        QMessageBox::critical(nullptr, "错误", "无法创建目录: " + outputDir);
        return -1;
    }

    // 创建空的数据库文件（如果不存在）
    if (!QFile::exists(dbPath)) {
        QFile file(dbPath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(nullptr, "错误", "无法创建数据库文件");
            return -1;
        }
        file.close();
        qDebug() << "数据库文件已创建";
    }

    // ==== 第二步：初始化数据库 ====
    QMessageBox::information(nullptr, "启动状态", "2. 初始化数据库...");

    try {
        auto& db = SeatDatabase::getInstance(dbPath.toStdString());
        if (!db.initialize()) {
            QMessageBox::critical(nullptr, "数据库错误", "数据库初始化失败！\n请检查数据库文件是否被其他程序占用。");
            return -1;
        }
        QMessageBox::information(nullptr, "启动状态", "3. 数据库初始化成功");
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "异常", QString("数据库异常: %1").arg(e.what()));
        return -1;
    }

    // 建议改为 true：当最后一个窗口关闭时退出应用
    app.setQuitOnLastWindowClosed(true);

    applyGlobalStyle(app);

    // ==== 第三步：启动内置WebSocket服务 ====
    QMessageBox::information(nullptr, "启动状态", "4. 启动WebSocket服务...");

    // 创建WsHub对象，父对象设为app，这样生命周期随应用程序
    static WsHub* wsHub = new WsHub(&app);
    const quint16 port = 12345;

    if (!wsHub->start(port, QHostAddress::LocalHost)) {
        QMessageBox::warning(nullptr, "WebSocket警告",
                             QString("WebSocket服务启动失败（端口%1可能被占用），但程序将继续运行\n\n请检查是否有其他程序正在使用端口12345").arg(port));
        qWarning() << "[WS] Start failed on port" << port << ", try another port or close other process.";
    } else {
        QMessageBox::information(nullptr, "启动状态", "5. WebSocket服务启动成功");
        qDebug() << "[WS] WebSocket服务已启动，监听端口:" << port;
    }

    // —— 仅此处改变：一次创建两个"无角色限制"的登录窗口 ——
    QMessageBox::information(nullptr, "启动状态", "6. 创建登录窗口...");

    auto *login1 = new LoginWindow(nullptr);
    login1->setAttribute(Qt::WA_DeleteOnClose);
    login1->setWindowTitle(QStringLiteral("登录窗口 #1"));
    login1->show();

    auto *login2 = new LoginWindow(nullptr);
    login2->setAttribute(Qt::WA_DeleteOnClose);
    login2->setWindowTitle(QStringLiteral("登录窗口 #2"));
    login2->show();

    // 把两个窗口左右排开，测试更直观
    if (auto *scr = QGuiApplication::primaryScreen()) {
        const QRect r = scr->availableGeometry();
        const int w = 520;  // 按你的实际登录窗大小微调
        const int h = 380;
        login1->setGeometry(r.center().x() - w - 16, r.center().y() - h/2, w, h);
        login2->setGeometry(r.center().x() + 16,     r.center().y() - h/2, w, h);
    }

    QMessageBox::information(nullptr, "启动状态", "7. 窗口创建完成，进入主循环");

    return app.exec();
}
