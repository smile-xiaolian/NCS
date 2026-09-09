#pragma once

#include <QString>

namespace ncs {

// 管理端统一视觉样式,参考 my_device_link_sim 模拟器的浅色圆角设计语言。
// 背景柔和渐变、白色圆角卡片、蓝色主操作、语义色状态,整体简洁而有层次。
inline QString adminStyleSheet()
{
    return QStringLiteral(R"QSS(
/* ========== 全局 ========== */
* {
    font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI", sans-serif;
    font-size: 13px;
}

QMainWindow {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                stop:0 #E9EFF8, stop:1 #F4F7FB);
}
QStackedWidget, QWidget#loginPage, QWidget#workspace, QWidget#pageHost {
    background: transparent;
}

/* ========== 侧边栏 ========== */
QFrame#sidebar {
    background: #FFFFFF;
    border: none;
    border-right: 1px solid #E7ECF4;
}
QLabel#brandTitle {
    color: #1F2733;
    font-size: 16px;
    font-weight: 700;
}
QLabel#brandSub {
    color: #7A8699;
    font-size: 11px;
}
QLabel#brandLogo {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                stop:0 #5B8DFF, stop:1 #3D6CF0);
    color: #FFFFFF;
    font-size: 19px;
    font-weight: 700;
    border-radius: 11px;
}
QLabel#sectionLabel {
    color: #98A2B3;
    font-size: 11px;
    font-weight: 600;
}

QListWidget#nav {
    background: transparent;
    border: none;
    outline: 0;
}
QListWidget#nav::item {
    color: #4A5568;
    border-radius: 9px;
    padding: 10px 14px;
    margin: 2px 10px;
    font-size: 14px;
}
QListWidget#nav::item:hover {
    background: #EEF3FB;
    color: #2F6BED;
}
QListWidget#nav::item:selected {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                stop:0 #4C8DFF, stop:1 #4F74F0);
    color: #FFFFFF;
    font-weight: 600;
}

QFrame#accountCard {
    background: #F7F9FC;
    border: 1px solid #E6EBF3;
    border-radius: 12px;
}
QLabel#accountName {
    color: #26324B;
    font-size: 13px;
    font-weight: 600;
}
QLabel#accountRole {
    color: #98A2B3;
    font-size: 11px;
}
QLabel#onlineDot {
    background: #34C77B;
    border-radius: 4px;
}

/* ========== 页面标题 ========== */
QLabel#pageTitle {
    color: #1F2733;
    font-size: 21px;
    font-weight: 700;
}
QLabel#pageSub {
    color: #7A8699;
    font-size: 12px;
}

/* ========== 卡片面板 ========== */
QFrame#card {
    background: #FFFFFF;
    border: 1px solid #E4EAF2;
    border-radius: 14px;
}
QFrame#panelTick {
    background: #4C8DFF;
    border-radius: 2px;
}
QLabel#panelTitle {
    color: #1F2733;
    font-size: 14px;
    font-weight: 600;
}
QLabel#panelSub, QLabel#filterLabel {
    color: #8A94A6;
    font-size: 12px;
}

QFrame#metricCard {
    background: #FFFFFF;
    border: 1px solid #E4EAF2;
    border-radius: 14px;
}
QFrame#metricCard:hover {
    border-color: #B9CEF2;
    background: #FCFDFF;
}
QLabel#metricLabel {
    color: #7A8699;
    font-size: 12px;
}
QLabel#metricValue {
    font-size: 25px;
    font-weight: 700;
}

/* ========== 按钮 ========== */
QPushButton {
    background: #FFFFFF;
    border: 1px solid #D6DEE9;
    border-radius: 8px;
    padding: 7px 15px;
    color: #3A4459;
    font-size: 13px;
}
QPushButton:hover {
    background: #F2F6FC;
    border-color: #B9CEF2;
    color: #2F6BED;
}
QPushButton:pressed {
    background: #E9F1FE;
}
QPushButton:disabled {
    color: #B9C2CF;
    background: #F4F6FA;
    border-color: #E7EBF2;
}
QPushButton#accent {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #4C8DFF, stop:1 #3D7CF2);
    color: #FFFFFF;
    border: none;
}
QPushButton#accent:hover {
    background: #3D7CF2;
    color: #FFFFFF;
}
QPushButton#accent:pressed {
    background: #2E66C9;
}
QPushButton#danger {
    color: #D64545;
}
QPushButton#danger:hover {
    background: #FDEFED;
    border-color: #EBA8A0;
    color: #C0392B;
}

/* ========== 输入控件 ========== */
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background: #FFFFFF;
    border: 1px solid #D6DEE9;
    border-radius: 8px;
    padding: 6px 10px;
    color: #26324B;
    selection-background-color: #CFE0FB;
    selection-color: #1F2733;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #4C8DFF;
    background: #FFFFFF;
}
QLineEdit:disabled, QComboBox:disabled {
    background: #F4F6FA;
    color: #B9C2CF;
}
QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {
    width: 22px;
    background: #EEF2F8;
    border: 1px solid #E2E8F2;
    border-radius: 4px;
    margin: 2px;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background: #DDE8F7;
    border-color: #C4D6F2;
}
QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed,
QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed {
    background: #C6D8F5;
    border-color: #A9C4EC;
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
    image: url(:/icons/spin_up.png);
    width: 10px;
    height: 10px;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
    image: url(:/icons/spin_down.png);
    width: 10px;
    height: 10px;
}
QComboBox::drop-down {
    border: none;
    width: 24px;
}
QComboBox QAbstractItemView {
    background: #FFFFFF;
    border: 1px solid #DCE3EE;
    border-radius: 8px;
    padding: 4px;
    outline: none;
    selection-background-color: #E9F1FE;
    selection-color: #1F2733;
}
QComboBox QAbstractItemView::item {
    min-height: 30px;
    padding: 2px 8px;
    border-radius: 6px;
}

/* ========== 表格 ========== */
QTableView {
    background: #FFFFFF;
    alternate-background-color: #F8FAFD;
    border: none;
    gridline-color: transparent;
    outline: none;
}
QTableView::item {
    padding: 8px 10px;
    border-bottom: 1px solid #EEF2F8;
    color: #3A4459;
}
QTableView::item:hover {
    background: #F5F8FE;
}
QTableView::item:selected {
    background: #E9F1FE;
    color: #1F2733;
}
QHeaderView {
    background: #F3F6FA;
}
QHeaderView::section {
    background: #F3F6FA;
    border: none;
    border-bottom: 1px solid #E7EDF5;
    padding: 11px 10px;
    color: #667085;
    font-size: 12px;
    font-weight: 600;
}
QTableCornerButton::section {
    background: #F3F6FA;
    border: none;
}

/* ========== 滚动条 / 分割条 ========== */
QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: #C9D3E2;
    border-radius: 4px;
    min-height: 28px;
}
QScrollBar::handle:vertical:hover {
    background: #AAB8CC;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: transparent;
    height: 8px;
    margin: 2px;
}
QScrollBar::handle:horizontal {
    background: #C9D3E2;
    border-radius: 4px;
    min-width: 28px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}
QSplitter::handle {
    background: transparent;
}

/* ========== 状态栏 / 提示 / 对话框 ========== */
QStatusBar {
    background: #FFFFFF;
    border-top: 1px solid #E7ECF4;
    color: #7A8699;
    font-size: 12px;
}
QStatusBar::item {
    border: none;
}
QDialog, QMessageBox {
    background: #FFFFFF;
}
QMessageBox {
    min-height: 320px;
}
QMessageBox QLabel {
    min-width: 360px;
    min-height: 110px;
    font-size: 14px;
}
QMessageBox QPushButton {
    min-width: 88px;
    min-height: 34px;
}
QToolTip {
    background: #26324B;
    color: #F2F6FC;
    border: none;
    padding: 5px 8px;
    border-radius: 6px;
    font-size: 12px;
}
QMenu {
    background: #FFFFFF;
    border: 1px solid #DCE3EE;
    border-radius: 8px;
    padding: 4px;
    color: #3A4459;
}
QMenu::item {
    padding: 6px 18px;
    border-radius: 6px;
}
QMenu::item:selected {
    background: #E9F1FE;
    color: #1F2733;
}

/* ========== 登录页 ========== */
QFrame#loginCard {
    background: #FFFFFF;
    border: 1px solid #E3E9F3;
    border-radius: 18px;
}
QPushButton#loginButton {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #4C8DFF, stop:1 #3D6CF0);
    color: #FFFFFF;
    border: none;
    border-radius: 10px;
    padding: 11px 0;
    font-size: 15px;
    font-weight: 600;
}
QPushButton#loginButton:hover {
    background: #3D7CF2;
}
QPushButton#loginButton:pressed {
    background: #2E66C9;
}
QLabel#loginTitle {
    color: #1F2733;
    font-size: 22px;
    font-weight: 700;
}
QLabel#loginSub {
    color: #8A94A6;
    font-size: 12px;
}
QLabel#loginHint {
    color: #98A2B3;
    font-size: 11px;
}
QFrame#divider {
    background: #EDF1F7;
}
)QSS");
}

} // namespace ncs
