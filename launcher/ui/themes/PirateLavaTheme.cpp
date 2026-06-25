// SPDX-License-Identifier: GPL-3.0-only
#include "PirateLavaTheme.h"

#include <QObject>

QString PirateLavaTheme::id()
{
    return "pirate_lava";
}

QString PirateLavaTheme::name()
{
    return QObject::tr("Pirate Lava");
}

QString PirateLavaTheme::tooltip()
{
    return QObject::tr("A dark pirate theme with lava highlights, gold trim, and retro-styled controls.");
}

QPalette PirateLavaTheme::colorScheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(22, 32, 42));
    palette.setColor(QPalette::WindowText, QColor(235, 223, 202));
    palette.setColor(QPalette::Base, QColor(16, 21, 26));
    palette.setColor(QPalette::AlternateBase, QColor(27, 35, 41));
    palette.setColor(QPalette::ToolTipBase, QColor(245, 210, 122));
    palette.setColor(QPalette::ToolTipText, QColor(20, 16, 14));
    palette.setColor(QPalette::Text, QColor(235, 223, 202));
    palette.setColor(QPalette::Button, QColor(31, 42, 49));
    palette.setColor(QPalette::ButtonText, QColor(245, 210, 122));
    palette.setColor(QPalette::BrightText, QColor(255, 112, 45));
    palette.setColor(QPalette::Link, QColor(255, 171, 64));
    palette.setColor(QPalette::Highlight, QColor(242, 117, 43));
    palette.setColor(QPalette::HighlightedText, QColor(18, 12, 10));
    palette.setColor(QPalette::PlaceholderText, QColor(139, 129, 112));
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double PirateLavaTheme::fadeAmount()
{
    return 0.45;
}

QColor PirateLavaTheme::fadeColor()
{
    return QColor(22, 32, 42);
}

bool PirateLavaTheme::hasStyleSheet()
{
    return true;
}

QString PirateLavaTheme::appStyleSheet()
{
    return QStringLiteral(
        "QToolTip {"
        "  color: #14100e;"
        "  background-color: #f5d27a;"
        "  border: 1px solid #f29e35;"
        "}"
        "QMainWindow, QDialog { background: #16202a; }"
        "QMenuBar, QMenu {"
        "  background: #12191f;"
        "  color: #eadfca;"
        "  border: 1px solid #5e4724;"
        "}"
        "QMenuBar::item:selected, QMenu::item:selected {"
        "  background: #4b2a1f;"
        "  color: #f5d27a;"
        "}"
        "QToolBar {"
        "  background: #111920;"
        "  border: 0;"
        "  border-bottom: 2px solid #c66a24;"
        "  spacing: 4px;"
        "}"
        "QStatusBar {"
        "  background: #111920;"
        "  color: #d7c7a2;"
        "  border-top: 1px solid #7b5b28;"
        "}"
        "QPushButton, QToolButton {"
        "  background: #1f2a31;"
        "  color: #f5d27a;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "  padding: 5px 9px;"
        "}"
        "QPushButton:hover, QToolButton:hover { background: #2d3d45; border-color: #f29e35; }"
        "QPushButton:pressed, QToolButton:pressed { background: #4b2a1f; }"
        "QPushButton:disabled, QToolButton:disabled { color: #7a746b; border-color: #3b342d; background: #1a2025; }"
        "QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QSpinBox {"
        "  background: #10151a;"
        "  color: #eadfca;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #f29e35; }"
        "QTabWidget::pane { border: 1px solid #7b5b28; border-radius: 6px; }"
        "QTabBar::tab {"
        "  background: #17222a;"
        "  color: #d7c7a2;"
        "  border: 1px solid #5e4724;"
        "  border-bottom: none;"
        "  padding: 7px 13px;"
        "}"
        "QTabBar::tab:selected { background: #1c2d39; color: #f5d27a; border-top: 2px solid #f29e35; }"
        "QTabBar::tab:hover:!selected { background: rgba(242, 158, 53, 36); }"
        "QTreeView, QListView, QTableView {"
        "  background: #10151a;"
        "  alternate-background-color: #1b2329;"
        "  color: #eadfca;"
        "  border: 1px solid #4f3d23;"
        "  border-radius: 6px;"
        "}"
        "QTreeView::item:hover, QListView::item:hover, QTableView::item:hover { background: rgba(242, 158, 53, 32); }"
        "QTreeView::item:selected, QListView::item:selected, QTableView::item:selected {"
        "  background: #284b58;"
        "  color: #ffffff;"
        "}"
        "QGroupBox {"
        "  border: 1px solid #5e4724;"
        "  border-radius: 6px;"
        "  margin-top: 10px;"
        "  color: #f5d27a;"
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QScrollBar:vertical, QScrollBar:horizontal { background: #10151a; border: 0; margin: 0; }"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal { background: #7b5b28; border-radius: 4px; min-height: 24px; min-width: 24px; }"
        "QScrollBar::handle:hover { background: #f29e35; }"
        "QSplitter::handle { background: #5e4724; }"
        "#legacyLavaStrip, #flameLavaStrip {"
        "  border: 1px solid #6d2c18;"
        "  border-radius: 6px;"
        "  background: #2b110c;"
        "}");
}

LogColors PirateLavaTheme::logColorScheme()
{
    auto colors = defaultLogColors(colorScheme());
    colors.foreground[MessageLevel::Launcher] = QColor(245, 210, 122);
    colors.foreground[MessageLevel::Debug] = QColor(122, 198, 172);
    colors.foreground[MessageLevel::Warning] = QColor(255, 171, 64);
    colors.foreground[MessageLevel::Error] = QColor(255, 96, 64);
    colors.background[MessageLevel::Fatal] = QColor(20, 12, 10);
    return colors;
}
