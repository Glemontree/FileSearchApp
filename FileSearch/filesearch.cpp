#include "filesearch.h"

#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QComboBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QProgressDialog>
#include <QTextStream>
#include <QApplication>

FileSearch::FileSearch(QWidget *parent)
	: QDialog(parent)
{
	browseButton = createButton(tr("&Browse..."), SLOT(browse()));
	findButton = createButton(tr("&Find"), SLOT(find()));
	fileComboBox = createComboBox(tr("*"));
	textComboBox = createComboBox();
	directoryComboBox = createComboBox(QDir::currentPath());

	fileLabel = new QLabel(tr("Named:"));
	textLabel = new QLabel(tr("Contained text:"));
	directoryLabel = new QLabel(tr("In directory:"));
	filesFoundLabel = new QLabel;

	createFilesTable();

	QHBoxLayout* buttonsLayout = new QHBoxLayout;
	buttonsLayout->addStretch();
	buttonsLayout->addWidget(findButton);

	QGridLayout* mainLayout = new QGridLayout;
	mainLayout->addWidget(fileLabel, 0, 0);
	mainLayout->addWidget(fileComboBox, 0, 1, 1, 2);
	mainLayout->addWidget(textLabel, 1, 0);
	mainLayout->addWidget(textComboBox, 1, 1, 1, 2);
	mainLayout->addWidget(directoryLabel, 2, 0);
	mainLayout->addWidget(directoryComboBox, 2, 1);
	mainLayout->addWidget(browseButton, 2, 2);
	mainLayout->addWidget(filesTable, 3, 0, 1, 3);
	mainLayout->addWidget(filesFoundLabel, 4, 0, 1, 3);
	mainLayout->addLayout(buttonsLayout, 5, 0, 1, 3);
	setLayout(mainLayout);

	setWindowTitle(tr("Find Files"));
	resize(700, 300);
}

FileSearch::~FileSearch()
{
	// 理论上应该在这里释放资源，但是Qt会帮助我们释放
}

QPushButton* FileSearch::createButton(const QString& text, const char* member) {
	QPushButton* button = new QPushButton(text);
	connect(button, SIGNAL(clicked()), this, member);
	return button;
}

QComboBox* FileSearch::createComboBox(const QString& text /* = QString() */) {
	QComboBox* comboBox = new QComboBox;
	comboBox->setEditable(true);
	comboBox->addItem(text);
	comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	return comboBox;
}

void FileSearch::createFilesTable() {
	filesTable = new QTableWidget(0, 2);
	filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

	QStringList labels;
	labels << tr("File Name") << tr("Size");
	filesTable->setHorizontalHeaderLabels(labels);
	filesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	filesTable->verticalHeader()->hide();
	filesTable->setShowGrid(false);
	connect(filesTable, SIGNAL(cellActivated(int, int)), this, SLOT(openFileOfItem(int, int)));
}

void FileSearch::openFileOfItem(int row, int column) {
	QTableWidgetItem* item = filesTable->item(row, 0);
	QDesktopServices::openUrl(QUrl::fromLocalFile(currentDir.absoluteFilePath(item->text())));
}

void FileSearch::browse() {
	QString directory = QFileDialog::getExistingDirectory(this, tr("Find Files"), QDir::currentPath());
	if (!directory.isEmpty()) {
		if (directoryComboBox->findText(directory) == -1) {
			directoryComboBox->addItem(directory);
		}
		directoryComboBox->setCurrentIndex(directoryComboBox->findText(directory));
	}
}

void FileSearch::find() {
	filesTable->setRowCount(0);
	QString fileName = fileComboBox->currentText();
	QString text = textComboBox->currentText();
	QString path = directoryComboBox->currentText();

	currentDir = QDir(path);
	QStringList files;
	if (fileName.isEmpty()) {
		fileName = "*";
	}
	files = currentDir.entryList(QStringList(fileName), QDir::Files | QDir::NoSymLinks);
	if (!text.isEmpty()) {
		files = findFiles(files, text);
	}
	showFiles(files);
}

QStringList FileSearch::findFiles(const QStringList& files, const QString& text) {
	QProgressDialog progressDialog(this);
	progressDialog.setCancelButtonText(tr("&Cancel"));
	progressDialog.setRange(0, files.size());
	progressDialog.setWindowTitle(tr("Find Files"));

	QStringList foundFiles;

	for (int i = 0; i < files.size(); ++i) {
		progressDialog.setValue(i);
		progressDialog.setLabelText(tr("Searching file number %1 of %2...").arg(i).arg(files.size()));
		qApp->processEvents();
		if (progressDialog.wasCanceled()) {
			break;
		}
		QFile file(currentDir.absoluteFilePath(files[i]));
		if (file.open(QIODevice::ReadOnly)) {
			QString line;
			QTextStream in(&file);
			while (!in.atEnd()) {
				if (progressDialog.wasCanceled()) {
					break;
				}
				line = in.readLine();
				if (line.contains(text)) {
					foundFiles << files[i];
					break;
				}
			}
		}
	}
	return foundFiles;
}

void FileSearch::showFiles(const QStringList& files) {
	for (int i = 0; i < files.size(); i++) {
		QFile file(currentDir.absoluteFilePath(files[i]));
		qint64 size = QFileInfo(file).size();

		QTableWidgetItem* fileNameItem = new QTableWidgetItem(files[i]);
		fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
		fileNameItem->setTextAlignment(Qt::AlignCenter);
		QTableWidgetItem* sizeItem = new QTableWidgetItem(tr("%1 KB").arg(int((size + 1023) / 1024)));
		sizeItem->setTextAlignment(Qt::AlignCenter);
		sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);

		int row = filesTable->rowCount();
		filesTable->insertRow(row);
		filesTable->setItem(row, 0, fileNameItem);
		filesTable->setItem(row, 1, sizeItem);
	}
	filesFoundLabel->setText(tr("%1 file(s) found").arg(files.size()) +
		(" (Double click on a file to open it)"));
}
