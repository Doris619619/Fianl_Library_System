#include <seatui/student/student_window.hpp>
#include <seatui/launcher/login_window.hpp>
#include <seatui/student/navigation_canvas.hpp>
#include <QCheckBox>

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <QTextEdit>
#include <QFileDialog>
#include <QImageReader>
#include <QBuffer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QScrollArea>

#include <QMimeDatabase>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include <QDebug>
#include <seatui/widgets/card_dialog.hpp>

#include <seatui/student/book_search.hpp>

#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>


#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

// 时间归一，把 DB/服务端来的各种时间串，统一
static QString unifyTsToLocalIso(const QString& s) {
    if (s.isEmpty()) return s;
    QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
    if (!dt.isValid()) {
        // 常见的“空格分隔本地时”
        dt = QDateTime::fromString(s, "yyyy-MM-dd HH:mm:ss");
    }
    if (!dt.isValid()) return s; 
    // 统一转成本地时显示（你也可以改成 toUTC() + .toString(Qt::ISODate)）
    return dt.toLocalTime().toString(Qt::ISODate); // 例如 2025-12-03T21:50:10+08:00
}



// 映射：Unseated->0，其他->1
static int mapStateTextToCode(const QString& s) {
    if (s == "Unseated") return 0;  // 没人
    return 1;  // 有人（包括 "Seated" 和 "Anomaly"）
}



// 将0，1转换成文字
static QString demoStateText(int s){
    if (s == 1) return QStringLiteral("有人");
    return QStringLiteral("没人");
}

// demoCellCss ，展示座位占用情况的小格子
static QString demoCellCss(int s){
    if (s == 1) return "QFrame{ background:#064e3b; border:1px solid #115e59; border-radius:12px; } QLabel{ color:#d1fae5; }"; // 绿：有人
    return "QFrame{ background:#101319; border:1px solid #374151; border-radius:12px; } QLabel{ color:#cbd5e1; }";        // 灰：没人
}


//文件读取，把所有地方都翻一遍
static QString locateBooksFile() {
    // 运行目录 /Input/books.txt  
    QStringList candidates;
    const QString appDir = QCoreApplication::applicationDirPath();
    candidates << QDir(appDir).filePath("Input/books.txt");

    // 兼容某些构建目录层级（上一层、上两层）
    candidates << QDir(appDir + "/..").filePath("Input/books.txt");

    candidates << QDir(appDir + "/../..").filePath("Input/books.txt");

    // 当前工作目录（少数IDE会把 cwd 设置为别处）
    candidates << QDir::current().filePath("Input/books.txt");
    candidates << QDir(appDir).filePath("../src/student_app/books.txt");

    for (const QString& p : candidates) {
        if (QFileInfo::exists(p)) return QFileInfo(p).absoluteFilePath();
    }
    return QString(); // 没找到
}


// 左侧侧边栏创建按钮
static QPushButton* makeSideBtn(const QString& text, QWidget* parent) {
    auto *b = new QPushButton(text, parent);
    b->setCheckable(true);
    b->setMinimumHeight(40);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(
        "QPushButton{ text-align:left; padding:8px 12px; border:0; "
        " border-radius:8px; color:#e5e7eb; background:transparent; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.06);} "
        "QPushButton:checked{ background:rgba(59,130,246,0.18); color:#fff; }"
        );
    return b;
}



StudentWindow::StudentWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(u8"SeatUI 学生端");
    resize(1000, 680);

    // ===== 左侧侧边栏 =====
    auto side = new QFrame(this);
    side->setFixedWidth(190);
    side->setStyleSheet("QFrame{ background:#0f172a; border-right:1px solid #1f2937; }");
    auto sideLy = new QVBoxLayout(side);
    sideLy->setContentsMargins(12,16,12,16);
    sideLy->setSpacing(10);

    // 顶部返回登录按钮（非互斥按钮，不高亮选中状态）
    auto btnBack = new QPushButton(u8"← 返回登录", side);
    btnBack->setCursor(Qt::PointingHandCursor);
    btnBack->setStyleSheet(
        "QPushButton{ text-align:left; padding:8px 12px; border:0; border-radius:8px; "
        "  color:#cbd5e1; background:rgba(255,255,255,0.04);} "
        "QPushButton:hover{ background:rgba(255,255,255,0.10);} "
        "QPushButton:pressed{ background:rgba(37,99,235,0.25); color:#fff; }"
        );
    sideLy->addWidget(btnBack);

    auto title = new QLabel(u8"学生端", side);
    title->setStyleSheet("color:#cbd5e1; font-weight:600; padding:4px;");
    sideLy->addWidget(title);

    //侧边栏功能按钮
    btnDash = makeSideBtn(u8"🏠 欢迎", side);
    btnNav  = makeSideBtn(u8"🧭 馆内地图", side);
    btnHeat = makeSideBtn(u8"🌐 操作说明", side);
    btnHelp = makeSideBtn(u8"🆘 一键求助", side);
    btnLive = makeSideBtn(u8"💺 座位实况", side);
    btnBook = makeSideBtn(u8"📚 图书查询", side);
    btnPomo = makeSideBtn(u8"🍅 专注时刻", side);

    // 按钮互斥
    btnDash->setAutoExclusive(true);
    btnNav->setAutoExclusive(true);
    btnHeat->setAutoExclusive(true);
    btnHelp->setAutoExclusive(true);
    btnLive->setAutoExclusive(true);
    btnBook->setAutoExclusive(true);
    btnPomo->setAutoExclusive(true);

    //加入布局
    sideLy->addWidget(btnDash);
    sideLy->addWidget(btnNav);
    sideLy->addWidget(btnHeat);
    sideLy->addWidget(btnHelp);
    sideLy->addWidget(btnLive);
    sideLy->addWidget(btnBook);
    sideLy->addWidget(btnPomo);
    sideLy->addStretch();

    // 页面区
    pages = new QStackedWidget(this);
    pages->addWidget(buildDashboardPage());   // 0
    pages->addWidget(buildNavigationPage());  // 1
    pages->addWidget(buildHeatmapPage());     // 2
    pages->addWidget(buildHelpPage());        // 3
    pages->addWidget(buildLivePage());        // 4
    pages->addWidget(buildBookSearchPage());
    pages->addWidget(buildPomodoroPage()); //6  // 5 - 书籍搜索页面

    // 默认页面
    pages->setCurrentIndex(0);
    btnDash->setChecked(true);

    // 根布局：左侧栏 + 右侧页面
    auto central = new QWidget(this);
    auto root = new QHBoxLayout(central);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);
    root->addWidget(side);
    root->addWidget(pages, 1);
    setCentralWidget(central);

    // 点击按钮可以切换到该去的页面
    connect(btnBack, &QPushButton::clicked, this, &StudentWindow::onBackToLogin);
    connect(btnDash, &QPushButton::clicked, this, &StudentWindow::gotoDashboard);
    connect(btnNav,  &QPushButton::clicked, this, &StudentWindow::gotoNavigation);
    connect(btnHeat, &QPushButton::clicked, this, &StudentWindow::gotoHeatmap);
    connect(btnHelp, &QPushButton::clicked, this, &StudentWindow::gotoHelp);
    connect(btnLive, &QPushButton::clicked, this, &StudentWindow::gotoLive);
    connect(btnBook, &QPushButton::clicked, this, &StudentWindow::gotoBookSearch);
    connect(btnPomo, &QPushButton::clicked, this, &StudentWindow::gotoPomodoro);
    initWsClient();
}





QWidget* StudentWindow::buildDashboardPage() {
    auto page = new QWidget(this);
    // 保持深色背景风格
    page->setStyleSheet("background:#111827;");

    auto ly = new QVBoxLayout(page);
    ly->setContentsMargins(40, 60, 40, 60); 
    ly->setSpacing(20);

    // 1. 大标题
    auto title = new QLabel(u8"👋 欢迎使用图书馆管理系统", page);
    title->setStyleSheet("color:#ffffff; font-size:32px; font-weight:bold;");
    title->setAlignment(Qt::AlignCenter);

    // 2. 副标题/Slogan
    auto subTitle = new QLabel(u8"智慧助学 · 高效便捷 ", page);
    subTitle->setStyleSheet("color:#60a5fa; font-size:18px; font-weight:600; letter-spacing: 2px;");
    subTitle->setAlignment(Qt::AlignCenter);

    // 3. 装饰性分割线
    auto line = new QFrame(page);
    line->setFrameShape(QFrame::HLine);
    line->setFixedWidth(100);
    line->setStyleSheet("background-color: #374151;");
    
    // 4. 说明文字
    auto desc = new QLabel(
        u8"这是一个集成了座位管理、图书检索以及专注力辅助的综合性学生端系统。\n"
        u8"请点击左侧菜单栏开始您的学习之旅。", page);
    desc->setStyleSheet("color:#9ca3af; font-size:15px; line-height: 150%;");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true); 

    // 添加到布局
    ly->addStretch(); 
    ly->addWidget(title);
    ly->addWidget(subTitle);
    
    // 让分割线居中
    auto hLineLay = new QHBoxLayout();
    hLineLay->addStretch();
    hLineLay->addWidget(line);
    hLineLay->addStretch();
    ly->addLayout(hLineLay);

    ly->addWidget(desc);
    ly->addStretch(); 

    return page;
}
QWidget* StudentWindow::buildNavigationPage() {
    auto page = new QWidget(this);

    // 保持原有样式不变
    page->setStyleSheet(
        "QWidget{ background:#0b1220; }"
        "QLabel{ color:#cbd5e1; }"
        "QCheckBox{ color:#e5e7eb; spacing: 5px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; }"
        "#mapFrame{ background:#101319; border:1px solid #374151; border-radius:12px; }"
    );

    auto root = new QVBoxLayout(page);
    root->setContentsMargins(20,20,20,20);
    root->setSpacing(12);


    auto ctrl = new QHBoxLayout();
    
    auto titleLabel = new QLabel(u8"🗺️ 图书馆 1F 平面分布图", page);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #60a5fa;");
    
//平滑
    auto ssaaBox = new QCheckBox(u8"启用高清矢量渲染 (SSAA)", page);
    ssaaBox->setChecked(true);
    ssaaBox->setCursor(Qt::PointingHandCursor);

    ctrl->addWidget(titleLabel);
    ctrl->addStretch(); // 弹簧把选项推到右边
    ctrl->addWidget(ssaaBox);

    auto canvasWidget = new NavigationCanvas(page);
    canvasWidget->setObjectName("mapFrame");
    navCanvas = canvasWidget;

    connect(ssaaBox, &QCheckBox::toggled, canvasWidget, &NavigationCanvas::setSuperSample);

    navStatus = new QLabel(u8"提示：该视图实时渲染图书馆区域布局，支持无损缩放。", page);
    navStatus->setStyleSheet("color:#93a4b5;");

    root->addLayout(ctrl);
    root->addWidget(navCanvas, 1);
    root->addWidget(navStatus);

    return page;
}

QWidget* StudentWindow::buildHeatmapPage() {
    auto page = new QWidget(this);
    page->setStyleSheet("QWidget{ background:#0b1220; }");

    auto ly = new QVBoxLayout(page);
    ly->setContentsMargins(20, 20, 20, 20);
    ly->setSpacing(10);

    // 顶部标题
    auto title = new QLabel(u8"📖 系统操作说明", page);
    title->setStyleSheet("color:#e5e7eb; font-weight:bold; font-size:20px; margin-bottom: 10px;");
    ly->addWidget(title);

    auto viewer = new QTextEdit(page);
    viewer->setReadOnly(true);
    // 样式：深色背景，浅色文字，无边框
    viewer->setStyleSheet(
        "QTextEdit { "
        "   background-color: #111827; "
        "   color: #d1d5db; "
        "   border: 1px solid #374151; "
        "   border-radius: 12px; "
        "   padding: 15px; "
        "   font-size: 14px; "
        "   line-height: 24px; "
        "}"
        "QScrollBar:vertical { width: 8px; background: #111827; }"
        "QScrollBar::handle:vertical { background: #4b5563; border-radius: 4px; }"
    );

    // 编写 HTML 内容
    QString htmlContent = u8R"(
    <style>
        h3 { color: #60a5fa; margin-top: 20px; font-size: 16px; }
        p { color: #cbd5e1; margin-bottom: 8px; }
        li { color: #9ca3af; margin-bottom: 4px; }
        b { color: #e5e7eb; }
    </style>

    <h3>1. 🧭馆内地图</h3>
    <p>用于查找学生座位位置。</p>
    <ul>
        <li>在下拉框中可看到目标区域（<b>A / B / C / D</b>）。</li>
    </ul>

    <h3>2. 💺 座位实况</h3>
    <p>实时查看图书馆座位占用情况（需连接服务器）。</p>
    <ul>
        <li><b>绿色 (有人)：</b> 座位已被使用。</li>
        <li><b>灰色 (没人)：</b> 座位空闲，可自由入座。</li>
        <li>系统通过 WebSocket 实时推送状态变化，无需手动刷新。</li>
    </ul>

    <h3>3. 📚 图书查询</h3>
    <p>快速检索馆藏书籍信息。</p>
    <ul>
        <li>输入书名或作者关键词（如 "C++", "Data"）。</li>
        <li>点击搜索后，结果将显示 ISBN、索书号及借阅状态。</li>
        <li>若显示“暂无此书籍”，请尝试更换关键词。</li>
    </ul>

    <h3>4. 🍅 专注时刻 (番茄钟)</h3>
    <p>帮助您保持专注的学习计时器。</p>
    <ul>
        <li>默认设定为 <b>25分钟</b> 工作时间。</li>
        <li>点击“开始专注”启动倒计时，结束后建议休息 5 分钟。</li>
        <li>中途可暂停或重置计时。</li>
    </ul>

    <h3>5. 🆘 一键求助</h3>
    <p>遇到设施故障或纠纷时使用。</p>
    <ul>
        <li>填写文字描述或上传现场照片。</li>
        <li>点击提交后，管理员端会立即收到通知并处理。</li>
    </ul>
    )";

    viewer->setHtml(htmlContent);
    
    ly->addWidget(viewer, 1); 
    return page;
}

QWidget* StudentWindow::buildBookSearchPage() {
    auto page = new QWidget(this);
    page->setStyleSheet("QWidget{ background:#0b1220; } QLabel{ color:#cbd5e1; }");

    auto root = new QVBoxLayout(page);
    root->setContentsMargins(20,20,20,20);
    root->setSpacing(12);

    auto title = new QLabel(u8"📚 图书查询", page);
    title->setStyleSheet("font-weight:600; font-size:16px;");
    root->addWidget(title);

    // to give some tips
    auto row = new QHBoxLayout();
    row->setSpacing(8);
    auto lab = new QLabel(u8"关键词：", page);
    bookInput = new QLineEdit(page);
    bookInput->setPlaceholderText(u8"输入作者或书名的一部分，例如：Tanenbaum / Data / Prata …");

    // theme mode: dark but let the words stand out
    bookInput->setStyleSheet(
        "QLineEdit{"
        "  color:#e5e7eb;"                   
        "  background:#0f172a;"               /* 深色背景 */
        "  border:1px solid #94a3b8;"
        "  border-radius:12px;"
        "  padding:8px 12px;"
        "  selection-background-color:#334155;"/* 选中文本底色 */
        "}"
        "QLineEdit::placeholder{"
        "  color:#9ca3af;"                    /* placeholder 浅灰 */
        "}"
        );

    
    {
        QPalette pal = bookInput->palette();
        pal.setColor(QPalette::Text, QColor("#e5e7eb"));               // 正文字
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
        pal.setColor(QPalette::PlaceholderText, QColor("#9ca3af"));    // Qt 6
#endif
        bookInput->setPalette(pal);
    }


    bookSearchBtn = new QPushButton(u8"搜索", page);


    bookSearchBtn->setStyleSheet(
        "QPushButton{"
        "  color:#e5e7eb;"
        "  background:#1f2937;"
        "  border:1px solid #334155;"
        "  border-radius:10px;"
        "  padding:6px 14px;"
        "}"
        "QPushButton:hover{"
        "  background:#223047;"
        "}"
        "QPushButton:pressed{"
        "  background:#1b2638;"
        "}"
        );



    bookSearchBtn->setProperty("type","primary");
    row->addWidget(lab);
    row->addWidget(bookInput, 1);
    row->addWidget(bookSearchBtn);
    root->addLayout(row);

    // result table
    bookTable = new QTableWidget(page);
    bookTable->setColumnCount(9);
    bookTable->setStyleSheet(
        "QTableWidget {"
        "   background-color: #0f172a;"   
        "   color: #e5e7eb;"              
        "   gridline-color: #334155;"     
        "   border: 1px solid #334155;"   
        "   border-radius: 8px;"
        "   selection-background-color: #334155;" 
        "   selection-color: #ffffff;"            
        "}"
        "QHeaderView::section {"         
        "   background-color: #1e293b;"   
        "   color: #94a3b8;"             
        "   padding: 8px;"
        "   border: none;"
        "   border-bottom: 1px solid #334155;"
        "   border-right: 1px solid #0f172a;"
        "}"
        "QTableCornerButton::section {"   
        "   background-color: #1e293b;"
        "   border: none;"
        "}"
    );
    bookTable->setHorizontalHeaderLabels({
        "ISBN","Title","Author","Publisher","Date","Category","CallNumber","Total","Available"
    });
    bookTable->horizontalHeader()->setStretchLastSection(true);
    bookTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    bookTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bookTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    root->addWidget(bookTable, 1);

    // tips when there is no book
    bookHint = new QLabel(u8"暂无此书籍。", page);
    bookHint->setStyleSheet("color:#93a4b5;");
    bookHint->setVisible(false);
    root->addWidget(bookHint);

    // connect search buttion
    connect(bookSearchBtn, &QPushButton::clicked, this, &StudentWindow::onSearchBooks);

    // load books
    if (!bookEngine.ready()) {
        const QString path = locateBooksFile(); 
        QString err;
        if (!path.isEmpty()) {
            bookEngine.loadFromFile(path, &err); 
        }
    }


    bookPage = page;
    return page;
}

void StudentWindow::onSearchBooks() {
    const QString kw = bookInput ? bookInput->text().trimmed() : QString();
    if (kw.isEmpty()) {
        CardDialog(u8"提示", u8"请输入作者或书名关键词。", this).exec();
        return;
    }

    // 若尚未加载，尝试再加载一次）
    if (!bookEngine.ready()) {
        const QString path = locateBooksFile();
        QString err;
        if (path.isEmpty()) {
            CardDialog(u8"读取失败",
                       u8"未找到 books.txt。\n"
                       u8"请确认已将文件放在 项目根目录/Input/books.txt，"
                       u8"并且 CMake 已执行拷贝到运行目录。",
                       this).exec();
            return;
        }
        if (!bookEngine.loadFromFile(path, &err)) {
            CardDialog(u8"读取失败",
                       u8"无法读取：\n" + path + u8"\n\n错误：" + err,
                       this).exec();
            return;
        }
    }


    const auto matches = bookEngine.searchByKeyword(kw);
    bookTable->setRowCount(0);

    if (matches.isEmpty()) {
        bookHint->setVisible(true);  // 显示"暂无此书籍。"
        return;
    }
    bookHint->setVisible(false);
// fill in the table
    bookTable->setRowCount(matches.size());
    for (int i=0;i<matches.size();++i) {
        const auto& b = matches[i];
        auto set = [&](int c, const QString& text){
            auto *it = new QTableWidgetItem(text);
            bookTable->setItem(i, c, it);
        };
        set(0, b.isbn);
        set(1, b.title);
        set(2, b.author);
        set(3, b.publisher);
        set(4, b.date);
        set(5, b.category);
        set(6, b.callNumber);
        set(7, QString::number(b.total));
        set(8, QString::number(b.available));
    }
}

QWidget* StudentWindow::buildHelpPage() {
    auto page = new QWidget(this);
    page->setStyleSheet(
        "QWidget{ background:#0b1220; }"
        "QLabel{ color:#cbd5e1; }"
        "QTextEdit{ color:#e5e7eb; background:#0f172a; border:1px solid #374151; "
        "  border-radius:8px; padding:8px 10px; }"
        "QPushButton{ color:#e5e7eb; background:#1f2937; border:1px solid #374151; "
        "  border-radius:8px; padding:6px 12px; }"
        "QPushButton:hover{ background:#374151; }"
        "QPushButton:pressed{ background:#2563eb; border-color:#2563eb; }"
        "#imgBox{ background:#101319; border:1px dashed #374151; border-radius:12px; }"
        );

    auto root = new QVBoxLayout(page);
    root->setContentsMargins(20,20,20,20);
    root->setSpacing(12);

    auto title = new QLabel(u8"🆘 一键求助", page);
    title->setStyleSheet("color:#e5e7eb; font-weight:600; font-size:16px;");
    root->addWidget(title);

    auto tip = new QLabel(u8"请描述你的问题（可选附图）。提交后管理员端将实时收到。", page);
    tip->setStyleSheet("color:#93a4b5;");
    root->addWidget(tip);

    // 填写文本区域
    helpText_ = new QTextEdit(page);
    helpText_->setPlaceholderText(u8"例如：自习区有人高声通话 / 插座损坏 / 座位被物品长期占用…（必填其一：文字或图片）");
    helpText_->setMinimumHeight(120);
    root->addWidget(helpText_);

    //  图片区域：预览 + 选择
    auto imgRow = new QHBoxLayout();
    imgRow->setSpacing(12);
//图片外框
    auto imgBox = new QFrame(page);
    imgBox->setObjectName("imgBox");
    imgBox->setMinimumSize(220, 160);
    auto imgLy = new QVBoxLayout(imgBox);
    imgLy->setContentsMargins(12,12,12,12);
    imgLy->setSpacing(8);

    //图片预览，默认是无图片
    helpImgPreview_ = new QLabel(imgBox);
    helpImgPreview_->setAlignment(Qt::AlignCenter);
    helpImgPreview_->setText(u8"（无图片）");
    helpImgPreview_->setStyleSheet("color:#66758a;");
    helpImgPreview_->setMinimumHeight(120);
    imgLy->addWidget(helpImgPreview_, 1);

    //图片选择按钮
    helpPickBtn_ = new QPushButton(u8"选择图片…", imgBox);
    imgLy->addWidget(helpPickBtn_, 0, Qt::AlignRight);

    imgRow->addWidget(imgBox, 0);

    imgRow->addStretch();
    root->addLayout(imgRow);

    // 重置和提交
    auto op = new QHBoxLayout();
    op->addStretch();
    helpResetBtn_  = new QPushButton(u8"重置", page);
    helpSubmitBtn_ = new QPushButton(u8"提交", page); helpSubmitBtn_->setEnabled(true);
    op->addWidget(helpResetBtn_);
    op->addWidget(helpSubmitBtn_);
    root->addLayout(op);

    // 连接
    connect(helpPickBtn_,  &QPushButton::clicked, this, &StudentWindow::onPickImage);
    connect(helpResetBtn_, &QPushButton::clicked, this, &StudentWindow::onResetHelp);
    connect(helpSubmitBtn_,&QPushButton::clicked, this, &StudentWindow::onSubmitHelp);

    return page;
}

// 构建座位实况页面
QWidget* StudentWindow::buildLivePage() {
    auto page = new QWidget(this);
    page->setStyleSheet("QWidget{ background:#0b1220; } QLabel{ color:#cbd5e1; }");

    auto root = new QVBoxLayout(page);
    root->setContentsMargins(20,20,20,20);
    root->setSpacing(12);

    // 标题
    auto title = new QLabel(u8"座位实况（Demo：2×2，S1~S4）", page);
    title->setStyleSheet("font-weight:600; font-size:16px;");
    root->addWidget(title);

    // 2×2 的格子
    auto grid = new QGridLayout();
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);

    liveCells_.clear();
    liveCells_.reserve(4);

    // Qframe是座位，右侧顶部有显示
    auto makeCell = [&](const QString& id){
        auto box = new QFrame(page);
        box->setMinimumSize(160,120);
        box->setStyleSheet("QFrame{ background:#101319; border:1px solid #374151; border-radius:12px; }");

        auto v = new QVBoxLayout(box);
        v->setContentsMargins(12,12,12,12);
        v->setSpacing(6);

        auto head = new QLabel(QString(u8"座位 %1").arg(id), box);
        head->setStyleSheet("color:#e5e7eb; font-weight:600;");

        // 右下角状态（初始显示 ）
        auto body = new QLabel(u8"—", box);
        body->setStyleSheet("color:#93a4b5;");

        v->addWidget(head);
        v->addStretch();
        v->addWidget(body, 0, Qt::AlignRight);

        liveCells_.push_back(body);   // 记录下来，后面按索引设置
        return box;
    };
//创建四个格子
    grid->addWidget(makeCell("S1"), 0,0);
    grid->addWidget(makeCell("S2"), 0,1);
    grid->addWidget(makeCell("S3"), 1,0);
    grid->addWidget(makeCell("S4"), 1,1);

    root->addLayout(grid, 1);

    // 提示说明
    auto tip = new QLabel(u8"颜色：绿=有人、黄=占座(有物无人)、灰=没人；下方文字为状态与 since（演示先写死）。", page);
    tip->setStyleSheet("color:#66758a;");
    root->addWidget(tip);

    // 进入页面时直接先写一组状态
    //   S1=有人(1) S2=占座(2) S3=没人(0) S4=有人(1)
    const QString demoSince = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    liveSetCell("S1", 0, demoSince);  // 有人
    liveSetCell("S2", 0, demoSince);  // 有人（原来是占座，现在也算有人）
    liveSetCell("S3", 0, demoSince);  // 没人
    liveSetCell("S4", 0, demoSince);  // 有人

    livePage = page;
    return page;
}

/* 侧边栏切换 */
void StudentWindow::gotoDashboard() {
    pages->setCurrentIndex(0);
    btnDash->setChecked(true);
}

void StudentWindow::gotoNavigation(){
    pages->setCurrentIndex(1);
    btnNav->setChecked(true);
}

void StudentWindow::gotoHeatmap()   {
    pages->setCurrentIndex(2);
    btnHeat->setChecked(true);
}

void StudentWindow::gotoHelp() {
    pages->setCurrentIndex(3);
    btnHelp->setChecked(true);
}

void StudentWindow::gotoLive() {
    if (livePage) {
        pages->setCurrentWidget(livePage);
        if (btnLive) btnLive->setChecked(true);
    }
}

void StudentWindow::gotoBookSearch() {
    if (bookPage) {
        pages->setCurrentWidget(bookPage);
        if (btnBook) btnBook->setChecked(true);
    }
}

/* ---------- 返回登录 ---------- */
#include <QTimer>

void StudentWindow::onBackToLogin() {
    this->hide();
    QTimer::singleShot(0, this, [this]{
        auto *login = new LoginWindow();
        login->setAttribute(Qt::WA_DeleteOnClose);
        login->show();
        this->deleteLater();
    });
}



void StudentWindow::onPickImage() {
    const QString file = QFileDialog::getOpenFileName(
        this, u8"选择图片",
        QString(),
        u8"图像文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
        );
    if (file.isEmpty()) return;

    QImageReader reader(file);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (img.isNull()) {
        CardDialog(u8"读取失败", u8"无法读取该图片文件。", this).exec(); // 复用你的卡片弹框
        return;
    }

    // 预览：自适应缩放
    const int maxW = 360, maxH = 200;
    QImage scaled = img.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    helpImgPreview_->setPixmap(QPixmap::fromImage(scaled));
    helpImgPreview_->setText(QString());

    // 编码：优先 PNG
    QByteArray bytes;
    {
        QBuffer buf(&bytes);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG", 6);
    }
    helpImgBytes_    = bytes;
    helpImgFilename_ = QFileInfo(file).fileName();
    helpImgMime_     = "image/png";
}

void StudentWindow::onResetHelp() {
    helpText_->clear();
    helpImgPreview_->setPixmap(QPixmap());
    helpImgPreview_->setText(u8"（无图片）");
    helpImgBytes_.clear();
    helpImgFilename_.clear();
    helpImgMime_.clear();
}



void StudentWindow::onSubmitHelp() {
    const QString desc = helpText_->toPlainText().trimmed();

    // 确保至少有文本或图片
    if (desc.isEmpty() && helpImgBytes_.isEmpty()) {
        CardDialog(u8"内容为空", u8"请至少填写文字或选择一张图片。", this).exec();
        return;
    }

    // 构建 JSON 数据
    QJsonObject root;
    root["type"] = "student_help";
    root["user"] = "student";  // 可以替换为登录用户名
    root["description"] = desc;
    root["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    if (!helpImgBytes_.isEmpty()) {
        QJsonObject img;
        img["filename"] = helpImgFilename_.isEmpty() ? "help.png" : helpImgFilename_;
        img["mime"] = helpImgMime_.isEmpty() ? "image/png" : helpImgMime_;
        img["base64"] = QString::fromLatin1(helpImgBytes_.toBase64());
        root["image"] = img;
    }

    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

    // 发送给管理员端
    wsSend(payload);

    // 提示用户已提交
    CardDialog(u8"已提交", u8"你的求助信息已发送，管理员会尽快处理。", this).exec();

    // 清空内容
    onResetHelp();
}



void StudentWindow::initWsClient() {
    ws_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    ws_->ignoreSslErrors();  // 处理非 TLS 连接
    wsReady_ = false;

    connect(ws_, &QWebSocket::connected, this, [this]{
        wsReady_ = true;
        // 握手发送角色信息
        ws_->sendTextMessage(QStringLiteral(R"({"type":"hello","role":"student"})"));
    });

    connect(ws_, &QWebSocket::disconnected, this, [this]{
        wsReady_ = false;
        // 简单重连（1秒后重连）
        QTimer::singleShot(1000, this, [this]{
            ws_->open(QUrl(QStringLiteral("ws://127.0.0.1:12345")));
        });
    });

    connect(ws_, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, [this](auto){
                // 错误时重连
                QTimer::singleShot(1000, this, [this]{
                    if (ws_ && !wsReady_) ws_->open(QUrl(QStringLiteral("ws://127.0.0.1:12345")));
                });
            });

    // WebSocket 连接成功后
    connect(ws_, &QWebSocket::textMessageReceived, this, [this](const QString& msg) {
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(msg.toUtf8(), &error);

        // 如果解析出错或不是有效的 JSON 对象，则返回
        if (error.error != QJsonParseError::NoError || !doc.isObject()) return;

        const auto obj = doc.object();

        // 如果接收到的是 seat_snapshot 类型的消息
        if (obj.value("type").toString() == "seat_snapshot") {
            const auto arr = obj.value("items").toArray();

            // 遍历每个座位的状态
            for (const auto& item : arr) {
                if (!item.isObject()) continue;
                const auto o = item.toObject();
                const QString id = o.value("seat_id").toString();
                const int st = o.value("state").toInt(0);
                const QString since = o.value("since").toString();

                // 更新 UI：更新座位状态（只保留这一行，删除表格相关代码）
                liveSetCell(id, st, since);
            }
        }



        else if (obj.value("type").toString() == "seat_update") {
            const auto seats = obj.value("seats").toArray();
            for (const auto& v : seats) {
                if (!v.isObject()) continue;
                const auto o = v.toObject();
                const QString id = o.value("id").toString();
                const QString state = o.value("state").toString();

                // 使用新的映射函数
                const int code = mapStateTextToCode(state);
                const QString since = unifyTsToLocalIso(o.value("last_update").toString());

                // 更新座位显示
                liveSetCell(id, code, since);
            }
        }



    });

    // 首次连接
    ws_->open(QUrl(QStringLiteral("ws://127.0.0.1:12345")));
}

void StudentWindow::wsSend(const QByteArray& utf8Json) {
    if (ws_ && wsReady_) {
        ws_->sendTextMessage(QString::fromUtf8(utf8Json));
    } else {
        // 兜底：连不上就提醒（不丢数据也行：可选入本地队列/DB）
        CardDialog(u8"未连接", u8"尚未连接管理员端（WS）。稍后将自动重试。", this).exec();
    }
}








//liveSetCell 函数负责更新座位实况页面的座位卡片显示。
// 去掉S，找对应的Qlabel, 设置label的文字和颜色
void StudentWindow::liveSetCell(const QString& id, int state, const QString& sinceIso) {
    bool ok = false;
    int idx = id.mid(1).toInt(&ok);
    if (!ok || idx < 1 || idx > 4 || idx > liveCells_.size()) return;

    QLabel* lab = liveCells_[idx - 1];
    lab->setText(demoStateText(state) + "\n" + sinceIso);

    // 更新外层卡片的颜色
    if (auto box = qobject_cast<QFrame*>(lab->parentWidget())) {
        box->setStyleSheet(demoCellCss(state));
    }
}



void StudentWindow::gotoPomodoro() {
    // must match the up page index
    pages->setCurrentIndex(6);
    btnPomo->setChecked(true);
}

QWidget* StudentWindow::buildPomodoroPage() {
    auto page = new QWidget(this);
    page->setStyleSheet("background:#0b1220;"); // dark background

    auto ly = new QVBoxLayout(page);
    ly->setAlignment(Qt::AlignCenter);
    ly->setSpacing(30);

    // state label
    pomoStatusLabel = new QLabel(u8"💪 保持专注", page);
    pomoStatusLabel->setStyleSheet("color:#9ca3af; font-size:24px; font-weight:600;");
    pomoStatusLabel->setAlignment(Qt::AlignCenter);// place it in the center

    // 2. counting down number
    pomoTimeLabel = new QLabel("25:00", page);
    pomoTimeLabel->setAlignment(Qt::AlignCenter);
    pomoTimeLabel->setStyleSheet(
        "color:#e5e7eb; font-size:90px; font-weight:bold; font-family:Consolas, Monospace;"
        );

    // 3. buttons
    auto btnBox = new QHBoxLayout();
    btnBox->setSpacing(20);
    btnBox->setAlignment(Qt::AlignCenter);

    // strat and end button
    pomoStartBtn = new QPushButton(u8"开始专注", page);
    pomoStartBtn->setFixedSize(140, 50);
    pomoStartBtn->setCursor(Qt::PointingHandCursor);
    pomoStartBtn->setStyleSheet(
        "QPushButton{ background:#2563eb; color:white; border-radius:25px; font-size:18px; font-weight:600; }"
        "QPushButton:hover{ background:#1d4ed8; }"
        );

    // reseting the button
    auto resetBtn = new QPushButton(u8"重置", page);
    resetBtn->setFixedSize(100, 50);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet(
        "QPushButton{ background:#374151; color:#e5e7eb; border-radius:25px; font-size:16px; }"
        "QPushButton:hover{ background:#4b5563; }"
        );

    btnBox->addWidget(pomoStartBtn);
    btnBox->addWidget(resetBtn);

    // 4. hint label
    auto hint = new QLabel(u8"工作 25分钟 · 休息 5分钟", page);
    hint->setStyleSheet("color:#6b7280; font-size:14px; margin-top:10px;");
    hint->setAlignment(Qt::AlignCenter);


    ly->addStretch();
    ly->addWidget(pomoStatusLabel);
    ly->addWidget(pomoTimeLabel);
    ly->addLayout(btnBox);
    ly->addWidget(hint);
    ly->addStretch();

    // initializa timer
    pomoTimer = new QTimer(this);
    pomoTimer->setInterval(1000); // once a time per second
// start or pause
    connect(pomoStartBtn, &QPushButton::clicked, this, &StudentWindow::onPomoToggle);
// reset
    connect(resetBtn,     &QPushButton::clicked, this, &StudentWindow::onPomoReset);
//time out
    connect(pomoTimer,    &QTimer::timeout, this, &StudentWindow::onPomoTick);

    return page;
}

// start or pause logic
void StudentWindow::onPomoToggle() {
    if (isPomoRunning) {
        // pause
        pomoTimer->stop();
        isPomoRunning = false;
        pomoStartBtn->setText(u8"继续");
        pomoStartBtn->setStyleSheet("QPushButton{ background:#2563eb; color:white; border-radius:25px; font-size:18px; font-weight:600; }");
    } else {
        // start
        pomoTimer->start();
        isPomoRunning = true;
        pomoStartBtn->setText(u8"暂停");
        pomoStartBtn->setStyleSheet("QPushButton{ background:#ca8a04; color:white; border-radius:25px; font-size:18px; font-weight:600; }");
    }
}

// reset logic
void StudentWindow::onPomoReset() {
    pomoTimer->stop();
    isPomoRunning = false;
    isPomoWorkState = true;
    pomoRemainingSec = 25 * 60; // reset to 25 minutes

    // reset UI again
    pomoTimeLabel->setText("25:00");
    pomoTimeLabel->setStyleSheet("color:#e5e7eb; font-size:90px; font-weight:bold; font-family:Consolas, Monospace;");
    pomoStatusLabel->setText(u8"💪 保持专注");
    pomoStartBtn->setText(u8"开始专注");
    pomoStartBtn->setStyleSheet("QPushButton{ background:#2563eb; color:white; border-radius:25px; font-size:18px; font-weight:600; }");
}


// counting down logic
void StudentWindow::onPomoTick() {
    if (pomoRemainingSec > 0) {
        pomoRemainingSec--;

        int m = pomoRemainingSec / 60;
        int s = pomoRemainingSec % 60;
        QString t = QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        pomoTimeLabel->setText(t);
    }
    else {
        // times up
        pomoTimer->stop();
        isPomoRunning = false;

        if (isPomoWorkState) {
            // ending work- rest
            CardDialog(u8"专注完成", u8"太棒了！休息 5 分钟吧。", this).exec();

            isPomoWorkState = false;
            pomoRemainingSec = 5 * 60;

            pomoTimeLabel->setText("05:00");
            pomoTimeLabel->setStyleSheet("color:#10b981; font-size:90px; font-weight:bold; font-family:Consolas, Monospace;");
            pomoStatusLabel->setText(u8"☕ 休息时间");
            pomoStartBtn->setText(u8"开始休息");
            pomoStartBtn->setStyleSheet("QPushButton{ background:#10b981; color:white; border-radius:25px; font-size:18px; font-weight:600; }");
        } else {
            // ending the rest - work
            CardDialog(u8"休息结束", u8"准备好开始新一轮专注了吗？", this).exec();
            onPomoReset();
        }
    }
}


