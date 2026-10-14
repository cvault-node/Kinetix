#include "CodeEditor.hpp"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFont>
#include <QDir>
#include <QFileInfo>
#include <QStringConverter>

CodeEditor::CodeEditor(QWidget *parent)
    : QWidget(parent)
    , m_editor(new QPlainTextEdit(this))
    , m_toolbarLayout(new QHBoxLayout())
    , m_mainLayout(new QVBoxLayout(this))
    , m_btnOpen(new QPushButton(tr("Abrir"), this))
    , m_btnSave(new QPushButton(tr("Guardar"), this))
    , m_btnClear(new QPushButton(tr("Limpiar"), this))
    , m_currentPath()
{
    QFont font(QStringLiteral("Consolas"));
    font.setPointSize(10);
    m_editor->setFont(font);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editor->setTabStopDistance(40);

    m_toolbarLayout->addWidget(m_btnOpen);
    m_toolbarLayout->addWidget(m_btnSave);
    m_toolbarLayout->addWidget(m_btnClear);
    m_toolbarLayout->addStretch();

    m_mainLayout->addLayout(m_toolbarLayout);
    m_mainLayout->addWidget(m_editor);
    setLayout(m_mainLayout);

    connect(m_btnOpen, &QPushButton::clicked, this, &CodeEditor::openFile);
    connect(m_btnSave, &QPushButton::clicked, this, &CodeEditor::saveFile);
    connect(m_btnClear, &QPushButton::clicked, this, [this]() { m_editor->clear(); });
}

QString CodeEditor::title() const
{
    if (!m_currentPath.isEmpty()) {
        return QStringLiteral("Editor: ") + QFileInfo(m_currentPath).fileName();
    }
    return QStringLiteral("Editor: sin_titulo");
}

QPlainTextEdit *CodeEditor::editor() const
{
    return m_editor;
}

void CodeEditor::openFile()
{
    const QString filter = tr("Todos (*.*);;Python (*.py);;HTML (*.html *.htm);;CSS (*.css);;JavaScript (*.js);;JSON (*.json);;Texto (*.txt)");
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir archivo"), QString(), filter);
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), file.errorString());
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    const QString content = in.readAll();
    file.close();

    m_editor->setPlainText(content);
    m_currentPath = path;
}

void CodeEditor::saveFile()
{
    if (m_currentPath.isEmpty()) {
        const QString filter = tr("Python (*.py);;HTML (*.html);;CSS (*.css);;JavaScript (*.js);;JSON (*.json);;Texto (*.txt);;Todos (*.*)");
        const QString path = QFileDialog::getSaveFileName(this, tr("Guardar archivo"), QString(), filter);
        if (path.isEmpty())
            return;
        m_currentPath = path;
    }

    QFile file(m_currentPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), file.errorString());
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << m_editor->toPlainText();
    file.close();

    QMessageBox::information(this, tr("Guardado"), tr("Guardado en:\n%1").arg(m_currentPath));
}