/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "RobotModel.h"
using namespace std;

void RobotModel::SetAutoState(uint32_t state) {
    state_ = state;
}

uint32_t RobotModel::GetAutoState() {
    return state_;
}

void RobotModel::SetFlywheelOutput(double power){
    flywheelMotor1_->Set(power);
    //flywheelMotor2_->Set(-power);
}

double RobotModel::GetFlywheel1EncoderValue(){
    return flywheelEncoder1_->GetIntegratedSensorPosition();
}

double RobotModel::GetFlywheel2EncoderValue() {
    return -flywheelEncoder2_->GetIntegratedSensorPosition(); 
}

void RobotModel::EngageFlywheelHood() {
    flywheelHoodSolenoid_->Set(true);
}

void RobotModel::DisengageFlywheelHood() {
    flywheelHoodSolenoid_->Set(false);
}

void RobotModel::SetClimbWinchLeftOutput(double power){
    climberWinchLeftMotor_->Set(power);
}

void RobotModel::SetClimbWinchRightOutput(double power){
    climberWinchRightMotor_->Set(power);
}

void RobotModel::SetClimberElevatorOutput(double power){
    climberElevatorMotor_->Set(power);
}

void RobotModel::SetControlPanelOutput(double power){
    controlPanelMotor_->Set(power);
}

void RobotModel::SetIntakeRollersOutput(double power) {
    intakeRollersMotor_->Set(power);
}

void RobotModel::SetIntakeWristOutput(double power) {
    intakeWristMotor_->Set(power);
}

void RobotModel::SetIndexFunnelOutput(double power) {
    indexFunnelMotor_->Set(power);
}

void RobotModel::SetElevatorFeederOutput(double power) {
    elevatorFeederMotor_->Set(power);
    //elevatorMotor_->Set(power);
}

void RobotModel::SetElevatorOutput(double power) {
    elevatorMotor_->Set(power);
    //elevatorFeederMotor_->Set(power);
}

void RobotModel::SetLight(bool setLight){
	lightSolenoid_ -> Set(setLight);
}


WPI_TalonFX* RobotModel::GetFlywheelMotor1() {
    return flywheelMotor1_;
}

WPI_TalonFX* RobotModel::GetFlywheelMotor2() {
    return flywheelMotor2_;
}

double RobotModel::GetTargetDistance() {
    // vision code to get distance, idk how that works but it needs to work
    return 0.0;
}


void RobotModel::GetColorFromSensor() {
    detectedColor_ = colorSensor_->GetColor();
    // cout<<"red "<<detectedColor_.red<<endl;
    // cout<<"green "<<detectedColor_.green<<endl;
    // cout<<"blue "<<detectedColor_.blue<<endl;
}

std::string RobotModel::MatchColor() {
    colorConfidence_ = 0.9;
    matchedColor_ = colorMatcher_.MatchClosestColor(detectedColor_, colorConfidence_);
    
    if (matchedColor_ == kBlueTarget) {
      colorString_ = "Blue";
    } else if (matchedColor_ == kRedTarget) {
      colorString_ = "Red";
    } else if (matchedColor_ == kGreenTarget) {
      colorString_ = "Green";
    } else if (matchedColor_ == kYellowTarget) {
      colorString_ = "Yellow";
    } else {
      colorString_ = "Unknown"; // add some command to move forward if this happens
    }

    //cout<<colorString_<<endl;
    return colorString_;
}

std::string RobotModel::GetControlPanelGameData() {
    return controlPanelGameData_;
}


//should return degrees
double RobotModel::GetIntakeWristAngle(){
    return (360.0/4096)*intakeWristMotor_->GetSelectedSensorPosition();
}

bool RobotModel::GetElevatorFeederLightSensorStatus() {
    return  elevatorFeederLightSensor_->Get();
}
bool RobotModel::GetElevatorLightSensorStatus() {
    return elevatorLightSensor_->Get();
}

