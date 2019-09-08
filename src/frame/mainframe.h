#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <QFrame>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QDesktopWidget>
#include <DBlurEffectWidget>
#include <QPropertyAnimation>
#include <DPlatformWindowHandle>
#include <QPropertyAnimation>
#include <DForeignWindow>
#include <com_deepin_dde_daemon_dock.h>
#include <memory>

#include "mainpanel.h"

using DockInter = com::deepin::dde::daemon::Dock;

DWIDGET_USE_NAMESPACE

class MainFrame : public DBlurEffectWidget
{
    Q_OBJECT
public:
    explicit MainFrame(QWidget *parent = 0);
    ~MainFrame();

public slots:
    void showSetting();

private slots:
    void screenChanged();
#if (DTK_VERSION >= DTK_VERSION_CHECK(2, 0, 9, 10))
    void onWindowListChanged();
#endif
    void onWindowStateChanged(Qt::WindowState windowState);
    void delayedScreenChanged();

private:
    void init();
    void initConnect();
    void initAnimation();
    void onWindowPosChanged(DForeignWindow *window);
    void updateBackground();

private:
    QDesktopWidget *m_desktopWidget;
    dtb::MainPanel *m_mainPanel;
    DPlatformWindowHandle *m_handle;
    QPropertyAnimation *m_launchAni;
//    QPropertyAnimation *m_hideWithLauncher;
//    QPropertyAnimation *m_showWithLauncher;
    DockInter *m_dockInter;

    QMap<WId,DForeignWindow*> m_windowList;
    QList<WId> m_windowIdList;
    QList<WId> m_maxWindowList;
    QList<WId> m_overlapping;
};

#endif // MAINFRAME_H
