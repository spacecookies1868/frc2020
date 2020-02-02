/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "RobotModel.h"
using namespace std;


void RobotModel::SetFlywheelOutput(double power){
    flywheelMotor1_ -> Set(power);
    //flywheelMotor2_ -> Set(-power);
}
void RobotModel::SetClimberOutput(double power){
    climberMotor1_ -> Set(power);
    if (climberEncoder1_->  GetPosition() >= SPARK_ENCODER_TICKS) { // need to test this
        climberMotor2_ -> Set(-power);
    }
}
void RobotModel::SetControlPanelOutput(double power){
    controlPanelMotor_ -> Set(power);
}
void RobotModel::SetIntakeRollersOutput(double power) {
    intakeRollersMotor_->Set(power);
}
void RobotModel::SetIntakeWristOutput(double power) {
    intakeWristMotor_->Set(power);
}
void RobotModel::SetFunnelIndexOutput(double power) {
    funnelIndexMotor_->Set(power);
}
void RobotModel::SetBottomElevatorOutput(double power) {
    elevatorIndexMotor1_->Set(power);
}
void RobotModel::SetTopElevatorOutput(double power) {
    elevatorIndexMotor2_->Set(power);
}


void RobotModel::SetLight(bool setLight){
	lightSolenoid_ -> Set(setLight);
}


rev::CANSparkMax* RobotModel::GetFlywheelMotor1() {
    return flywheelMotor1_;
}

rev::CANSparkMax* RobotModel::GetFlywheelMotor2() {
    return flywheelMotor2_;
}


void RobotModel::GetColorFromSensor() {
    detectedColor_ = colorSensor_->GetColor();
    cout<<"red "<<detectedColor_.red<<endl;
    cout<<"green "<<detectedColor_.green<<endl;
    cout<<"blue "<<detectedColor_.blue<<endl;
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

    cout<<colorString_<<endl;
    return colorString_;
}

std::string RobotModel::GetControlPanelGameData() {
    return controlPanelGameData_;
}

AnalogGyro* RobotModel::GetGyro(){
	return gyro_;
}

double RobotModel::GetGyroAngle(){
    return gyro_->GetAngle();
}


