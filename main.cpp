#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QStringList>
#include <QListWidget>
#include <QLabel>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>


void showMessage(QWidget *parent, const QString &text) {
    QMessageBox::information(parent, "Информация", text);
}

QStringList splitPatterns(const QString &input)
{
    QString trimmed = input.trimmed();
    if (trimmed.isEmpty())
        return {};

    QStringList raw = trimmed.split(QRegularExpression("[/,]"),
                                    Qt::SkipEmptyParts);

    QStringList patterns;
    for (QString part : raw) {
        part = part.trimmed();
        if (!part.isEmpty())
            patterns << part;
    }
    return patterns;
}


QStringList loadSearchRootsFromJson()
{
    QFile f("fsc_config.json");
    if (!f.open(QIODevice::ReadOnly))
        return {};

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return {};

    QStringList roots;
    for (const QJsonValue &v : doc.array()) {
        if (v.isString())
            roots << v.toString();
    }
    return roots;
}


void saveSearchRootsToJson(const QStringList &roots)
{
    QJsonArray arr;
    for (const QString &r : roots)
        arr.append(r);

    QJsonDocument doc(arr);
    QFile f("fsc_config.json");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    f.write(doc.toJson());
    f.close();
}

void findFiles(const QString &patternText,
               const QStringList &roots,
               QTableWidget *table,
               QWidget *parent,
               QLabel *statusLabel)
{
    table->setRowCount(0);

    QStringList patterns = splitPatterns(patternText);
    if (patterns.isEmpty()) {
        showMessage(parent, "Введите часть имени файла/папки для поиска.");
        if (statusLabel)
            statusLabel->setText("Готово. Пустой запрос.");
        return;
    }

    if (statusLabel) {
        statusLabel->setText("Поиск...");
        QCoreApplication::processEvents();
    }

    int processedItems = 0;
    int foundItems     = 0;

    for (const QString &root : roots) {
        QDirIterator it(root,
                        QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString path = it.next();
            const QFileInfo info(path);
            const QString name = info.fileName();

            ++processedItems;


            if (processedItems % 500 == 0) {
                if (statusLabel) {
                    statusLabel->setText(
                        QString("  Скан: %1, найдено: %2")
                            .arg(processedItems)
                            .arg(foundItems)
                        );
                }
                QCoreApplication::processEvents();
            }

            bool match = false;
            for (const QString &p : patterns) {
                if (name.contains(p, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            if (!match)
                continue;

            const int row = table->rowCount();
            table->insertRow(row);

            QString displayName = name;
            if (info.isDir())
                displayName += " [DIR]";

            auto *nameItem = new QTableWidgetItem(displayName);
            nameItem->setForeground(QBrush(Qt::green));

            auto *pathItem = new QTableWidgetItem(path);

            table->setItem(row, 0, nameItem);
            table->setItem(row, 1, pathItem);

            ++foundItems;
        }
    }

    if (foundItems == 0) {
        showMessage(parent, "Файлы/папки не найдены.");
        if (statusLabel)
            statusLabel->setText("Готово. Ничего не найдено.");
    } else {
        if (statusLabel) {
            statusLabel->setText(
                QString("  Готово. Найдено: %1, просмотрено: %2")
                    .arg(foundItems)
                    .arg(processedItems)
                );
        }
    }
}


QStringList selectedFilePaths(QTableWidget *table)
{
    QStringList paths;
    const auto ranges = table->selectedRanges();

    for (const QTableWidgetSelectionRange &r : ranges) {
        for (int row = r.topRow(); row <= r.bottomRow(); ++row) {
            if (auto *pathItem = table->item(row, 1))
                paths << pathItem->text();
        }
    }

    return paths;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("FSC");
    window.resize(900, 620);

    auto *mainLayout = new QHBoxLayout(&window);


    auto *leftPanel  = new QWidget(&window);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    leftLayout->setSpacing(12);

    auto *searchEdit     = new QLineEdit(leftPanel);
    auto *searchButton   = new QPushButton("Search", leftPanel);
    auto *deleteButton   = new QPushButton("Удалить", leftPanel);
    auto *openButton     = new QPushButton("Перейти к файлу/папке", leftPanel);
    auto *moveButton     = new QPushButton("Переместить", leftPanel);

    auto *collapseButton = new QPushButton("≪", leftPanel);
    collapseButton->setFixedWidth(32);
    collapseButton->setToolTip("Скрыть меню");

    auto *settingsButton = new QPushButton("Settings", leftPanel);
    auto *statusLabel    = new QLabel("        Ожидание скана!", leftPanel);

    auto *infoLabelLeft = new QLabel(leftPanel);
    infoLabelLeft->setText(
        "FSC - утилита поиска\n"
        "файлов на ПК. Она\n"
        "реализована на C++ и\n"
        "позволяет быстро\n"
        "искать файлы и папки.\n\n"
        "Developed by DeziX.\n"
        "Version 0.2"
        );
    infoLabelLeft->setWordWrap(true);

    searchEdit->setPlaceholderText("Имя файлов/папки");

    leftLayout->addWidget(searchEdit);
    leftLayout->addWidget(searchButton);
    leftLayout->addSpacing(4);
    leftLayout->addWidget(deleteButton);
    leftLayout->addWidget(openButton);
    leftLayout->addWidget(moveButton);
    leftLayout->addSpacing(8);
    leftLayout->addWidget(infoLabelLeft);
    leftLayout->addStretch();
    leftLayout->addWidget(statusLabel);


    auto *bottomButtonsLayout = new QHBoxLayout();
    bottomButtonsLayout->addWidget(collapseButton);
    bottomButtonsLayout->addWidget(settingsButton);
    leftLayout->addLayout(bottomButtonsLayout);


    auto *rightPanel  = new QWidget(&window);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    rightLayout->setSpacing(8);


    auto *showLeftButton = new QPushButton("Панель", rightPanel);
    showLeftButton->setVisible(false);
    rightLayout->addWidget(showLeftButton, 0, Qt::AlignLeft);


    auto *backButton = new QPushButton("Back", rightPanel);
    backButton->setVisible(false);
    rightLayout->addWidget(backButton, 0, Qt::AlignRight);


    auto *stack = new QStackedWidget(rightPanel);
    rightLayout->addWidget(stack, 1);


    auto *resultsPage   = new QWidget(rightPanel);
    auto *resultsLayout = new QVBoxLayout(resultsPage);
    auto *resultsTable  = new QTableWidget(resultsPage);
    resultsTable->setColumnCount(2);
    resultsTable->setHorizontalHeaderLabels({"Имя", "Путь"});
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    resultsLayout->addWidget(resultsTable);


    auto *settingsPage   = new QWidget(rightPanel);
    auto *settingsLayout = new QVBoxLayout(settingsPage);
    auto *settingsLabel  = new QLabel("Корневые папки для поиска:", settingsPage);
    auto *rootsList      = new QListWidget(settingsPage);
    auto *settingsButtonsLayout = new QHBoxLayout();
    auto *addRootButton    = new QPushButton("Выбор папки", settingsPage);
    auto *removeRootButton = new QPushButton("Удалить выбранную", settingsPage);
    settingsButtonsLayout->addWidget(addRootButton);
    settingsButtonsLayout->addWidget(removeRootButton);

    settingsLayout->addWidget(settingsLabel);
    settingsLayout->addWidget(rootsList);
    settingsLayout->addLayout(settingsButtonsLayout);


    stack->addWidget(resultsPage);
    stack->addWidget(settingsPage);

    // кладём панели в основной лейаут
    mainLayout->addWidget(leftPanel, 0);
    mainLayout->addWidget(rightPanel, 1);
    mainLayout->setStretch(0, 0);
    mainLayout->setStretch(1, 1);


    QStringList searchRoots = loadSearchRootsFromJson();
    if (searchRoots.isEmpty()) {
#ifdef Q_OS_WIN
        searchRoots << "C:/";
#else
        searchRoots << QDir::homePath();
#endif
    }

    QObject::connect(searchButton, &QPushButton::clicked, [&]() {
        const QString pattern = searchEdit->text();
        findFiles(pattern, searchRoots, resultsTable, &window, statusLabel);
    });


    QObject::connect(deleteButton, &QPushButton::clicked, [&]() {
        const QStringList paths = selectedFilePaths(resultsTable);
        if (paths.isEmpty()) {
            showMessage(&window, "Выделите хотя бы один файл или папку для удаления.");
            return;
        }

        if (QMessageBox::question(&window, "Подтверждение",
                                  "Удалить выделенные объекты? Это действие необратимо.")
            != QMessageBox::Yes)
            return;

        for (const QString &path : paths) {
            QFileInfo info(path);
            bool ok = false;

            if (info.isDir()) {
                QDir dir(path);
                ok = dir.removeRecursively();
            } else {
                QFile file(path);
                ok = file.remove();
            }

            if (!ok) {
                QMessageBox::warning(&window, "Ошибка",
                                     "Не удалось удалить:\n" + path);
            }
        }

        findFiles(searchEdit->text(), searchRoots, resultsTable, &window, statusLabel);
    });


    QObject::connect(openButton, &QPushButton::clicked, [&]() {
        const QStringList paths = selectedFilePaths(resultsTable);
        if (paths.isEmpty()) {
            showMessage(&window, "Выделите объект для перехода.");
            return;
        }

        const QString firstPath = paths.first();
        QFileInfo info(firstPath);

        QString openPath;
        if (info.isDir())
            openPath = firstPath;
        else
            openPath = info.absolutePath();

        QDesktopServices::openUrl(QUrl::fromLocalFile(openPath));
    });


    QObject::connect(moveButton, &QPushButton::clicked, [&]() {
        const QStringList paths = selectedFilePaths(resultsTable);
        if (paths.isEmpty()) {
            showMessage(&window, "Выделите файлы/папки для перемещения.");
            return;
        }

        const QString targetDir = QFileDialog::getExistingDirectory(
            &window, "Выберите папку назначения");
        if (targetDir.isEmpty())
            return;

        QDir dir;

        for (const QString &path : paths) {
            const QFileInfo info(path);
            const QString newPath = targetDir + "/" + info.fileName();

            bool ok = false;
            if (info.isDir()) {
                ok = dir.rename(path, newPath);
            } else {
                ok = QFile::rename(path, newPath);
            }

            if (!ok) {
                QMessageBox::warning(&window, "Ошибка",
                                     "Не удалось переместить:\n" + path);
            }
        }

        findFiles(searchEdit->text(), searchRoots, resultsTable, &window, statusLabel);
    });


    QObject::connect(settingsButton, &QPushButton::clicked, [&]() {
        rootsList->clear();
        for (const QString &root : searchRoots)
            rootsList->addItem(root);

        stack->setCurrentWidget(settingsPage);
        backButton->setVisible(true);
    });


    QObject::connect(backButton, &QPushButton::clicked, [&]() {
        QStringList newRoots;
        for (int i = 0; i < rootsList->count(); ++i)
            newRoots << rootsList->item(i)->text();

        if (newRoots.isEmpty()) {
#ifdef Q_OS_WIN
            newRoots << "C:/";
#else
            newRoots << QDir::homePath();
#endif
        }

        searchRoots = newRoots;
        saveSearchRootsToJson(searchRoots);

        stack->setCurrentWidget(resultsPage);
        backButton->setVisible(false);
    });


    QObject::connect(addRootButton, &QPushButton::clicked, [&]() {
        const QString dirPath = QFileDialog::getExistingDirectory(&window, "Выберите папку");
        if (!dirPath.isEmpty())
            rootsList->addItem(dirPath);
    });


    QObject::connect(removeRootButton, &QPushButton::clicked, [&]() {
        if (auto *item = rootsList->currentItem())
            delete item;
    });


    QObject::connect(collapseButton, &QPushButton::clicked, [&]() {
        leftPanel->setVisible(false);
        showLeftButton->setVisible(true);
    });


    QObject::connect(showLeftButton, &QPushButton::clicked, [&]() {
        leftPanel->setVisible(true);
        showLeftButton->setVisible(false);
    });

    window.show();
    return app.exec();
}
