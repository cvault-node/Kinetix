#ifndef CODEEDITOR_HPP
#define CODEEDITOR_HPP

#include <QWidget>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QString>

class CodeEditor : public QWidget
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    QString title() const;
    QPlainTextEdit *editor() const;

public slots:
    void openFile();
    void saveFile();

private:
    QPlainTextEdit *m_editor;
    QHBoxLayout *m_toolbarLayout;
    QVBoxLayout *m_mainLayout;
    QPushButton *m_btnOpen;
    QPushButton *m_btnSave;
    QPushButton *m_btnClear;
    QString m_currentPath;
};

#endif // CODEEDITOR_HPP