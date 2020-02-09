/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#pragma once
#include "RobotModel.h"
#include "ControlBoard.h"

class SuperstructureController {
 public:
  SuperstructureController(RobotModel *robot, ControlBoard *humanControl);
  void Update();
  void DisabledUpdate();
  void RefreshShuffleboard();
  void FlywheelPIDControllerUpdate();
  double CalculateFlywheelPowerDesired();
  
  void FlywheelHoodUp();
  void FlywheelHoodDown();

  bool IndexUpdate(bool readyToShoot);

  void CalculateIntakeRollersPower();

  void ControlPanelStage2(double power);
  void ControlPanelStage3(double power);
  void ControlPanelFinalSpin();
  void Reset();

  enum SuperstructureState {
		kInit, kIdle, kShooting, kIndexing, kIntaking, kControlPanelStage2, kControlPanelStage3
	};

  enum IndexState {
    kIndexInit, kLower, kLift
  };

  ~SuperstructureController();

 private:
  RobotModel *robot_;
  ControlBoard *humanControl_;
  TalonFXSensorCollection *flywheelPIDController_;
  TalonFXSensorCollection *flywheelEncoder1_, *flywheelEncoder2_; // unused rn
  
  uint32_t currState_;
	uint32_t nextState_;
  IndexState currIndexState_, nextIndexState_;

  double flywheelPower_, desiredRPM_, flywheelResetTime_;
  double flywheelPFac_, flywheelIFac_, flywheelDFac_, flywheelFFFac_;
  PIDController *flywheelPID_;
  FlywheelPIDOutput* flywheelPIDOutput_;

  double lowerElevatorTimeout_;
  double elevatorTimeout_;
  double elevatorSlowPower_;
  double elevatorFastPower_;
  double elevatorFeederPower_;
  double indexFunnelPower_;

  double climberPower_;

  double desiredIntakeWristAngle_;
  double currGyroAngle_, lastGyroAngle_;
  double currTime_, lastTime_;

  double startIndexTime_;
  double startElevatorTime_;
  bool lastLower_;
  bool lastUpper_;

  int controlPanelCounter_;
  double initialControlPanelTime_;
  std::string initialControlPanelColor_, previousControlPanelColor_, colorDesired_;

  ShuffleboardLayout &superstructureLayout_;
  nt::NetworkTableEntry flywheelVelocityEntry_, flywheelPEntry_, flywheelIEntry_, flywheelDEntry_, flywheelFFEntry_;
  nt::NetworkTableEntry elevatorFeederLightSensorEntry_, elevatorLightSensorEntry_;
};
