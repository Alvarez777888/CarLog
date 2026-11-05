#pragma once
#include <QString>
inline QString app_theme() {
return R"(
        QMainWindow {
            background: #f5f7fb;
        }

        QWidget {
    background: #f5f7fb;
    color: #18202f;
    font-family: "DejaVu Sans";
    font-size: 10pt;
}

        QWidget#sidebar {
            background: #151b28;
        }

        QLabel#sideTitle {
            background: transparent;
            color: white;
            font-size: 25px;
            font-weight: 700;
        }

        QLabel#sideSub {
            background: transparent;
            color: #aeb7c7;
            font-size: 9pt;
        }

        QLabel#sideFoot {
            background: transparent;
            color: #7f899b;
            font-size: 8.5pt;
        }

        QPushButton#nav {
            background: transparent;
            color: #cbd3df;
            border: none;
            border-radius: 9px;
            text-align: left;
            padding: 11px 14px;
            margin: 2px 0;
            font-size: 10pt;
        }

        QPushButton#nav:hover {
            background: #252d3d;
            color: white;
        }

        QPushButton#nav:pressed {
            background: #303a4d;
            color: white;
        }

        QPushButton#nav:focus {
            outline: none;
            border: none;
        }

        QPushButton#nav[active="true"] {
            background: #39465d;
            color: white;
            font-weight: 600;
        }

        QPushButton#nav[active="true"]:hover {
            background: #39465d;
            color: white;
        }

        QPushButton#nav[active="true"]:pressed {
            background: #303b50;
            color: white;
        }

        QPushButton#nav[active="true"]:focus {
            background: #39465d;
            color: white;
            outline: none;
            border: none;
        }

        QLabel#pageTitle {
            background: transparent;
            color: #151b28;
            font-size: 26px;
            font-weight: 700;
        }

        QLabel#sub {
            background: transparent;
            color: #6b7484;
            font-size: 10pt;
        }

        QLabel#card {
            background: white;
            color: #18202f;
            border: 1px solid #e4e8ef;
            border-radius: 14px;
            padding: 18px;
            font-size: 12pt;
            font-weight: 600;
        }

        QFrame#section {
            background: white;
            border: 1px solid #e4e8ef;
            border-radius: 14px;
        }

        QFrame#carCard {
            background: white;
            border: 1px solid #e0e5ec;
            border-radius: 15px;
        }

        QPushButton#darkBtn {
            background: #1d2738;
            color: white;
            border: none;
            padding: 10px 17px;
            border-radius: 8px;
            font-weight: 600;
        }

        QPushButton#darkBtn:hover {
            background: #2b374b;
        }

        QPushButton#darkBtn:pressed {
            background: #111827;
        }

        QPushButton#darkBtn:focus {
            outline: none;
            border: none;
        }

        QPushButton#lightBtn {
            background: white;
            color: #263143;
            border: 1px solid #d9dee7;
            padding: 9px 16px;
            border-radius: 8px;
            font-weight: 500;
        }

        QPushButton#lightBtn:hover {
            background: #f1f4f8;
            border-color: #c7ceda;
        }

        QPushButton#lightBtn:pressed {
            background: #e7ebf1;
        }

        QPushButton#lightBtn:focus {
            outline: none;
            border: 1px solid #d9dee7;
        }

        QToolButton#carButton {
    background: white;
    color: #18202f;
    border: 1px solid #dfe4eb;
    border-radius: 16px;
    text-align: left;
    padding: 20px;
    font-family: "DejaVu Sans";
    font-size: 10pt;
    font-weight: 400;
}

QToolButton#carButton:hover {
    background: #f8fafc;
    border: 1px solid #bfc9d8;
}

QToolButton#carButton:pressed {
    background: #eef2f7;
    border: 1px solid #aeb9c9;
}

QToolButton#carButton[selected="true"] {
    background: #f5f8fc;
    border: 2px solid #43546d;
}

QToolButton#carButton[selected="true"]:hover {
    background: #eef3f9;
    border: 2px solid #3c4d66;
}

QToolButton#carButton:focus {
    outline: none;
    border: 1px solid #dfe4eb;
}

QToolButton#carButton[selected="true"]:focus {
    border: 2px solid #43546d;
}

        QComboBox,
        QLineEdit,
        QDateEdit,
        QSpinBox,
        QDoubleSpinBox {
            background: white;
            color: #18202f;
            border: 1px solid #d9dee7;
            border-radius: 7px;
            padding: 7px 9px;
            min-height: 20px;
        }

        QComboBox:hover,
        QLineEdit:hover,
        QDateEdit:hover,
        QSpinBox:hover,
        QDoubleSpinBox:hover {
            border-color: #c5ccd8;
        }

        QComboBox:focus,
        QLineEdit:focus,
        QDateEdit:focus,
        QSpinBox:focus,
        QDoubleSpinBox:focus {
            border: 1px solid #7b879b;
            outline: none;
        }

        QComboBox::drop-down {
            border: none;
            width: 25px;
        }

        QTableWidget {
            background: white;
            border: 1px solid #e4e8ef;
            border-radius: 10px;
            gridline-color: transparent;
            selection-background-color: #e9eef6;
            selection-color: #18202f;
            alternate-background-color: #fafbfc;
        }

        QTableWidget::item {
            padding: 8px;
            border-bottom: 1px solid #eef1f5;
        }

        QTableWidget::item:selected {
            background: #e9eef6;
            color: #18202f;
        }

        QHeaderView::section {
            background: #f4f6f9;
            color: #566173;
            border: none;
            border-bottom: 1px solid #e2e6ec;
            padding: 10px;
            font-weight: 600;
        }

        QGroupBox {
            background: white;
            border: 1px solid #e4e8ef;
            border-radius: 12px;
            margin-top: 10px;
            padding: 18px;
            font-weight: 600;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 14px;
            padding: 0 6px;
            color: #303b4e;
            background: white;
        }

        QScrollArea {
            border: none;
            background: transparent;
        }

        QDialog {
            background: #f5f7fb;
        }

        QMessageBox {
            background: white;
        }
    )";
}
