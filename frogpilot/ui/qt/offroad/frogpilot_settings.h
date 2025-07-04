#pragma once

#include "selfdrive/ui/qt/offroad/settings.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"

class FrogPilotSettingsWindow : public QFrame {
  Q_OBJECT

public:
  explicit FrogPilotSettingsWindow(SettingsWindow *parent);

  void updateVariables();

  bool hasAutoTune = true;
  bool hasBSM = true;
  bool hasDashSpeedLimits = true;
  bool hasExperimentalOpenpilotLongitudinal = false;
  bool hasNNFFLog = true;
  bool hasOpenpilotLongitudinal = true;
  bool hasPCMCruise = false;
  bool hasRadar = true;
  bool hasSNG = false;
  bool isBolt = false;
  bool isC3 = false;
  bool isGM = true;
  bool isHKG = true;
  bool isHKGCanFd = true;
  bool isHonda = false;
  bool isSubaru = false;
  bool isTorqueCar = false;
  bool isToyota = true;
  bool isTSK = false;
  bool isVolt = true;
  bool keepScreenOn = false;
  bool openpilotLongitudinalControlDisabled = false;
  bool tacoHacksAllowed = false;

  float delayStock;
  float frictionStock;
  float kpStock;
  float latAccelStock;
  float steerRatioStock;

  int tuningLevel;

  QJsonObject frogpilotToggleLevels;

signals:
  void closeSubPanel();
  void closeSubSubPanel();
  void openPanel();
  void openSubPanel();
  void openSubSubPanel();
  void updateMetric(bool metric, bool bootRun=false);

private:
  void closePanel();
  void createPanelButtons(FrogPilotListWidget *list);
  void hideEvent(QHideEvent *event) override;
  void updateState();

  bool panelOpen;

  FrogPilotButtonsControl *drivingPanelButtons;
  FrogPilotButtonsControl *systemPanelButtons;
  FrogPilotButtonsControl *vehiclePanelButtons;

  Params params;
  Params params_memory{"/dev/shm/params"};
  Params params_tracking{"/cache/tracking"};

  QStackedLayout *mainLayout;

  ScrollView *frogpilotPanel;
};
