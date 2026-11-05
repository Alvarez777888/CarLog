#pragma once
#include <QWidget>
#include <QBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QScroller>
#include <QScreen>
#include <QGuiApplication>
#include <QDialog>

inline bool mobile_ui() {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return qEnvironmentVariableIntValue("CARLOG_MOBILE")==1;
#endif
}
inline void mobile_content(QWidget *page) {
    if (!mobile_ui()) return;
    for (auto *box : page->findChildren<QBoxLayout *>())
        if (box->direction()==QBoxLayout::LeftToRight) box->setDirection(QBoxLayout::TopToBottom);
    for (auto *form : page->findChildren<QFormLayout *>()) form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    for (auto *grid : page->findChildren<QGridLayout *>()) {
        QList<QLayoutItem *> items;
        while (auto *item=grid->takeAt(0)) items.append(item);
        int row=0;
        for (auto *item : items) grid->addItem(item,row++,0);
    }
    for (auto *label : page->findChildren<QLabel *>()) {
        label->setWordWrap(true);
        if (label->objectName()=="pageTitle" || label->objectName()=="FormTitle")
            label->setStyleSheet("background:transparent; color:#18202f; font-size:23px; font-weight:700;");
    }
    for (auto *scroll : page->findChildren<QScrollArea *>())
        QScroller::grabGesture(scroll->viewport(),QScroller::TouchGesture);
}
inline void mobile_dialog(QDialog *dialog) {
    if (!mobile_ui()) return;
    dialog->setMinimumWidth(0);
    dialog->setMaximumWidth(10000);
    auto *layout=dialog->layout();
    if (!layout) return;
    QWidget temporary;
    temporary.setLayout(layout);
    auto *content=new QWidget;
    content->setLayout(layout);
    mobile_content(content);
    auto *scroll=new QScrollArea(dialog);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    QScroller::grabGesture(scroll->viewport(),QScroller::TouchGesture);
    auto *outer=new QVBoxLayout(dialog);
    outer->setContentsMargins(8,8,8,8);
    outer->addWidget(scroll);
    if (auto *screen=QGuiApplication::primaryScreen()) {
        auto available=screen->availableGeometry().size();
        if (auto *parent=dialog->parentWidget()) available=available.boundedTo(parent->window()->size());
        dialog->resize(available);
    }
}
