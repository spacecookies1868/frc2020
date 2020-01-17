/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "MainProgram.h"

#include <iostream>

#include <frc/smartdashboard/SmartDashboard.h>

void MainProgram::RobotInit() {
    robot_ = new RobotModel();
    humanControl_ = new ControlBoard();
    superstructureController_ = new SuperstructureController(robot_, humanControl_);
    driveController_ = new DriveController(robot_, humanControl_);
    robot_->ResetDriveEncoders();
}

/**
 * This function is called every robot packet, no matter the mode. Use
 * this for items like diagnostics that you want ran during disabled,
 * autonomous, teleoperated and test.
 *
 * <p> This runs after the mode specific periodic functions, but before
 * LiveWindow and SmartDashboard integrated updating.
 */
void MainProgram::RobotPeriodic() {
    driveController_->RefreshShuffleboard();
    superstructureController_->RefreshShuffleboard();
    robot_->RefreshShuffleboard();
}

/**
 * This autonomous (along with the chooser code above) shows how to select
 * between different autonomous modes using the dashboard. The sendable chooser
 * code works with the Java SmartDashboard. If you prefer the LabVIEW Dashboard,
 * remove all of the chooser code and uncomment the GetString line to get the
 * auto name from the text box below the Gyro.
 *
 * You can add additional auto modes by adding additional comparisons to the
 * if-else structure below with additional strings. If using the SendableChooser
 * make sure to add them to the chooser code above as well.
 */
void MainProgram::AutonomousInit() {
    robot_->ResetDriveEncoders();
    robot_->ZeroNavXYaw();
    tempNavXSource_ = new NavXPIDSource(robot_);
    tempPivot_ = new PivotCommand(robot_, 90.0, true, tempNavXSource_);
    tempPivot_->Init();
}

void MainProgram::AutonomousPeriodic() {
    if(!tempPivot_->IsDone()){
        tempPivot_->Update(0.0, 0.0);
    }
}

void MainProgram::TeleopInit() {
    robot_->ResetDriveEncoders();
}

void MainProgram::TeleopPeriodic() {
    humanControl_->ReadControls();
    driveController_->Update();
    superstructureController_->Update();
}

void MainProgram::TestPeriodic() {}

#ifndef RUNNING_FRC_TESTS
int main() { return frc::StartRobot<MainProgram>(); }
#endif
