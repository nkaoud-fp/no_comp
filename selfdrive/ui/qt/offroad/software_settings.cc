#include "selfdrive/ui/qt/offroad/settings.h"

#include <cassert>
#include <cmath>
#include <string>

#include <QDebug>
#include <QLabel>

#include "common/params.h"
#include "common/util.h"
#include "selfdrive/ui/ui.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/widgets/controls.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "system/hardware/hw.h"


void SoftwarePanel::checkForUpdates() {
  std::system("pkill -SIGUSR1 -f system.updated.updated");
}

SoftwarePanel::SoftwarePanel(QWidget* parent) : ListWidget(parent) {
  onroadLbl = new QLabel(tr("Updates are only downloaded while the car is off or in park."));
  onroadLbl->setStyleSheet("font-size: 50px; font-weight: 400; text-align: left; padding-top: 30px; padding-bottom: 30px;");
  addItem(onroadLbl);

  // current version
  versionLbl = new LabelControl(tr("Current Version"), "");
  addItem(versionLbl);

  // automatic updates toggle
  ParamControl *automaticUpdatesToggle = new ParamControl("AutomaticUpdates", tr("Automatically Update FrogPilot"),
                                                       tr("FrogPilot will automatically update itself and it's assets when you're offroad and have an active internet connection."), "");
  connect(automaticUpdatesToggle, &ToggleControl::toggleFlipped, this, &updateFrogPilotToggles);
  addItem(automaticUpdatesToggle);

  // download update btn
  downloadBtn = new ButtonControl(tr("Download"), tr("CHECK"));
  connect(downloadBtn, &ButtonControl::clicked, [=]() {
    downloadBtn->setEnabled(false);
    if (downloadBtn->text() == tr("CHECK")) {
      checkForUpdates();
    } else {
      std::system("pkill -SIGHUP -f system.updated.updated");
    }
    frogpilotUIState()->params_memory.putBool("ManualUpdateInitiated", true);
  });
  addItem(downloadBtn);

  // install update btn
  installBtn = new ButtonControl(tr("Install Update"), tr("INSTALL"));
  connect(installBtn, &ButtonControl::clicked, [=]() {
    installBtn->setEnabled(false);
    params.putBool("DoReboot", true);
  });
  addItem(installBtn);

  // branch selecting
  targetBranchBtn = new ButtonControl(tr("Target Branch"), tr("SELECT"));
  connect(targetBranchBtn, &ButtonControl::clicked, [=]() {
    auto current = params.get("GitBranch");
    QStringList branches = QString::fromStdString(params.get("UpdaterAvailableBranches")).split(",");
    if (!frogpilotUIState()->frogpilot_toggles.value("frogs_go_moo").toBool()) {
      branches.removeAll("FrogPilot-Development");
      branches.removeAll("FrogPilot-Vetting");
      branches.removeAll("FrogPilot-Test");
      branches.removeAll("MAKE-PRS-HERE");
    }
    for (QString b : {current.c_str(), "devel-staging", "devel", "nightly", "master-ci", "master"}) {
      auto i = branches.indexOf(b);
      if (i >= 0) {
        branches.removeAt(i);
        branches.insert(0, b);
      }
    }

    QString cur = QString::fromStdString(params.get("UpdaterTargetBranch"));
    QString selection = MultiOptionDialog::getSelection(tr("Select a branch"), branches, cur, this);
    if (!selection.isEmpty()) {
      params.put("UpdaterTargetBranch", selection.toStdString());
      targetBranchBtn->setValue(QString::fromStdString(params.get("UpdaterTargetBranch")));
      checkForUpdates();

      if (selection.toStdString() != current) {
        if (FrogPilotConfirmationDialog::yesorno(tr("This branch must be downloaded before switching. Would you like to download it now?"), this)) {
          std::system("pkill -SIGHUP -f system.updated.updated");

          frogpilotUIState()->params_memory.putBool("ManualUpdateInitiated", true);
        }
      }
    }
  });
  addItem(targetBranchBtn);

  // uninstall button
  auto uninstallBtn = new ButtonControl(tr("Uninstall %1").arg(getBrand()), tr("UNINSTALL"));
  connect(uninstallBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to uninstall?"), tr("Uninstall"), this)) {
      if (FrogPilotConfirmationDialog::yesorno(tr("Do you want to delete deep storage FrogPilot assets? This includes your toggle settings for quick reinstalls."), this)) {
        if (FrogPilotConfirmationDialog::yesorno(tr("Are you sure? This is 100% unrecoverable and if you reinstall FrogPilot you'll lose all your previous settings!"), this)) {
          std::system("rm -rf /cache/params/d");
        }
      }
      params.putBool("DoUninstall", true);
    }
  });
  addItem(uninstallBtn);

  // error log button
  auto errorLogBtn = new ButtonControl(tr("Error Log"), tr("VIEW"), tr("View the error log for openpilot crashes."));
  connect(errorLogBtn, &ButtonControl::clicked, [=]() {
    std::string txt = util::read_file("/data/error_logs/error.txt");
    ConfirmationDialog::rich(QString::fromStdString(txt), this);
  });
  addItem(errorLogBtn);

  fs_watch = new ParamWatcher(this);
  QObject::connect(fs_watch, &ParamWatcher::paramChanged, [=](const QString &param_name, const QString &param_value) {
    updateLabels();
  });

  connect(uiState(), &UIState::offroadTransition, [=](bool offroad) {
    is_onroad = !offroad;
    updateLabels();
  });

  updateLabels();
}

void SoftwarePanel::showEvent(QShowEvent *event) {
  // nice for testing on PC
  installBtn->setEnabled(true);

  updateLabels();
}

void SoftwarePanel::updateLabels() {
  FrogPilotUIState &fs = *frogpilotUIState();
  FrogPilotUIScene &frogpilot_scene = fs.frogpilot_scene;

  // add these back in case the files got removed
  fs_watch->addParam("LastUpdateTime");
  fs_watch->addParam("UpdateFailedCount");
  fs_watch->addParam("UpdaterState");
  fs_watch->addParam("UpdateAvailable");

  if (!isVisible()) {
    frogpilot_scene.downloading_update = false;
    return;
  }

  // updater only runs offroad or when parked
  bool parked = frogpilot_scene.parked || frogpilot_scene.frogpilot_toggles.value("frogs_go_moo").toBool();

  onroadLbl->setVisible(is_onroad && !parked);
  downloadBtn->setVisible(!is_onroad || parked);

  // download update
  QString updater_state = QString::fromStdString(params.get("UpdaterState"));
  bool failed = std::atoi(params.get("UpdateFailedCount").c_str()) > 0;
  if (updater_state != "idle") {
    downloadBtn->setEnabled(false);
    downloadBtn->setValue(updater_state);
    frogpilot_scene.downloading_update = true;
  } else {
    frogpilot_scene.downloading_update = false;
    if (failed) {
      downloadBtn->setText(tr("CHECK"));
      downloadBtn->setValue(tr("failed to check for update"));
    } else if (params.getBool("UpdaterFetchAvailable")) {
      downloadBtn->setText(tr("DOWNLOAD"));
      downloadBtn->setValue(tr("update available"));
    } else {
      QString lastUpdate = tr("never");
      auto tm = params.get("LastUpdateTime");
      if (!tm.empty()) {
        lastUpdate = timeAgo(QDateTime::fromString(QString::fromStdString(tm + "Z"), Qt::ISODate));
      }
      downloadBtn->setText(tr("CHECK"));
      downloadBtn->setValue(tr("up to date, last checked %1").arg(lastUpdate));
    }
    downloadBtn->setEnabled(true);
  }
  targetBranchBtn->setValue(QString::fromStdString(params.get("UpdaterTargetBranch")));

  // current + new versions
  versionLbl->setText(QString::fromStdString(params.get("UpdaterCurrentDescription")));
  versionLbl->setDescription(QString::fromStdString(params.get("UpdaterCurrentReleaseNotes")));

  installBtn->setVisible((!is_onroad || parked) && params.getBool("UpdateAvailable"));
  installBtn->setValue(QString::fromStdString(params.get("UpdaterNewDescription")));
  installBtn->setDescription(QString::fromStdString(params.get("UpdaterNewReleaseNotes")));

  update();
}
