/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "auto/commands/DriveStraightCommand.h"
#include <frc/WPILib.h>

// constructing
DriveStraightCommand::DriveStraightCommand(NavXPIDSource* navXSource, TalonEncoderPIDSource* talonEncoderSource,
		AnglePIDOutput* anglePIDOutput, DistancePIDOutput* distancePIDOutput, RobotModel* robot,
		double desiredDistance) : AutoCommand(),
		driveStraightLayout_(robot->GetFunctionalityTab().GetLayout("DriveStraight", "List Layout")),
		driveStraightPIDLayout_(robot->GetModeTab().GetLayout("DriveStraight PID", "List Layout")),
		anglePIDLayout_(driveStraightLayout_.GetLayout("Angle", "List Layout")),
		distancePIDLayout_(driveStraightLayout_.GetLayout("Distance", "List Layout"))
		{
	isAbsoluteAngle_ = false;

	// initialize dependencies
	Initializations(navXSource, talonEncoderSource, anglePIDOutput, distancePIDOutput, robot, desiredDistance);
	
	leftStraightEntry_ = driveStraightLayout_.Add("Left Output", 0.0).GetEntry();
	rightStraightEntry_ = driveStraightLayout_.Add("Right Output", 0.0).GetEntry();
	desiredAngleEntry_ = driveStraightLayout_.Add("Desired Angle", 0.0).GetEntry();
	desiredTotalFeetEntry_ = driveStraightLayout_.Add("Desired Total Feet", 0.0).GetEntry();
	angleErrorEntry_ = driveStraightLayout_.Add("Angle Error", 0.0).WithWidget(BuiltInWidgets::kGraph).GetEntry();
	encoderErrorEntry_ = driveStraightLayout_.Add("Encoder Error", 0.0).WithWidget(BuiltInWidgets::kGraph).GetEntry();
	aPIDOutputEntry_ = driveStraightLayout_.Add("Angle PID Output", 0.0).GetEntry();
	dPIDOutputEntry_ = driveStraightLayout_.Add("Distance PID Output", 0.0).GetEntry();
    aPEntry_ = anglePIDLayout_.Add("P", 0.08).GetEntry();
    aIEntry_ = anglePIDLayout_.Add("I", 0.0).GetEntry();
    aDEntry_ = anglePIDLayout_.Add("D", 0.02).GetEntry();
    dPEntry_ = distancePIDLayout_.Add("P", 0.8).GetEntry();
    dIEntry_ = distancePIDLayout_.Add("I", 0.0).GetEntry();
    dDEntry_ = distancePIDLayout_.Add("D", 0.2).GetEntry();
	
}

// constructor
DriveStraightCommand::DriveStraightCommand(NavXPIDSource* navXSource, TalonEncoderPIDSource* talonEncoderSource,
		AnglePIDOutput* anglePIDOutput, DistancePIDOutput* distancePIDOutput, RobotModel* robot,
		double desiredDistance, double absoluteAngle) :
		driveStraightLayout_(robot->GetFunctionalityTab().GetLayout("DriveStraight", "List Layout")),
		driveStraightPIDLayout_(robot->GetModeTab().GetLayout("DriveStraight PID", "List Layout")),
		anglePIDLayout_(driveStraightLayout_.GetLayout("Angle", "List Layout")),
		distancePIDLayout_(driveStraightLayout_.GetLayout("Distance", "List Layout"))
		{
	isAbsoluteAngle_ = true;
	Initializations(navXSource, talonEncoderSource, anglePIDOutput, distancePIDOutput, robot, desiredDistance);
	desiredAngle_ = absoluteAngle;

	//NOTE: adding repetative title, this may be an issue later
}

// initialize class for run
void DriveStraightCommand::Init() {
	printf("IN DRIVESTRAIGHT INIT\n");
	isDone_ = false;

	robot_->ResetDriveEncoders();  //TODO sketch!!! works but we need to fix

	leftMotorOutput_ = 0.0;
	rightMotorOutput_ = 0.0;

	// Setting up PID vals
	anglePID_ = new PIDController(rPFac_, rIFac_, rDFac_, navXSource_, anglePIDOutput_);
	distancePID_ = new PIDController(dPFac_, dIFac_, dDFac_, talonEncoderSource_, distancePIDOutput_);

	GetIniValues();
	// absolute angle
	if (!isAbsoluteAngle_) {
		desiredAngle_ = navXSource_->PIDGet();
	}

	// initialize dependencies settings
	initialAvgDistance_ = talonEncoderSource_->PIDGet();
	desiredTotalAvgDistance_ = initialAvgDistance_ + desiredDistance_;

	anglePID_->SetPID(rPFac_, rIFac_, rDFac_);
	distancePID_->SetPID(dPFac_, dIFac_, dDFac_);

	anglePID_->SetSetpoint(desiredAngle_);
	distancePID_->SetSetpoint(desiredTotalAvgDistance_);

	anglePID_->SetContinuous(true);
	anglePID_->SetInputRange(-180, 180);
	distancePID_->SetContinuous(false);

	anglePID_->SetOutputRange(-rMaxOutput_, rMaxOutput_);
	distancePID_->SetOutputRange(-dMaxOutput_, dMaxOutput_);

	anglePID_->SetAbsoluteTolerance(rTolerance_);
	distancePID_->SetAbsoluteTolerance(dTolerance_);

	anglePID_->Enable();
	distancePID_->Enable();

	 // Assuming 5.0 ft / sec from the low gear speed
	driveTimeoutSec_ = fabs(desiredDistance_ / 3.0)+2; //TODO: add physics, also TODO remove +5
	initialDriveTime_ = robot_->GetTime();
	printf("%f Start chicken tenders drivestraight time driveTimeoutSec is %f\n", initialDriveTime_, driveTimeoutSec_);

	numTimesOnTarget_ = 0;

	lastDistance_ = talonEncoderSource_->PIDGet();
	lastDOutput_ = 0.0;
	printf("Initial Right Distance: %f\n "
			"Initial Left Distance: %f\n"
			"Initial Average Distance: %f\n"
			"Desired Distance: %f\n"
			"Desired Angle: %f\n"
			"Initial getPID(): %f\n"
			"Initial angle: %f \n"
			"Distance error: %f\n"
			"Angle error: %f \n",
			robot_->GetRightEncoderValue(), robot_->GetLeftEncoderValue(),
			initialAvgDistance_, desiredTotalAvgDistance_, desiredAngle_,
			talonEncoderSource_->PIDGet(),  navXSource_->PIDGet(),
			distancePID_->GetError(), anglePID_->GetError());
}

// update current values
void DriveStraightCommand::Update(double currTimeSec, double deltaTimeSec) { 
	// update shuffleboard values
	leftStraightEntry_.SetDouble(leftMotorOutput_);
	rightStraightEntry_.SetDouble(rightMotorOutput_);
	angleErrorEntry_.SetDouble(anglePID_->GetError());
	angleErrorGraphEntry_.SetDouble(anglePID_->GetError());
	desiredAngleEntry_.SetDouble(desiredAngle_);
	encoderErrorEntry_.SetDouble(distancePID_->GetError());
	encoderErrorGraphEntry_.SetDouble(distancePID_->GetError());
	desiredTotalFeetEntry_.SetDouble(desiredTotalAvgDistance_);

	diffDriveTime_ = robot_->GetTime() - initialDriveTime_;

// on target
	if (distancePID_->OnTarget() && fabs(talonEncoderSource_->PIDGet() - lastDistance_) < 0.04 ) {
		numTimesOnTarget_++;
		printf("%f Drivestraight error: %f\n", robot_->GetTime(), distancePID_->GetError());
	} else {
		numTimesOnTarget_ = 0;
	}

    //  error check
	if ((fabs(distancePID_->GetError()) < 1.0) && (robot_->CollisionDetected())) {
		numTimesStopped_++;
		printf("%f Collision Detected \n", robot_->GetTime());
	} else {
		numTimesStopped_ = 0;
	}

	lastDistance_ = talonEncoderSource_->PIDGet();
	if((numTimesOnTarget_ > 1) || (diffDriveTime_ > driveTimeoutSec_) || (numTimesStopped_ > 0)) { //LEAVING AS 10.0 FOR NOW BC WE DON'T KNOW ACTUAL VALUES
		if (diffDriveTime_ > driveTimeoutSec_) { //LEAVING AS 10.0 FOR NOW BC WE DON'T KNOW ACTUAL VALUES
			printf(" %f DRIVESTRAIGHT TIMED OUT!! :) go get chicken tenders %f\n", robot_->GetTime(), diffDriveTime_);
		}
		printf("%f Final Left Distance: %f\n" //encoder values not distances
				"Final Right Distance: %f\n"
				"Final Average Distance: %f\n"
				"Final Drivestraight error: %f\n",
				robot_->GetTime(), robot_->GetLeftEncoderValue(), robot_->GetRightEncoderValue(),
				talonEncoderSource_->PIDGet(), distancePID_->GetError());
		Reset();

		leftMotorOutput_ = 0.0;
		rightMotorOutput_ = 0.0;

		isDone_ = true;
	} else { // else run motor
		// receive PID outputs
		double dOutput = distancePIDOutput_->GetPIDOutput();
		double rOutput = anglePIDOutput_->GetPIDOutput();

		// shuffleboard update
		aPIDOutputEntry_.SetDouble(rOutput);
		dPIDOutputEntry_.SetDouble(dOutput);

		if (dOutput - lastDOutput_ > 0.5) { // only when accelerating forward
			dOutput = lastDOutput_ + 0.5; //0.4 for KOP

		}
		// set drive outputs
		rightMotorOutput_ = dOutput - rOutput; //sketch, check!
		leftMotorOutput_ = dOutput + rOutput; //TODO sketch check! TODODODODODO SKETCH MAKE SURE NOT OVER 1.0
		lastDOutput_ = dOutput;

//		double maxOutput = fmax(fabs(rightMotorOutput_), fabs(leftMotorOutput_));
	}
	// drive motors
	robot_->SetDriveValues(RobotModel::Wheels::kLeftWheels, leftMotorOutput_);
	robot_->SetDriveValues(RobotModel::Wheels::kRightWheels, rightMotorOutput_);
	//printf("times on target at %f \n\n", numTimesOnTarget_);
}

// repeatedly on target
bool DriveStraightCommand::IsDone() {
	return isDone_;
}

// reset robot to standby
void DriveStraightCommand::Reset() {
	// turn off mototrs
	robot_->SetDriveValues(0.0, 0.0);

	// destroy angle PID
	if (anglePID_ != NULL) {
		anglePID_->Disable();

		delete anglePID_;

		anglePID_ = NULL;

		printf("Reset Angle PID %f \n", robot_->GetNavXYaw());
	}

	// destroy distance PID
	if (distancePID_ != NULL) {
		distancePID_->Disable();

		delete distancePID_;

		distancePID_ = NULL;
//		printf("Reset Distance PID");

	}
	isDone_ = true;
}
 
//Get pid values from shuffleboard
void DriveStraightCommand::GetIniValues() { // Ini values are refreshed at the start of auto

	dPFac_ = 0.8;
	dIFac_ = 0.0;
	dDFac_ = 0.2;

	rPFac_ = 0.08;
	rIFac_ = 0.0;
	rDFac_ = 0.02;

	printf("DRIVESTRAIGHT COMMAND DRIVE p: %f, i: %f, d: %f\n", dPFac_, dIFac_, dDFac_);
	printf("DRIVESTRAIGHT COMMAND ANGLE p: %f, i: %f, d: %f\n", rPFac_, rIFac_, rDFac_);
}

// initialize dependencies
void DriveStraightCommand::Initializations(NavXPIDSource* navXSource, TalonEncoderPIDSource* talonEncoderSource,
			AnglePIDOutput* anglePIDOutput, DistancePIDOutput* distancePIDOutput, RobotModel* robot,
			double desiredDistance) {
	robot_ = robot;

	navXSource_ = navXSource;
	talonEncoderSource_ = talonEncoderSource;

	anglePIDOutput_ = anglePIDOutput;
	distancePIDOutput_ = distancePIDOutput;

	desiredAngle_ = navXSource_->PIDGet();
	initialAvgDistance_ = talonEncoderSource_->PIDGet();

	desiredDistance_ = desiredDistance;
	desiredTotalAvgDistance_ = 2.0; //TODO CHANGE //initialAvgDistance_ + desiredDistance_;
	printf("Total desired distance is: %f", desiredTotalAvgDistance_);

	leftMotorOutput_ = 0.0;
	rightMotorOutput_ = 0.0;
	isDone_ = false;
	initialDriveTime_ = robot_->GetTime();
	diffDriveTime_ = robot_->GetTime() - initialDriveTime_;

	// Setting up the PID controllers to NULL
	GetIniValues();
	anglePID_ = NULL;
	distancePID_ = NULL;

	rTolerance_ = 0.5;
	dTolerance_ = 3.0 / 12.0;

	rMaxOutput_ = 0.15;
	dMaxOutput_ = 0.85;

	// initializing number of times robot is on target
	numTimesOnTarget_ = 0;
	numTimesStopped_ = 0;

	lastDistance_ = talonEncoderSource_->PIDGet();
	lastDOutput_ = 0.0;
}


DriveStraightCommand::~DriveStraightCommand() {
	//leftStraightEntry_->Remove();
	anglePID_->Disable();
	distancePID_->Disable();
	anglePID_->~PIDController();
	distancePID_->~PIDController();

	leftStraightEntry_.Delete();
	rightStraightEntry_.Delete();
	angleErrorEntry_.Delete();
	angleErrorGraphEntry_.Delete();
	desiredAngleEntry_.Delete();
	encoderErrorEntry_.Delete();
	encoderErrorGraphEntry_.Delete();
	desiredTotalFeetEntry_.Delete();
	dPIDOutputEntry_.Delete();
	aPIDOutputEntry_.Delete();
}