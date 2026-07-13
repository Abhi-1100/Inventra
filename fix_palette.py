import os, re

src = 'd:/confres/src'
for root, dirs, files in os.walk(src):
    for file in files:
        if file.endswith('.cpp'):
            path = os.path.join(root, file)
            with open(path, 'r', encoding='utf-8') as f:
                content = f.read()
            if 'Palette::' in content:
                content = re.sub(r'QColor\(\s*Palette::(\w+)\s*\)', r'ThemeManager::instance().tokens().\1', content)
                content = re.sub(r'Palette::(\w+)', r'ThemeManager::instance().tokens().\1', content)
                if 'ThemeManager.h' not in content:
                    content = '#include "core/ThemeManager.h"\n' + content
                with open(path, 'w', encoding='utf-8') as f:
                    f.write(content)
