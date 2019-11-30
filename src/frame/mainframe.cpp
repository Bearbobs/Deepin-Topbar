#include "mainframe.h"

#include "frame/utils/global.h"

#include <QScreen>
#include <QApplication>
#include <QApplication>
#include <QRect>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <QtX11Extras/QX11Info>
#include <DPlatformWindowHandle>
#include <DForeignWindow>
#include <QTimer>
#include <DWindowManagerHelper>
#include <QScreen>

DWIDGET_USE_NAMESPACE

#define DOCK_POS_TOP 0
#define DOCK_POS_RIGHT 1
#define DOCK_POS_BOTTOM 2
#define DOCK_POS_LEFT 3

static const QStringList SKIP_APP {
    "dde-desktop",
    "dde-launcher",
    "dde-osd"
};

MainFrame::MainFrame(QWidget *parent)
    : DBlurEffectWidget(parent)
{
    init();
    initAnimation();
    initConnect();
    screenChanged();

    const QPoint self_point = geometry().topLeft();
    move(QPoint(self_point.x() / devicePixelRatioF(), self_point.y() / devicePixelRatioF() - TOPHEIGHT));

    QTimer::singleShot(100, this, [=] {
        m_launchAni->setStartValue(geometry().topLeft());
        m_launchAni->setEndValue(self_point);
        m_launchAni->start();
    });

#if (DTK_VERSION >= DTK_VERSION_CHECK(2, 0, 9, 10))
    QTimer::singleShot(1000, this, [=] {
        connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::windowListChanged, this, &MainFrame::onWindowListChanged, Qt::QueuedConnection);
        onWindowListChanged();
    });
#endif
}

MainFrame::~MainFrame()
{
    m_dockInter->deleteLater();
    m_desktopWidget->deleteLater();
}

/*
     Think zccrs, Perfect protection against launcher. It won't stop launcher at last.
     */

void MainFrame::init()
{
 	
    m_desktopWidget = QApplication::desktop();
    setWindowFlags(Qt::X11BypassWindowManagerHint); 
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    setAttribute(Qt::WA_TranslucentBackground);
    setMaskColor(DBlurEffectWidget::DarkColor);
    setFull(true);	
    DPlatformWindowHandle handle(this);	
    handle.setShadowColor(QColor(0, 0, 0, 0));	
    m_mainPanel = new dtb::MainPanel(this);

//    m_showWithLauncher =new QPropertyAnimation(m_mainPanel, "pos", m_mainPanel);
//    m_showWithLauncher->setDuration(300);
//    m_showWithLauncher->setStartValue(QPoint(m_mainPanel->x(), -m_mainPanel->height()));
//    m_showWithLauncher->setEndValue(QPoint(m_mainPanel->x(), 0));
//    m_showWithLauncher->setEasingCurve(QEasingCurve::InOutCubic);

//    m_hideWithLauncher =new QPropertyAnimation(m_mainPanel, "pos", m_mainPanel);
//    m_hideWithLauncher->setDuration(300);
//    m_hideWithLauncher->setStartValue(QPoint(m_mainPanel->x(), 0));
//    m_hideWithLauncher->setEndValue(QPoint(m_mainPanel->x(), -m_mainPanel->height()));
//    m_hideWithLauncher->setEasingCurve(QEasingCurve::InOutCubic);

    m_dockInter = new DockInter("com.deepin.dde.daemon.Dock",
                                "/com/deepin/dde/daemon/Dock",
                                QDBusConnection::sessionBus(),
                                this);
}

void MainFrame::initConnect()
{
    connect(m_desktopWidget, &QDesktopWidget::resized, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
    connect(m_desktopWidget, &QDesktopWidget::primaryScreenChanged, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
  connect(m_desktopWidget, &QDesktopWidget::workAreaResized, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
    connect(m_dockInter, &DockInter::DisplayModeChanged, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
    connect(m_dockInter, &DockInter::HideModeChanged, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
    connect(m_dockInter, &DockInter::PositionChanged, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
    connect(m_dockInter, &DockInter::IconSizeChanged, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
    connect(m_dockInter, &DockInter::FrontendWindowRectChanged, this, &MainFrame::delayedScreenChanged, Qt::QueuedConnection);
}


void MainFrame::initAnimation()
{
    m_launchAni = new QPropertyAnimation(this, "pos", this);
    m_launchAni->setDuration(1000);
    m_launchAni->setEasingCurve(QEasingCurve::OutBounce);

//    connect(m_showWithLauncher, &QPropertyAnimation::valueChanged, this, [=](const QVariant &value) {
//        move(value.toPoint());
//        update();
//    });

//    connect(m_hideWithLauncher, &QPropertyAnimation::valueChanged, this, [=](const QVariant &value) {
//        move(value.toPoint());
//        update();
//    });
}

void MainFrame::showSetting()
{
    QTimer::singleShot(1, m_mainPanel, &dtb::MainPanel::showSettingDialog);
}

void MainFrame::delayedScreenChanged()
{
    QTimer::singleShot(1000, this, &MainFrame::screenChanged);
}

void MainFrame::screenChanged()
{
    xcb_ewmh_connection_t m_ewmh_connection;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(QX11Info::connection(), &m_ewmh_connection);
    xcb_ewmh_init_atoms_replies(&m_ewmh_connection, cookie, NULL);

    xcb_atom_t atoms[1];
    atoms[0] = m_ewmh_connection._NET_WM_WINDOW_TYPE_DOCK;
    xcb_ewmh_set_wm_window_type(&m_ewmh_connection, winId(), 1, atoms);

    xcb_ewmh_wm_strut_partial_t strutPartial;
    memset(&strutPartial, 0, sizeof(xcb_ewmh_wm_strut_partial_t));

    // clear strut partial
    xcb_ewmh_set_wm_strut_partial(&m_ewmh_connection, winId(), strutPartial);

    // set strct partial
    xcb_ewmh_wm_strut_partial_t strut_partial;
    memset(&strut_partial, 0, sizeof(xcb_ewmh_wm_strut_partial_t));

    const QRect dockRect = m_dockInter->frontendWindowRect();
    const QRect primaryRect = QApplication::primaryScreen()->geometry();

    switch (m_dockInter->position()) {
    case DOCK_POS_BOTTOM:
        strut_partial.top = TOPHEIGHT * devicePixelRatioF();
        strut_partial.top_start_x = primaryRect.x();
        strut_partial.top_end_x = primaryRect.x() + primaryRect.width();

        setFixedSize(primaryRect.width(), TOPHEIGHT);
        move(primaryRect.x(), primaryRect.y());
        m_mainPanel->resize(primaryRect.width(), TOPHEIGHT);
        m_mainPanel->move(0, 0);
        break;
    case DOCK_POS_LEFT:
        strut_partial.top = TOPHEIGHT * devicePixelRatioF();
        strut_partial.top_start_x = primaryRect.x();
        strut_partial.top_end_x = primaryRect.x() + primaryRect.width();

        if (dockRect.bottomRight().y() / devicePixelRatioF() + TOPHEIGHT >= primaryRect.height()) {
            m_mainPanel->resize(primaryRect.width() - dockRect.topRight().x() / devicePixelRatioF(), TOPHEIGHT);
            m_mainPanel->move(0, 0);
            setFixedSize(primaryRect.width() - dockRect.topRight().x() / devicePixelRatioF(), TOPHEIGHT);
            move(primaryRect.x() + dockRect.topRight().x() / devicePixelRatioF(), primaryRect.y());
        }
        else {
            m_mainPanel->resize(primaryRect.width(), TOPHEIGHT);
            m_mainPanel->move(0, 0);
            setFixedSize(primaryRect.width(), TOPHEIGHT);
            move(primaryRect.x(), primaryRect.y());
        }
        break;
    case DOCK_POS_RIGHT:
        strut_partial.top = TOPHEIGHT * devicePixelRatioF();
        strut_partial.top_start_x = primaryRect.x();
        strut_partial.top_end_x = primaryRect.x() + primaryRect.width();

        if (dockRect.height() / devicePixelRatioF() + TOPHEIGHT >= primaryRect.height()) {
            setFixedSize((dockRect.topLeft().x() - primaryRect.x()) / devicePixelRatioF(), TOPHEIGHT);
            m_mainPanel->resize((dockRect.topLeft().x() - primaryRect.x()) / devicePixelRatioF(), TOPHEIGHT);
        }
        else {
            setFixedSize(primaryRect.width(), TOPHEIGHT);
            m_mainPanel->resize(primaryRect.width(), TOPHEIGHT);
        }

        move(primaryRect.x(), primaryRect.y());
        m_mainPanel->move(0, 0);
        break;
    case DOCK_POS_TOP:
        strut_partial.top = TOPHEIGHT * devicePixelRatioF() + dockRect.height() / devicePixelRatioF();
        strut_partial.top_start_x = primaryRect.x();
        strut_partial.top_end_x = primaryRect.x() + primaryRect.width();

        move(primaryRect.x(), primaryRect.y() + dockRect.bottomRight().y() / devicePixelRatioF());
        setFixedSize(primaryRect.width(), TOPHEIGHT);
        m_mainPanel->resize(primaryRect.width(), TOPHEIGHT);
        m_mainPanel->move(0, 0);
        break;
    default:
        break;
    }

    xcb_ewmh_set_wm_strut_partial(&m_ewmh_connection, winId(), strut_partial);
}

#if (DTK_VERSION >= DTK_VERSION_CHECK(2, 0, 9, 10))
void MainFrame::onWindowListChanged()
{
    QVector<quint32> windowList = DWindowManagerHelper::instance()->currentWorkspaceWindowIdList();

    for (WId wid : windowList) {
        if (wid == this->window()->winId() || m_windowList.keys().contains(wid)) continue;

        DForeignWindow *window = DForeignWindow::fromWinId(wid);
        if (SKIP_APP.contains(window->wmClass())) {
            window->deleteLater();
            continue;
        }

        m_windowList[wid] = window;

        connect(window, &DForeignWindow::windowStateChanged, this, &MainFrame::onWindowStateChanged, Qt::QueuedConnection);
        connect(window, &DForeignWindow::yChanged, this, [=] {
            onWindowPosChanged(window);
        }, Qt::QueuedConnection);

        emit window->windowStateChanged(window->windowState());
        emit window->yChanged(window->y());
    }

    for (auto it = m_windowList.begin(); it != m_windowList.end();) {
        if (windowList.contains(it.key())) {
            ++it;
        }
        else {
            it.value()->deleteLater();
            it = m_windowList.erase(it);
        }
    }
}
#endif

void MainFrame::onWindowStateChanged(Qt::WindowState windowState)
{
    DForeignWindow *w = static_cast<DForeignWindow *>(sender());
    Q_ASSERT(w);

    WId wid = w->winId();

    auto checkWindow = [&] (DForeignWindow* window) -> bool {
        const qreal ratio { devicePixelRatioF() };
        const bool isMaxWindow { window->windowState() == Qt::WindowMaximized };
        const QRect rect { qApp->primaryScreen()->geometry() };
        const QRect winRect { QRect(window->geometry().topLeft(), window->geometry().size()) };

        return rect.contains(winRect) && isMaxWindow;
    };

    if (checkWindow(w)) {
        if (!m_maxWindowList.contains(wid)) {
            m_maxWindowList << wid;
        }
    }
    else {
        if (m_maxWindowList.contains(wid)) {
            m_maxWindowList.removeOne(wid);
        }
    }

    updateBackground();
}

void MainFrame::onWindowPosChanged(DForeignWindow *window)
{
    const QRect rect{ geometry().adjusted(0, 0, 0, 25) };
    const QRect winRect{ window->geometry().topLeft() * devicePixelRatioF(),
                         window->geometry().size() };
    const WId   wid{ window->winId() };

    if (window->isVisible() && rect.contains(winRect.topLeft())) {
        if (!m_overlapping.contains(wid)) {
            m_overlapping << window->winId();
        }
    }
    else {
        m_overlapping.removeOne(window->winId());
    }

    updateBackground();
}

void MainFrame::updateBackground() {
    QVector<quint32> windowList = DWindowManagerHelper::instance()->currentWorkspaceWindowIdList();
    for (auto it = m_maxWindowList.begin(); it != m_maxWindowList.end();) {
        if (!windowList.contains(*it)) {
            it = m_maxWindowList.erase(it);
        }
        else {
            ++it;
        }
    }

    for (auto it = m_overlapping.begin(); it != m_overlapping.end();) {
        if (!windowList.contains(*it)) {
            it = m_overlapping.erase(it);
        }
        else {
            ++it;
        }
    }

    if (m_maxWindowList.isEmpty() && m_overlapping.isEmpty()) {
        m_mainPanel->setBackground(QColor(0, 0, 0, 0));
    }
    else {
        m_mainPanel->setBackground(QColor(0, 0, 0, 255));
    }
}
