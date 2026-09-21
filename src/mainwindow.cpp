#include "mainwindow.h"
#include "codeeditor.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
class PathLabel final : public QLabel {
public:
    explicit PathLabel(QWidget *parent) : QLabel(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    QSize sizeHint() const override { return {200, fontMetrics().height() + 8}; }
    QSize minimumSizeHint() const override { return {0, fontMetrics().height() + 8}; }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setPen(palette().color(QPalette::WindowText));
        const QRect area = rect().adjusted(8, 0, -8, 0);
        painter.drawText(area, Qt::AlignLeft | Qt::AlignVCenter,
                         fontMetrics().elidedText(text(), Qt::ElideMiddle, qMax(0, area.width())));
    }
};
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_tabs(new QTabWidget(this)) {
    resize(1080, 720);
    m_tabs->setDocumentMode(true);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    setCentralWidget(m_tabs);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this] { refreshStatus(); if (editor()) editor()->setFocus(); });

    auto *header = addToolBar(tr("Document"));
    header->setMovable(false);
    m_path = new PathLabel(this);
    header->addWidget(m_path);
    auto *menuButton = new QToolButton(this);
    menuButton->setText(tr("Menu"));
    menuButton->setPopupMode(QToolButton::InstantPopup);
    auto *menu = new QMenu(menuButton);
    menuButton->setMenu(menu);
    header->addWidget(menuButton);
    const auto action = [this, menu](const QString &text, const QKeySequence &shortcut, auto handler) {
        auto *item = menu->addAction(text);
        item->setShortcut(shortcut);
        addAction(item);
        connect(item, &QAction::triggered, this, handler);
    };
    action(tr("New"), QKeySequence::New, [this] { newFile(); });
    action(tr("Open…"), QKeySequence::Open, [this] {
        const auto paths = QFileDialog::getOpenFileUrls(this, tr("Open file"), {}, tr("All files (*)"));
        for (const auto &path : paths) open(path);
    });
    action(tr("Save"), QKeySequence::Save, [this] { saveEditor(editor()); });
    action(tr("Save As…"), QKeySequence::SaveAs, [this] { saveEditor(editor(), true); });
    action(tr("Close tab"), QKeySequence::Close, [this] { closeTab(m_tabs->currentIndex()); });
    action(tr("Find and replace…"), QKeySequence::Find, [this] { showFind(); });
    action(tr("Next tab"), QKeySequence::NextChild, [this] { setCurrentIndex((m_tabs->currentIndex() + 1) % tabCount()); });
    action(tr("Previous tab"), QKeySequence::PreviousChild, [this] { setCurrentIndex((m_tabs->currentIndex() + tabCount() - 1) % tabCount()); });
    menu->addSeparator();
    action(tr("About qOmaedit"), {}, [this] {
        QMessageBox::about(this, tr("About qOmaedit"), tr("qOmaedit %1<br>Licensed under the MIT License.<br><a href=\"https://github.com/seth-reee/qOmaedit\">GitHub repository</a>").arg(qApp->applicationVersion()));
    });

    auto *settings = new QPushButton(tr("Settings"), this);
    connect(settings, &QPushButton::clicked, this, &MainWindow::showSettings);
    statusBar()->addWidget(settings);
    m_status = new QLabel(this);
    statusBar()->addWidget(m_status, 1);
    m_wrap = new QPushButton(tr("Wrap: Off"), this);
    connect(m_wrap, &QPushButton::clicked, this, [this] {
        m_wrapping = !m_wrapping;
        m_wrap->setText(m_wrapping ? tr("Wrap: On") : tr("Wrap: Off"));
        for (int i = 0; i < tabCount(); ++i) editorAt(i)->setWrapEnabled(m_wrapping);
    });
    statusBar()->addPermanentWidget(m_wrap);
    connect(&m_theme, &Theme::changed, this, &MainWindow::applyTheme);
    connect(&m_settings, &Settings::tabsEnabledChanged, this, &MainWindow::applySettings);
    connect(&m_settings, &Settings::syntaxEnabledChanged, this, &MainWindow::applySettings);
    connect(&m_settings, &Settings::wrappedLineSpacingChanged, this, &MainWindow::applySettings);
    newFile();
    applySettings();
    applyTheme();
}

CodeEditor *MainWindow::editorAt(int index) const { return qobject_cast<CodeEditor *>(m_tabs->widget(index)); }
CodeEditor *MainWindow::editor() const { return editorAt(m_tabs->currentIndex()); }
int MainWindow::tabCount() const { return m_tabs->count(); }
void MainWindow::setCurrentIndex(int index) { m_tabs->setCurrentIndex(index); }

CodeEditor *MainWindow::createEditor() {
    auto *edit = new CodeEditor(&m_theme, m_tabs);
    edit->setSyntaxEnabled(m_settings.syntaxEnabled());
    edit->setWrapEnabled(m_wrapping);
    edit->setWrapGuides(m_settings.wrappedLineSpacing());
    connect(edit, &CodeEditor::error, this, &MainWindow::showError);
    connect(edit, &CodeEditor::fileChanged, this, [this, edit] { refreshTab(edit); });
    connect(edit, &QPlainTextEdit::modificationChanged, this, [this, edit] { refreshTab(edit); });
    connect(edit, &QPlainTextEdit::textChanged, this, [this, edit] { if (edit == editor()) refreshStatus(); });
    return edit;
}

void MainWindow::newFile() {
    if (editor() && editor()->filePath().isEmpty() && editor()->document()->isEmpty() && !editor()->document()->isModified()) return;
    auto *edit = createEditor();
    m_tabs->setCurrentIndex(m_tabs->addTab(edit, edit->fileName()));
    edit->setFocus();
}

bool MainWindow::open(const QUrl &url) {
    const QString path = CodeEditor::localPath(url);
    for (int i = 0; i < tabCount(); ++i) {
        if (!path.isEmpty() && editorAt(i)->filePath() == path) { setCurrentIndex(i); return true; }
    }
    auto *edit = createEditor();
    if (!edit->load(url)) { delete edit; return false; }
    auto *empty = editor();
    if (empty && empty->filePath().isEmpty() && empty->document()->isEmpty() && !empty->document()->isModified()) {
        const int index = m_tabs->currentIndex();
        m_tabs->removeTab(index);
        delete empty;
        m_tabs->insertTab(index, edit, edit->fileName());
        setCurrentIndex(index);
    } else setCurrentIndex(m_tabs->addTab(edit, edit->fileName()));
    refreshStatus();
    return true;
}

bool MainWindow::saveEditorAs(CodeEditor *edit, const QUrl &url) {
    if (!edit || url.isEmpty()) return false;
    const QString path = CodeEditor::localPath(url);
    for (int i = 0; i < tabCount(); ++i) {
        if (editorAt(i) != edit && !path.isEmpty() && editorAt(i)->filePath() == path) {
            showError(tr("This file is already open in another tab.")); return false;
        }
    }
    return edit->saveAs(url);
}
bool MainWindow::saveEditor(CodeEditor *edit, bool saveAs) {
    if (!edit) return false;
    if (saveAs || edit->filePath().isEmpty()) return saveEditorAs(edit, chooseSavePath(edit));
    return edit->save();
}
QUrl MainWindow::chooseSavePath(CodeEditor *edit) {
    return QFileDialog::getSaveFileUrl(this, tr("Save file"), QUrl::fromLocalFile(edit->filePath()), tr("All files (*)"));
}
MainWindow::CloseChoice MainWindow::confirmClose(CodeEditor *edit) {
    const auto choice = QMessageBox::warning(this, tr("Unsaved changes"), tr("Save changes to %1?").arg(edit->fileName()),
                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Save) return CloseChoice::Save;
    if (choice == QMessageBox::Discard) return CloseChoice::Discard;
    return CloseChoice::Cancel;
}
bool MainWindow::allowClose(CodeEditor *edit) {
    if (!edit->document()->isModified()) return true;
    m_tabs->setCurrentWidget(edit);
    const auto choice = confirmClose(edit);
    return choice == CloseChoice::Discard || (choice == CloseChoice::Save && saveEditor(edit));
}
bool MainWindow::closeTab(int index) {
    auto *edit = editorAt(index);
    if (!edit || !allowClose(edit)) return false;
    m_tabs->removeTab(index);
    delete edit;
    if (!tabCount()) newFile();
    return true;
}
void MainWindow::closeEvent(QCloseEvent *event) {
    for (int i = 0; i < tabCount(); ++i) {
        if (!allowClose(editorAt(i))) { event->ignore(); return; }
    }
    event->accept();
}
void MainWindow::showError(const QString &message) { QMessageBox::warning(this, tr("Couldn’t read or write file"), message); }

void MainWindow::refreshTab(CodeEditor *edit) {
    const int index = m_tabs->indexOf(edit);
    if (index >= 0) m_tabs->setTabText(index, (edit->document()->isModified() ? QStringLiteral("● ") : QString()) + edit->fileName());
    if (edit == editor()) refreshStatus();
}
void MainWindow::refreshStatus() {
    if (!editor()) return;
    const auto *edit = editor();
    setWindowTitle((edit->document()->isModified() ? QStringLiteral("● ") : QString()) + edit->fileName() + " — qOmaedit");
    const QString path = edit->filePath().isEmpty() ? tr("Untitled") : (m_settings.tabsEnabled() ? QFileInfo(edit->filePath()).absolutePath() : edit->filePath());
    m_path->setText(path);
    m_path->setToolTip(edit->filePath());
    m_status->setText(tr("UTF-8   %1   %2 lines · %3 chars").arg(m_settings.syntaxEnabled() ? edit->language() : QString()).arg(edit->blockCount()).arg(edit->document()->characterCount() - 1));
}
void MainWindow::applyTheme() {
    QPalette p = palette();
    p.setColor(QPalette::Window, m_theme.background());
    p.setColor(QPalette::WindowText, m_theme.foreground());
    p.setColor(QPalette::Base, m_theme.panel());
    p.setColor(QPalette::AlternateBase, m_theme.surface());
    p.setColor(QPalette::Text, m_theme.foreground());
    p.setColor(QPalette::Button, m_theme.surface());
    p.setColor(QPalette::ButtonText, m_theme.foreground());
    p.setColor(QPalette::Highlight, m_theme.selection());
    p.setColor(QPalette::HighlightedText, m_theme.foreground());
    p.setColor(QPalette::ToolTipBase, m_theme.panel());
    p.setColor(QPalette::ToolTipText, m_theme.foreground());
    setPalette(p);
}
void MainWindow::applySettings() {
    m_tabs->tabBar()->setVisible(m_settings.tabsEnabled());
    for (int i = 0; i < tabCount(); ++i) {
        editorAt(i)->setSyntaxEnabled(m_settings.syntaxEnabled());
        editorAt(i)->setWrapGuides(m_settings.wrappedLineSpacing());
    }
    refreshStatus();
}

void MainWindow::showFind() {
    if (!m_findDialog) {
        m_findDialog = new QDialog(this);
        m_findDialog->setWindowTitle(tr("Find and replace"));
        auto *layout = new QFormLayout(m_findDialog);
        m_find = new QLineEdit(m_findDialog);
        m_replace = new QLineEdit(m_findDialog);
        layout->addRow(tr("Find"), m_find);
        layout->addRow(tr("Replace with"), m_replace);
        auto *buttons = new QDialogButtonBox(m_findDialog);
        auto *next = buttons->addButton(tr("Find next"), QDialogButtonBox::ActionRole);
        auto *replace = buttons->addButton(tr("Replace"), QDialogButtonBox::ActionRole);
        auto *all = buttons->addButton(tr("Replace all"), QDialogButtonBox::ActionRole);
        buttons->addButton(QDialogButtonBox::Close);
        layout->addRow(buttons);
        connect(next, &QPushButton::clicked, this, [this] { editor()->findNext(m_find->text()); });
        connect(m_find, &QLineEdit::returnPressed, next, &QPushButton::click);
        connect(replace, &QPushButton::clicked, this, [this] {
            if (!editor()->replaceCurrent(m_find->text(), m_replace->text())) editor()->findNext(m_find->text());
        });
        connect(m_replace, &QLineEdit::returnPressed, replace, &QPushButton::click);
        connect(all, &QPushButton::clicked, this, [this] { editor()->replaceAll(m_find->text(), m_replace->text()); });
        connect(buttons, &QDialogButtonBox::rejected, m_findDialog, &QDialog::hide);
    }
    m_findDialog->show();
    m_findDialog->raise();
    m_find->setFocus();
    m_find->selectAll();
}
void MainWindow::showSettings() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Settings"));
    auto *layout = new QVBoxLayout(&dialog);
    const auto checkbox = [&dialog, layout](const QString &text, bool checked) {
        auto *box = new QCheckBox(text, &dialog);
        box->setChecked(checked);
        layout->addWidget(box);
        return box;
    };
    connect(checkbox(tr("Show tabs"), m_settings.tabsEnabled()), &QCheckBox::toggled, &m_settings, &Settings::setTabsEnabled);
    connect(checkbox(tr("Syntax highlighting"), m_settings.syntaxEnabled()), &QCheckBox::toggled, &m_settings, &Settings::setSyntaxEnabled);
    connect(checkbox(tr("Mark wrapped continuation lines"), m_settings.wrappedLineSpacing()), &QCheckBox::toggled, &m_settings, &Settings::setWrappedLineSpacing);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}
