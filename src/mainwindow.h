#pragma once
#include <QMainWindow>
#include <QUrl>
#include "settings.h"
#include "theme.h"

class CodeEditor;
class QTabWidget;
class QLabel;
class QDialog;
class QLineEdit;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    CodeEditor *editor() const;
    CodeEditor *editorAt(int index) const;
    int tabCount() const;
    void setCurrentIndex(int index);
    void newFile();
    bool open(const QUrl &url);
    bool saveEditor(CodeEditor *editor, bool saveAs = false);
    bool saveEditorAs(CodeEditor *editor, const QUrl &url);
    bool closeTab(int index);
    enum class CloseChoice { Save, Discard, Cancel };
protected:
    void closeEvent(QCloseEvent *event) override;
    virtual CloseChoice confirmClose(CodeEditor *editor);
    virtual QUrl chooseSavePath(CodeEditor *editor);
    virtual void showError(const QString &message);
private:
    CodeEditor *createEditor();
    bool allowClose(CodeEditor *editor);
    void refreshTab(CodeEditor *editor);
    void refreshStatus();
    void applyTheme();
    void showFind();
    void showSettings();
    void applySettings();
    Theme m_theme;
    Settings m_settings;
    QTabWidget *m_tabs;
    QLabel *m_path;
    QLabel *m_status;
    QPushButton *m_wrap;
    QDialog *m_findDialog = nullptr;
    QLineEdit *m_find = nullptr;
    QLineEdit *m_replace = nullptr;
    bool m_wrapping = false;
};
