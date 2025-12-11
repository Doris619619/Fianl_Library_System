#include <seatui/launcher/login_window.hpp>
#include <seatui/launcher/role_selector.hpp>
#include <seatui/student/student_window.hpp>
#include <seatui/admin/admin_window.hpp>
#include <seatui/widgets/card_dialog.hpp>
#include <QStackedWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QAction>
#include <QCheckBox>
#include <QGraphicsDropShadowEffect>
#include <QColor>
#include <QMessageBox>
#include <QDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QTimer>
#include <QGuiApplication>
#include <QScreen>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget* parent)
    : QMainWindow(parent), stacked_(new QStackedWidget(this)),
    user_(nullptr), pass_(nullptr), msg_(nullptr) {
    setWindowTitle(u8"SeatUI 登录");// Set the window title.
    setCentralWidget(stacked_);// Set the central widget of the main window to stacked_.
    stacked_->addWidget(buildLoginPage()); // Add the login page (index 0).
    stacked_->addWidget(buildRolePage());  // index 1
}


//Build the login page.
QWidget* LoginWindow::buildLoginPage() {
    auto page = new QWidget(this);// Create a new page.

    auto outer = new QVBoxLayout(page);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->addStretch();

    auto card = new QFrame(page);
    card->setObjectName("card");
    card->setMinimumWidth(420);
    auto effect = new QGraphicsDropShadowEffect(card);
    effect->setBlurRadius(24);
    effect->setOffset(0, 6);
    effect->setColor(QColor(0,0,0,40));
    card->setGraphicsEffect(effect);

    auto v = new QVBoxLayout(card);
    v->setContentsMargins(28, 28, 28, 28);
    v->setSpacing(14);

    auto title = new QLabel(u8"欢迎登录 SeatUI", card);
    title->setStyleSheet("font-size: 18pt; font-weight: 600;");

    auto sub = new QLabel(u8"请输入账号信息", card);
    sub->setProperty("hint", true);

    auto form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignVCenter);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(10);

    user_ = new QLineEdit(card);
    user_->setPlaceholderText(u8"学号 / 工号");

    pass_ = new QLineEdit(card);
    pass_->setPlaceholderText(u8"密码");
    pass_->setEchoMode(QLineEdit::Password);

    auto toggle = new QAction(u8"显示", pass_);
    toggle->setCheckable(true);
    pass_->addAction(toggle, QLineEdit::TrailingPosition);
    QObject::connect(toggle, &QAction::toggled, pass_, [this, toggle](bool on){
        pass_->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
        toggle->setText(on ? u8"隐藏" : u8"显示");
    });

    form->addRow(u8"用户名", user_);
    form->addRow(u8"密码",   pass_);

    auto row = new QHBoxLayout();
    auto remember = new QCheckBox(u8"记住我", card);
    row->addWidget(remember);
    row->addStretch();

    auto btn = new QPushButton(u8"登录", card);
    btn->setProperty("type", "primary");
    btn->setMinimumHeight(40);

    msg_ = new QLabel(card);
    msg_->setStyleSheet("color:#b71c1c;");

    v->addWidget(title);
    v->addWidget(sub);
    v->addSpacing(4);
    v->addLayout(form);
    v->addLayout(row);
    v->addSpacing(6);
    v->addWidget(btn);
    v->addWidget(msg_);

    outer->addWidget(card, 0, Qt::AlignHCenter);
    outer->addStretch();

    connect(btn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    return page;
}

//Build the role selection page.
QWidget* LoginWindow::buildRolePage() {
    auto role = new RoleSelector(this);

    // After selection, launch the corresponding main window (this window remains open to facilitate fallback and testing).


    connect(role, &RoleSelector::openStudent, this, [this]() {
    });
    connect(role, &RoleSelector::openAdmin, this, [this]() {
    });

    return role;
}

void LoginWindow::onLoginClicked() {
    const QString u = user_->text().trimmed();
    const QString p = pass_->text();

    if (u.isEmpty() || p.isEmpty()) {
        msg_->setText(u8"用户名或密码不能为空。");
        return;
    }
    msg_->clear();

    struct Cred { const char* user; const char* pass; const char* role; };
    static const Cred CREDS[] = {
        {"student", "123456", "student"},
        {"admin",   "123456", "admin"}
    };

    for (const auto& c : CREDS) {
        if (u == QString::fromUtf8(c.user) && p == QString::fromUtf8(c.pass)) {
            if (QString::fromUtf8(c.role) == "student") {
                auto* w = new StudentWindow();
                w->setAttribute(Qt::WA_DeleteOnClose);
                w->show();
            } else {
                auto* w = new AdminWindow();
                w->setAttribute(Qt::WA_DeleteOnClose);
                w->show();
            }
            // 关键修改：下一拍再关登录窗，更稳
            QTimer::singleShot(0, this, [this]{ this->close(); });
            return;
        }
    }

    CardDialog(u8"登录失败", u8"用户名或密码错误。", this).exec();
}





