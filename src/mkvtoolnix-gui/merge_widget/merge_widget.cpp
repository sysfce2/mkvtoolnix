#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge_widget/merge_widget.h"
#include "mkvtoolnix-gui/forms/main_window.h"
#include "mkvtoolnix-gui/forms/merge_widget.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QString>

MergeWidget::MergeWidget(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::MergeWidget}
  , m_filesModel{new SourceFileModel{this}}
  , m_tracksModel{new TrackModel{this}}
  , m_attachmentsModel{new AttachmentModel{this, m_config.m_attachments}}
  , m_currentlySettingInputControlValues{false}
  , m_addAttachmentsAction{nullptr}
  , m_removeAttachmentsAction{nullptr}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupMenu();
  setupInputControls();
  setupAttachmentsControls();
}

MergeWidget::~MergeWidget() {
  delete ui;
}

void
MergeWidget::onSaveConfig() {
  if (m_config.m_configFileName.isEmpty())
    onSaveConfigAs();
  else {
    m_config.save();
    MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
  }
}

void
MergeWidget::onSaveConfigAs() {
  auto fileName = QFileDialog::getSaveFileName(this, Q(""), Settings::get().m_lastConfigDir.path(), QY("MKVToolNix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  m_config.save(fileName);
  Settings::get().m_lastConfigDir = QFileInfo{fileName}.path();
  Settings::get().save();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
}

void
MergeWidget::onOpenConfig() {
  auto fileName = QFileDialog::getOpenFileName(this, Q(""), Settings::get().m_lastConfigDir.path(), QY("MKVToolNix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  Settings::get().m_lastConfigDir = QFileInfo{fileName}.path();
  Settings::get().save();

  try {
    m_config.load(fileName);
    MainWindow::get()->setStatusBarMessage(QY("The configuration has been loaded."));

  } catch (mtx::InvalidSettingsX &) {
    m_config.reset();
    QMessageBox::critical(this, QY("Error loading settings file"), QY("The settings file '%1' contains invalid settings and was not loaded.").arg(fileName));
  }
}

void
MergeWidget::onNew() {
  // TODO
}

void
MergeWidget::onAddToJobQueue() {
  // TODO
}

void
MergeWidget::onStartMuxing() {
  // TODO
}

QString
MergeWidget::getOpenFileName(QString const &title,
                            QString const &filter,
                            QLineEdit *lineEdit) {
  auto fullFilter = filter;
  if (!fullFilter.isEmpty())
    fullFilter += Q(";;");
  fullFilter += QY("All files") + Q(" (*)");

  auto dir      = lineEdit->text().isEmpty() ? Settings::get().m_lastOpenDir.path() : QFileInfo{ lineEdit->text() }.path();
  auto fileName = QFileDialog::getOpenFileName(this, title, dir, fullFilter);
  if (fileName.isEmpty())
    return fileName;

  Settings::get().m_lastOpenDir = QFileInfo{fileName}.path();
  Settings::get().save();

  lineEdit->setText(fileName);

  return fileName;
}

QString
MergeWidget::getSaveFileName(QString const &title,
                            QString const &filter,
                            QLineEdit *lineEdit) {
  auto fullFilter = filter;
  if (!fullFilter.isEmpty())
    fullFilter += Q(";;");
  fullFilter += QY("All files") + Q(" (*)");

  auto dir      = lineEdit->text().isEmpty() ? Settings::get().m_lastOutputDir.path() : QFileInfo{ lineEdit->text() }.path();
  auto fileName = QFileDialog::getSaveFileName(this, title, dir, fullFilter);
  if (fileName.isEmpty())
    return fileName;

  Settings::get().m_lastOutputDir = QFileInfo{fileName}.path();
  Settings::get().save();

  lineEdit->setText(fileName);

  return fileName;
}

void
MergeWidget::resizeViewColumnsToContents(QTreeView *view)
  const {
  auto columnCount = view->model()->columnCount(QModelIndex{});
  for (auto column = 0; columnCount > column; ++column)
    view->resizeColumnToContents(column);
}

void
MergeWidget::setupMenu() {
  auto mwUi = MainWindow::get()->getUi();

  connect(mwUi->actionNew,                            SIGNAL(triggered()), this,              SLOT(onNew()));
  connect(mwUi->actionOpen,                           SIGNAL(triggered()), this,              SLOT(onOpenConfig()));
  connect(mwUi->actionSave,                           SIGNAL(triggered()), this,              SLOT(onSaveConfig()));
  connect(mwUi->actionSave_as,                        SIGNAL(triggered()), this,              SLOT(onSaveConfigAs()));
  // connect(mwUi->actionSave_option_file,               SIGNAL(triggered()), this,              SLOT(onSaveOpenFile()));
  connect(mwUi->actionExit,                           SIGNAL(triggered()), MainWindow::get(), SLOT(close()));

  connect(mwUi->action_Start_muxing,                  SIGNAL(triggered()), this,              SLOT(onStartMuxing()));
  connect(mwUi->actionAdd_to_job_queue,               SIGNAL(triggered()), this,              SLOT(onAddToJobQueue()));
  // connect(mwUi->action_Job_manager,                   SIGNAL(triggered()), this,              SLOT(onJobManager()));
  // connect(mwUi->actionShow_mkvmerge_command_line,     SIGNAL(triggered()), this,              SLOT(onShowCommandLine()));
  // connect(mwUi->actionCopy_command_line_to_clipboard, SIGNAL(triggered()), this,              SLOT(onCopyCommandLineToClipboard()));
}
