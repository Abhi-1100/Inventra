import os

# Fix Sidebar.cpp
path = 'd:/confres/src/ui/Sidebar.cpp'
with open(path, 'r', encoding='utf-8') as f:
    c = f.read()
c = c.replace('setFixedSize(160, 44);', 'setFixedSize(260, 44);')
c = c.replace('setFixedWidth(160);', 'setFixedWidth(260);')
c = c.replace('setFixedWidth(240);', 'setFixedWidth(260);')
c = c.replace('f.setFamily(QStringLiteral("Segoe UI"));', 'f.setFamily(QStringLiteral("Hanken Grotesk"));')
c = c.replace('f.setPixelSize(12);', 'f.setPixelSize(13);')
# Remove stylesheet
import re
c = re.sub(r'setStyleSheet\(QStringLiteral\(\s*"QWidget#Sidebar.*?\) \}\"\)\);', '', c, flags=re.DOTALL)
with open(path, 'w', encoding='utf-8') as f:
    f.write(c)

# Fix MetricCard.cpp
path = 'd:/confres/src/widgets/MetricCard.cpp'
with open(path, 'r', encoding='utf-8') as f:
    c = f.read()
# Remove stylesheet block
c = re.sub(r'setStyleSheet\(\s*"QFrame#MetricCard \{"\s*"  background:.*?"\s*"  border:.*?"\s*"  border-radius:.*?"\s*"\}"\s*\);', '', c, flags=re.DOTALL)
c = c.replace('font-family: \'Segoe UI\', \'Inter\', sans-serif;', 'font-family: \'Hanken Grotesk\', \'Segoe UI\', \'Inter\', sans-serif;')
c = c.replace('font-size: 10px;', 'font-size: 13px;')
c = c.replace('font-family: \'Consolas\', \'JetBrains Mono\', \'Courier New\', monospace;', 'font-family: \'JetBrains Mono\', \'Consolas\', monospace;')
c = c.replace('font-size: 30px;', 'font-size: 32px;')
c = c.replace('font-size: 11px;', 'font-size: 13px;')
with open(path, 'w', encoding='utf-8') as f:
    f.write(c)

