import os

# Fix AuthWidget.h
path = 'd:/confres/src/ui/AuthWidget.h'
with open(path, 'r', encoding='utf-8') as f:
    c = f.read()
if 'void keyPressEvent' not in c:
    c = c.replace('void loginFailed();\n', 'void loginFailed();\n\nprotected:\n    void keyPressEvent(QKeyEvent* event) override;\n')
with open(path, 'w', encoding='utf-8') as f:
    f.write(c)

# Fix AuthWidget.cpp
path = 'd:/confres/src/ui/AuthWidget.cpp'
with open(path, 'r', encoding='utf-8') as f:
    c = f.read()

if 'setFocusPolicy' not in c:
    c = c.replace('grid->addWidget(m_keyboard, 4, 0, 1, 4);\n}', 'grid->addWidget(m_keyboard, 4, 0, 1, 4);\n    setFocusPolicy(Qt::StrongFocus);\n}')

if 'void AuthWidget::keyPressEvent' not in c:
    append_str = """
void AuthWidget::keyPressEvent(QKeyEvent* event) {
    if (m_mode == Mode::Registration) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) {
        onPinDigitPressed(event->key() - Qt::Key_0);
        event->accept();
    } else if (event->key() == Qt::Key_Backspace) {
        onPinBackspace();
        event->accept();
    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (m_pinBuffer.length() == 4) submitPin();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}
"""
    # Insert before the last closing brace and namespace Kirana
    c = c.replace('\n} // namespace Kirana', append_str + '\n} // namespace Kirana')

with open(path, 'w', encoding='utf-8') as f:
    f.write(c)

