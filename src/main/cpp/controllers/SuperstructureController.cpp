/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "controllers/SuperstructureController.h"
#include <math.h>
using namespace std;

SuperstructureController::SuperstructureController(RobotModel *robot, ControlBoard *humanControl) :
    flywheelPIDLayout_(robot->GetSuperstructureTab().GetLayout("Flywheel", "List Layout").WithPosition(0, 0)),
    sensorsLayout_(robot->GetSuperstructureTab().GetLayout("Sensors", "List Layout").WithPosition(0, 1)),
    manualOverrideLayout_(robot->GetModeTab().GetLayout("climb override", "List Layout").WithPosition(1,1)),
    powerLayout_(robot->GetSuperstructureTab().GetLayout("power control", "List Layout").WithPosition(3, 0))
    {
    
    robot_ = robot;
    humanControl_ = humanControl; 

    // fix all of this
    climbElevatorUpPower_ = 0.5; // fix
    climbElevatorDownPower_ = -0.4; // fix
    climbPowerDesired_ = 0.0; // needs to equal 0
    climbWinchPower_ = 0.75; // fix
    
    desiredFlywheelVelocity_ = 0.0;//7000; // ticks per 0.1 seconds
    closeFlywheelVelocity_ = 3550;//0.55;//5000.0*2048.0/60000.0;//2000; //5000 r/m * 2048 tick/r / (60 s/m * 1000 ms/s)
    flywheelResetTime_ = 2.0; // fix //why does this exist
    stopDetectionTime_ = 0.0;
    // create talon pid controller
    // fix encoder source
    std::cout << "start flywheel encoder creation" << std::endl << std::flush;
    //flywheelEncoder1_ = &robot_->GetFlywheelMotor1()->GetSensorCollection();
    //flywheelEncoder2_ = &robot_->GetFlywheelMotor2()->GetSensorCollection();
    std::cout << "end flywheel encoder creation" << std::endl << std::flush;

    elevatorFeederPower_ = 1.0; // fix
    elevatorSlowPower_ = 0.4; //fix
    elevatorFastPower_ = 0.75; //fix
    indexFunnelPower_ = 0.3; // fix
    lowerElevatorTimeout_ = 2.0; //fix
    elevatorTimeout_ = 2.0;
    //lastBottomStatus_ = false;
    manualRollerPower_ = 0.5;
    autoArmPower_ = 0.3;

    wristPFac_ = 0.006;

    controlPanelPower_ = 0.5; // fix
    controlPanelCounter_ = 0;
    controlPanelStage2_ = false;
    controlPanelStage3_ = false;

    desiredIntakeWristAngle_ = 150.0;//237.0; //down

    closePrepping_ = false;
    farPrepping_ = false;
    atTargetSpeed_ = false;
    numTimeAtSpeed_ = 0.0;

    currTime_ = robot_->GetTime();
    shootPrepStartTime_ = currTime_;
    startResetTime_ = currTime_;
    resetTimeout_ = 2.0;

    currState_ = kDefaultTeleop;
    nextState_ = kDefaultTeleop;
    currAutoState_ = kAutoInit;
    nextAutoState_ = kAutoIndexing;
    currWristState_ = kRaising;
    
    currTime_ = robot_->GetTime();
    startElevatorTime_ = currTime_;
    startIndexTime_ = currTime_-lowerElevatorTimeout_-1.0;
    startResetTime_ = currTime_-elevatorTimeout_-1.0;

    // shuffleboard
    flywheelVelocityEntry_ = flywheelPIDLayout_.Add("flywheel velocity", 0.0).WithWidget(frc::BuiltInWidgets::kGraph).GetEntry();
    flywheelVelocityErrorEntry_ = flywheelPIDLayout_.Add("flywheel error", 0.0).WithWidget(frc::BuiltInWidgets::kGraph).GetEntry();
    
    flywheelPEntry_ = flywheelPIDLayout_.Add("flywheel P", 0.0).GetEntry();
    flywheelIEntry_ = flywheelPIDLayout_.Add("flywheel I", 0.0).GetEntry();
    flywheelDEntry_ = flywheelPIDLayout_.Add("flywheel D", 0.0).GetEntry();
    flywheelFEntry_ = flywheelPIDLayout_.Add("flywheel FF", 1.0).GetEntry();

    wristPEntry_ = robot_->GetSuperstructureTab().Add("wrist P", 0.006).GetEntry();

    autoWinchEntry_ = manualOverrideLayout_.Add("auto climber", false).WithWidget(frc::BuiltInWidgets::kToggleSwitch).GetEntry();
    autoWristEntry_ = manualOverrideLayout_.Add("auto wrist", false).WithWidget(frc::BuiltInWidgets::kToggleSwitch).GetEntry();

    slowElevatorEntry_ = powerLayout_.Add("slow elevator", elevatorSlowPower_).GetEntry();
    fastElevatorEntry_ = powerLayout_.Add("fast elevator", elevatorFastPower_).GetEntry();
    funnelEntry_ = powerLayout_.Add("funnel", indexFunnelPower_).GetEntry();
    rollerManualEntry_ = powerLayout_.Add("manual rollers", manualRollerPower_).GetEntry();
    closeFlywheelEntry_ = powerLayout_.Add("close flywheel", closeFlywheelVelocity_).GetEntry();
    autoArmPowerEntry_ = powerLayout_.Add("arm in auto", autoArmPower_).GetEntry();
    targetSpeedEntry_ = flywheelPIDLayout_.Add("target speed", atTargetSpeed_).GetEntry();
    flywheelMotorOutputEntry_ = flywheelPIDLayout_.Add("flywheel motor output", robot_->FlywheelMotorOutput()).WithWidget(frc::BuiltInWidgets::kGraph).GetEntry();

    //TODO make timeout


    elevatorTopLightSensorEntry_ = sensorsLayout_.Add("bottom elevator", false).GetEntry();
    elevatorBottomLightSensorEntry_ = sensorsLayout_.Add("top elevator", false).GetEntry();
    intakeWristAngleEntry_ = sensorsLayout_.Add("intake wrist angle", 0.0).GetEntry();
    printf("end of superstructure controller constructor\n");

    shootingIsDone_ = false;
}

void SuperstructureController::Reset() { // might not need this
    currState_ = kDefaultTeleop;
    nextState_ = kDefaultTeleop;
    currHandlingState_ = kIndexing;
    // check whether autostates should be reset here or should 
    currAutoState_ = kAutoInit;
    nextAutoState_ = kAutoIndexing;
    currWristState_ = kRaising;
}

void SuperstructureController::WristUpdate(){
    //printf("wrist update\n");
    if(!autoWristEntry_.GetBoolean(true)){
        //printf("HERE OISDHAFOSDKJFLASKDHFJ\n");
        // human: decide rollers auto or manual
        if (humanControl_->GetDesired(ControlBoard::Buttons::kWristUpButton)){
            robot_->SetIntakeWristOutput(-0.5);
            //printf("wrist up\n");
        }
        else if (humanControl_->GetDesired(ControlBoard::Buttons::kWristDownButton)){
            robot_->SetIntakeWristOutput(0.5);
            //printf("wrist down\n");
        }
        else{
            robot_->SetIntakeWristOutput(0.0);
        }
        if(humanControl_->GetDesired(ControlBoard::Buttons::kReverseRollersButton)){
            robot_->SetIntakeRollersOutput(-0.5);
        } else if(humanControl_->GetDesired(ControlBoard::Buttons::kRunRollersButton)){
            //printf("RUNNING ROLLERS RIGH NOWWWWW at %f\n", manualRollerPower_);
            robot_->SetIntakeRollersOutput(manualRollerPower_);
        }else {
            robot_->SetIntakeRollersOutput(0.0);
        }
    } else {
        //("inside auto wrist in mode %d\n", currWristState_);
        currWristAngle_ = robot_->GetIntakeWristAngle(); // might not need?
        switch (currWristState_){
            case kRaising:
                // might not need lowering if we have an idle
                robot_->SetIntakeRollersOutput(0.0);
                //printf("current wrist angle %f\n", currWristAngle_);
                if(currWristAngle_ > 10.0) {
                    robot_->SetIntakeWristOutput(-autoArmPower_);//(0.0-currWristAngle_)*wristPFac_); 
                    //printf("DONE LOLS\n");
                    //robot_->SetIntakeWristOutput(-0.5);
                }
                else{
                    robot_->SetIntakeWristOutput(0.0);
                }
                break;
            case kLowering:
                //printf("lowering, pfac: %f, desired angle: %f, current angle %f\n", wristPFac_, desiredIntakeWristAngle_, currWristAngle_);
                if(currWristAngle_ < desiredIntakeWristAngle_-45.0) {
                    robot_->SetIntakeWristOutput(autoArmPower_);//(desiredIntakeWristAngle_-currWristAngle_)*wristPFac_);
                    //robot_->SetIntakeWristOutput(0.5);
                } else{
                    robot_->SetIntakeWristOutput(0.0);
                    //printf("LOWERED, not running wrist\n");
                }
                if(currWristAngle_ > desiredIntakeWristAngle_ - 45.0 - 45.0){ //within acceptable range, ~740 degrees in sensor is 90 degrees on wrist
                    robot_->SetIntakeRollersOutput(CalculateIntakeRollersPower());
                }
                else{
                    robot_->SetIntakeRollersOutput(0.0);
                }
                break;
            default:
                printf("ERROR: no state in wrist controller \n");
                robot_->SetIntakeWristOutput(0.0);
                robot_->SetIntakeRollersOutput(0.0);
            }

    }
}

void SuperstructureController::UpdatePrep(bool isAuto){
    if (!isAuto){
        UpdateButtons(); //moved button/state code into that function B)
    } /*else {
        if(farPrepping_){ //farPrepping_ biconditional kPrepping :(((((
            desiredFlywheelVelocity_ = CalculateFlywheelVelocityDesired();
            SetFlywheelPowerDesired(desiredFlywheelVelocity_);
            robot_->DisengageFlywheelHood(); //TODO add if distance > x
            closePrepping_ = false;
            farPrepping_ = true;
        }
        else{
            desiredFlywheelVelocity_ = 0.0;
            SetFlywheelPowerDesired(desiredFlywheelVelocity_);
            robot_->SetFlywheelOutput(0.0);
            robot_->DisengageFlywheelHood(); //TODO add if distance > x
        }*/

        //SetFlywheelPowerDesired(desiredFlywheelVelocity_); //TODO INTEGRATE VISION

        //MOVED FLYWHEEL VELOCITY SETTING TO SetPreppingState()
    
}
// START OF NEW STATE MACHINE!! - DO NOT TOUCH PLS
void SuperstructureController::Update(bool isAuto){
    //printf("UPDATING \n");
    currTime_ = robot_->GetTime(); // may or may not be necessary
    RefreshShuffleboard();
    if (!isAuto){
        CheckClimbDesired();
        CheckControlPanelDesired();
    }
    

    switch(currState_){
        case kDefaultTeleop:
            
            // should these be inside or outside power cell handling
            //CheckControlPanelDesired(); // might have to move out of the DefaultTeleop
            //CheckClimbDesired();

            UpdatePrep(isAuto);
            WristUpdate();
            //printf("default teleop\n");
            if (!isAuto){
                UpdateButtons();   
            }
            //UpdateButtons();   

            IndexPrep();

            //TODO replace "//robot_->SetArm(bool a);" with if !sensorGood set arm power small in bool a direction
            //in current code: true is arm down and false is arm up
            //TODO ADD CLIMBING AND SPINNER SEMIAUTO
            switch(currHandlingState_){
                case kIntaking:
                    printf("intaking state\n");
                    Intaking();
                    break;
                case kIndexing:
                    Indexing();
                    break;
                case kShooting:
                    //std::cout << "we in kShooting B)" << std::endl;
                    shootingIsDone_ = Shooting();
                    break;
                case kResetting:
                    Resetting();
                    break;
                case kUndoElevator:
                    UndoElevator();
                    break;
                default:
                    printf("ERROR: no state in Power Cell Handling \n");
            }
            //printf("DESIRED FLYWHEEL VELOCITY %f\n", desiredFlywheelVelocity_);
            currHandlingState_ = nextHandlingState_;
            break;
        case kControlPanel:
            printf("control panel state \n");
            if(controlPanelStage2_){
                ControlPanelStage2(controlPanelPower_);
                nextState_ = kDefaultTeleop;
            }
            if(controlPanelStage3_){
                ControlPanelStage3(controlPanelPower_);
                nextState_ = kDefaultTeleop;
            }
            break;
        case kClimbing:
            printf("climbing state \n");
            switch(currClimbingState_){
                case kClimbingIdle:
                    robot_->SetClimberElevatorOutput(0.0);
                    robot_->SetClimbWinchLeftOutput(0.0);
                    robot_->SetClimbWinchRightOutput(0.0);
                    printf("climbing mechanisms are idle \n");
                    break;
                case kClimbingElevator:
                    robot_->SetClimberElevatorOutput(climbPowerDesired_);
                    if(!humanControl_->GetDesired(ControlBoard::Buttons::kClimbElevatorUpButton)||!humanControl_->GetDesired(ControlBoard::Buttons::kClimbElevatorDownButton)){
                        currClimbingState_ = kClimbingIdle;
                        nextState_ = kDefaultTeleop; // verify that it should be next state
                    }
                    break;
                case kClimbingWinches:
                    // no more auto code?
                    break;
                default:
                    printf("ERROR: no state in climbing \n");
            }
            break;
        default:
            printf("ERROR: no state in superstructure controller\n");
            desiredFlywheelVelocity_ = 0.0;
            SetFlywheelPowerDesired(0.0);
            robot_->SetFlywheelOutput(0.0);
            //robot_->SetIntakeRollersOutput(0.0);
            robot_->SetIndexFunnelOutput(0.0);
            robot_->SetElevatorFeederOutput(0.0);
            currWristState_ = kRaising;

    }
    currState_ = nextState_;
}

void SuperstructureController::UpdateButtons(){
    if(humanControl_->GetDesired(ControlBoard::Buttons::kIntakeSeriesButton)){
        currHandlingState_ = kIntaking;
        //printf("STARTED INTAKING YAY\n");
    } else if (humanControl_->GetDesired(ControlBoard::Buttons::kShootingButton)/* && 
            currTime_ - shootPrepStartTime_ > 1.0*/){ //TODO remove this
        if(currHandlingState_!=kShooting){
            startIndexTime_ = currTime_;
        }
        currHandlingState_ = kShooting; 
    } else if(/*!humanControl_->GetDesired(ControlBoard::Buttons::kShootingButton) && */
            currHandlingState_ == kShooting){ //shooting to decide not to shoot
        currHandlingState_ = kResetting;
    } else if(currHandlingState_ != kResetting){ //not intaking, shooting, or resetting, only option is indexing or prepping (also includes indexing)
        currHandlingState_ = kIndexing;
    }

    PowerCellHandlingState previousState = currHandlingState_;
    if(humanControl_->GetDesired(ControlBoard::Buttons::kUndoElevatorButton)){
        printf("elevator is being undone\n");
        currHandlingState_ = kUndoElevator;
    } else if(currHandlingState_ == kUndoElevator && !humanControl_->GetDesired(ControlBoard::Buttons::kUndoElevatorButton)) {
        currHandlingState_ = previousState;
    }

    //flywheel control if not shooting
    if (currHandlingState_ != kShooting){
        if(humanControl_->GetDesired(ControlBoard::Buttons::kShootClosePrepButton)){
            if(!closePrepping_){
                shootPrepStartTime_ = currTime_;
                printf("start close prep shooting\n");
            }
            printf("in close PREPPING -------------\n");
            desiredFlywheelVelocity_ = closeFlywheelVelocity_;
            //robot_->SetFlywheelOutput(desiredFlywheelVelocity_);
            SetFlywheelPowerDesired(desiredFlywheelVelocity_);
            robot_->DisengageFlywheelHood();
            closePrepping_ = true;
            farPrepping_ = false;
        } else if (humanControl_->GetDesired(ControlBoard::Buttons::kShootFarPrepButton)){
            printf("in far PREPPING -------------\n");
            if(!farPrepping_){
                printf("start far prep shooting\n");
                shootPrepStartTime_ = currTime_;
            }
            desiredFlywheelVelocity_ = CalculateFlywheelVelocityDesired();
            //robot_->SetFlywheelOutput(desiredFlywheelVelocity_);
            SetFlywheelPowerDesired(desiredFlywheelVelocity_); //TODO INTEGRATE VISION
            robot_->EngageFlywheelHood(); //TODO add if distance > x
            closePrepping_ = false;
            farPrepping_ = true;
        } else {
            //printf("STOPPING FLYWHEEL\n");
            desiredFlywheelVelocity_ = 0.0;
            SetFlywheelPowerDesired(desiredFlywheelVelocity_);
            robot_->SetFlywheelOutput(0.0);
            robot_->DisengageFlywheelHood();
        }
    } 

}

void SuperstructureController::CheckControlPanelDesired(){
    if(humanControl_->GetDesired(ControlBoard::Buttons::kControlPanelStage2Button)){
        controlPanelStage2_ = true;
        controlPanelStage3_ = false;
        currState_ = kControlPanel; // would this be currState_ or nextState_
    }
    if(humanControl_->GetDesired(ControlBoard::Buttons::kControlPanelStage3Button)){
        controlPanelStage2_ = false;
        controlPanelStage3_ = true;
        currState_ = kControlPanel;
    }
}

void SuperstructureController::CheckClimbDesired(){
    if(humanControl_->GetDesired(ControlBoard::Buttons::kClimbElevatorUpButton)){
        climbPowerDesired_ = climbElevatorDownPower_;
        currClimbingState_ = kClimbingElevator;
        currState_ = kClimbing;
    } else if(humanControl_->GetDesired(ControlBoard::Buttons::kClimbElevatorDownButton)){
        climbPowerDesired_ = climbElevatorUpPower_;
        currClimbingState_ = kClimbingElevator;
        currState_ = kClimbing;
    } else{
        robot_->SetClimberElevatorOutput(0.0);
    }
    if(humanControl_->GetDesired(ControlBoard::Buttons::kClimbWinchLeftButton)){
        robot_->SetClimbWinchLeftOutput(climbWinchPower_);
    } else {
        robot_->SetClimbWinchLeftOutput(0.0);
    }
    if(humanControl_->GetDesired(ControlBoard::Buttons::kClimbWinchRightButton)){
        robot_->SetClimbWinchRightOutput(climbWinchPower_);
    }else {
        robot_->SetClimbWinchRightOutput(0.0);
    }
}

/*
void SuperstructureController::DisabledUpdate() {
    if (humanControl_ -> GetDesired(ControlBoard::Buttons::kAlignButton)){
        cout << "in light disabled" << endl;
        robot_ -> SetLight(true);
    } else {
        robot_ -> SetLight(false);
    }
}
*/

void SuperstructureController::IndexPrep(){
    bottomSensor_ = robot_->GetElevatorFeederLightSensorStatus();
    topSensor_ = robot_->GetElevatorLightSensorStatus();
    
    if(topSensor_){
        startElevatorTime_ = currTime_;
    }
    if(bottomSensor_){
        startIndexTime_ = currTime_;
    }

    tTimeout_ = currTime_-startElevatorTime_ > elevatorTimeout_;
    bTimeout_ = currTime_-startIndexTime_ > lowerElevatorTimeout_;
}

void SuperstructureController::Intaking(){
    printf("in kIntaking\n");
    //robot_->SetIntakeRollersOutput(CalculateIntakeRollersPower());
    currWristState_ = kLowering;
    IndexUpdate();
}

void SuperstructureController::Indexing(){
    IndexUpdate();
    //printf("in kIndexing\n");

    //robot_->SetIntakeRollersOutput(0.0);
    currWristState_ = kRaising;
}

bool SuperstructureController::Shooting() {
    //printf("in kShooting AAAAAAAAAAAAAAAA NOTICE ME !!! !! AAAA\n");// with %f\n", desiredFlywheelVelocity_);
    //robot_->SetFlywheelOutput(desiredFlywheelVelocity_);
    SetFlywheelPowerDesired(desiredFlywheelVelocity_);
    //std::cout << "velocity " << robot_->GetFlywheelMotor1Velocity()*FALCON_TO_RPM << std::endl;
    //raise elevator if not at speed, OR nothing at top and not timed out at bottom
    if(IsFlywheelAtSpeed() || (!topSensor_ && !bTimeout_)){
        robot_->SetElevatorOutput(elevatorSlowPower_);
    } else {                                                                                                
        robot_->SetElevatorOutput(0.0);
    }

    if(!bottomSensor_ && !bTimeout_){
        //robot_->SetIndexFunnelOutput(indexFunnelPower_); //TODO PUT BACK IN
        robot_->SetElevatorFeederOutput(elevatorFeederPower_);
    } else {
        robot_->SetIndexFunnelOutput(0.0);
        robot_->SetElevatorFeederOutput(0.0);
    }

    currWristState_ = kRaising;

    //we have stopDetectionTime_ < 0.001 because we don't want to keep setting it to currTime_ (if we do that we'll never stop shooting)
    if(tTimeout_ && bTimeout_ && stopDetectionTime_ < 0.001){ //stopping shooting i guess D:
        stopDetectionTime_ = robot_->GetTime();
        std::cout << "stop detecting " << stopDetectionTime_ << std::endl;
    }

    if(robot_->GetTime() >= stopDetectionTime_ + 2.0 && stopDetectionTime_ > 0.001){
        return true;
    }
    return false;

    //robot_->SetIntakeRollersOutput(0.0);
    
    //robot_->SetArm(false); 
}

void SuperstructureController::Resetting() {
    printf("in kResetting\n");
    desiredFlywheelVelocity_ = 0.0;
    SetFlywheelPowerDesired(desiredFlywheelVelocity_);
    robot_->SetFlywheelOutput(0.0);

    if(!bottomSensor_ && currTime_-startResetTime_ <= resetTimeout_){
        robot_->SetElevatorOutput(-elevatorFastPower_); //bring down elevator
    } else {
        robot_->SetElevatorOutput(0.0);
        nextHandlingState_ = kIndexing;
    }

    //robot_->SetIntakeRollersOutput(0.0);
    robot_->SetIndexFunnelOutput(0.0);
    robot_->SetElevatorFeederOutput(0.0);
    currWristState_ = kRaising;
    //robot_->SetArm(false); TODO IMPLEMENT
}

void SuperstructureController::UndoElevator(){
    robot_->SetElevatorOutput(-elevatorSlowPower_);
    robot_->SetElevatorFeederOutput(-elevatorFeederPower_);
    robot_->SetIndexFunnelOutput(-indexFunnelPower_);
}
void SuperstructureController::IndexUpdate(){

    //printf("top sensor %f and bottom sensor %f\n", topSensor_, bottomSensor_);

    //control top
    if(!topSensor_ && bottomSensor_){
        //printf("RUNNING TOP ELEVATOR\n");
        //printf("running elevator");
        robot_->SetElevatorOutput(elevatorFastPower_);
    } else {
        //printf("not running elevator");
        robot_->SetElevatorOutput(0.0);
    }

    //control bottom
    if(!bottomSensor_ && (!bTimeout_ || currHandlingState_ == kIntaking)){
        robot_->SetIndexFunnelOutput(indexFunnelPower_); //TODO PUT BACK IN
        robot_->SetElevatorFeederOutput(elevatorFeederPower_);
        //printf("RUNNNNINGGGG FUNNEL AND FEEDER\n");
    } else {
        robot_->SetIndexFunnelOutput(0.0);
        robot_->SetElevatorFeederOutput(0.0);
    }

    // if((topSensor_ && bottomSensor_) || !bTimeout_){
    //     return true;
    // } else {
    //     return false;
    // }
}

bool SuperstructureController::GetShootingIsDone(){
    return shootingIsDone_;
}

void SuperstructureController::SetShootingState(double autoVelocity){
    //robot_->SetLight(true);
    //distanceToTarget_ = robot_->GetDistance();
    //desiredFlywheelVelocity_ = (distanceToTarget_+1827.19)/0.547; //velocity from distance, using desmos
    //desiredFlywheelVelocity_=autoVelocity;
    currWristState_ = kRaising; //resetting whatever intake did
    currHandlingState_ = kShooting;
    printf("start Shooting\n");
}
void SuperstructureController::SetIndexingState(){
    currWristState_ = kRaising; //resetting whatever intake did
    currHandlingState_ = kIndexing;
    printf("start Indexing");
}
void SuperstructureController::SetIntakingState(){
    currWristState_ = kLowering;
    currHandlingState_ = kIntaking; 
    printf("start Intaking\n");
}

void SuperstructureController::SetPreppingState(double desiredVelocity){ //starts warming up shooter B)
    //robot_->SetLight(true);
    //distanceToTarget_ = robot_->GetDistance();

    //desiredVelocity = (distanceToTarget_+1827.19)/0.547; //velocity from distance, using desmos
    std::cout <<  "velocity " << robot_->GetFlywheelMotor1Velocity() << std::endl;
    currWristState_ = kRaising; //resetting whatever intake did
    if(!farPrepping_){ 
        shootPrepStartTime_ = robot_->GetTime(); //TODO FIX
        printf("start Prepping\n");
    }
    robot_->EngageFlywheelHood();
    farPrepping_ = true;
    closePrepping_ = false;
    desiredFlywheelVelocity_ = desiredVelocity;
    SetFlywheelPowerDesired(desiredFlywheelVelocity_);

}



void SuperstructureController::FlywheelPIDControllerUpdate() {

    flywheelFFac_ = RatioFlywheel();
    //printf("flywheel fFac %f\n", flywheelFFac_);
    robot_->ConfigFlywheelP(flywheelPFac_);
    robot_->ConfigFlywheelI(flywheelIFac_);
    robot_->ConfigFlywheelD(flywheelDFac_);

    double adjustedVelocity = desiredFlywheelVelocity_/FALCON_TO_RPM;
    double maxVelocity = RatioFlywheel()/FALCON_TO_RPM;
    double adjustedValue = (adjustedVelocity/maxVelocity*1023/adjustedVelocity);
    flywheelFFac_ = adjustedValue;
    //printf("adjustedValue %f\n", adjustedValue);
    robot_->ConfigFlywheelF(flywheelFFac_);
    // closed-loop error maybe?

}

double SuperstructureController::RatioFlywheel(){
    return MAX_FALCON_RPM*robot_->GetVoltage()/RATIO_BATTERY_VOLTAGE;
}

double SuperstructureController::CalculateFlywheelVelocityDesired() {
    return closeFlywheelVelocity_ ; // fix
}

//TODO actually implement
void SuperstructureController::SetFlywheelPowerDesired(double flywheelVelocityRPM) {
    robot_->SetControlModeVelocity(flywheelVelocityRPM/FALCON_TO_RPM);
}

bool SuperstructureController::IsFlywheelAtSpeed(){
    if(robot_->GetFlywheelMotor1Velocity()*FALCON_TO_RPM > desiredFlywheelVelocity_-0 && 
        robot_->GetFlywheelMotor1Velocity()*FALCON_TO_RPM < desiredFlywheelVelocity_+150){
        numTimeAtSpeed_++;
        if (numTimeAtSpeed_ >= 3){
            atTargetSpeed_ = true;
            //return true;
        }
        else{
            atTargetSpeed_ = false;
            //return false;
        }
    } 
    numTimeAtSpeed_ = 0;
    atTargetSpeed_ = false;
    return true;
    //return false;
}

//TODO pkease pleasd please
double SuperstructureController::CalculateIntakeRollersPower() { 
    /*double power = abs(robot_->GetDrivePower())*2;
    if (power <= 1)
        robot_->SetIntakeRollersOutput(power);
    else
        robot_->SetIntakeRollersOutput(1.0);*/
    return 0.5;//1.0;
}

void SuperstructureController::ControlPanelStage2(double power){
    previousControlPanelColor_ = initialControlPanelColor_;
    if (controlPanelCounter_ < 8) {
        robot_->SetControlPanelOutput(power);
        if (initialControlPanelColor_.compare(robot_->MatchColor()) == 0 && previousControlPanelColor_.compare(robot_->MatchColor()) != 0) {
            controlPanelCounter_++;
        }
        previousControlPanelColor_ = robot_->MatchColor();
    }
    if (controlPanelCounter_ >= 8) {
        robot_->SetControlPanelOutput(0.0);
    }
}

void SuperstructureController::ControlPanelStage3(double power) {
    // blue: cyan 100 (255, 0, 255)
    // green: cyan 100 yellow 100 (0, 255, 0)
    // red: magenta 100 yellow 100 (255, 0 , 0)
    // yellow: yellow 100 (255, 255, 0)


    if(robot_->GetControlPanelGameData().length() > 0)
    {
        switch(robot_->GetControlPanelGameData()[0])
        {
            case 'B' :
                colorDesired_ = "Red";
                if(colorDesired_.compare(robot_->MatchColor()) != 0) {
                     robot_->SetControlPanelOutput(power);
                }
                initialControlPanelTime_ = robot_->GetTime();
                ControlPanelFinalSpin();
                break;
            case 'G' :
                colorDesired_ = "Yellow";
                if(colorDesired_.compare(robot_->MatchColor()) != 0) {
                     robot_->SetControlPanelOutput(power);
                }
                initialControlPanelTime_ = robot_->GetTime();
                ControlPanelFinalSpin();
                break;
            case 'R' :
                colorDesired_ = "Blue";
                if(colorDesired_.compare(robot_->MatchColor()) != 0) {
                     robot_->SetControlPanelOutput(power);
                }
                initialControlPanelTime_ = robot_->GetTime();
                ControlPanelFinalSpin();
                break;
            case 'Y' :
                colorDesired_ = "Green";
                if(colorDesired_.compare(robot_->MatchColor()) != 0) {
                     robot_->SetControlPanelOutput(power);
                }
                initialControlPanelTime_ = robot_->GetTime();
                ControlPanelFinalSpin();
                break;
            default :
                printf("this data is BAD BOIZ");
                break;
        }
    } else {
        printf("no data received yet");
        // no data received yet
    }
    nextState_ = kDefaultTeleop;
}

void SuperstructureController::ControlPanelFinalSpin() {
    if(robot_->GetTime()-initialControlPanelTime_ < 2.0) { // fix time and change to if
        robot_->SetControlPanelOutput(0.3); // fix power
    }
    robot_->SetControlPanelOutput(0.0);
}

void SuperstructureController::RefreshShuffleboard(){
    manualRollerPower_ = rollerManualEntry_.GetDouble(manualRollerPower_);
    closeFlywheelVelocity_ = closeFlywheelEntry_.GetDouble(closeFlywheelVelocity_);
    elevatorFastPower_ = fastElevatorEntry_.GetDouble(elevatorFastPower_);
    elevatorSlowPower_ = slowElevatorEntry_.GetDouble(elevatorSlowPower_);
    indexFunnelPower_ = funnelEntry_.GetDouble(indexFunnelPower_);
    autoArmPower_ = autoArmPowerEntry_.GetDouble(autoArmPower_);

    elevatorBottomLightSensorEntry_.SetBoolean(robot_->GetElevatorFeederLightSensorStatus());
    elevatorTopLightSensorEntry_.SetBoolean(robot_->GetElevatorLightSensorStatus());
    targetSpeedEntry_.SetBoolean(atTargetSpeed_);

    flywheelPFac_ = flywheelPEntry_.GetDouble(0.0);
    flywheelIFac_ = flywheelIEntry_.GetDouble(0.0);
    flywheelDFac_ = flywheelDEntry_.GetDouble(0.0);
    flywheelFFac_ = flywheelFEntry_.GetDouble(0.0);
    FlywheelPIDControllerUpdate();

    wristPFac_ = wristPEntry_.GetDouble(0.006); //was 0.03
    flywheelVelocityEntry_.SetDouble(robot_->GetFlywheelMotor1Velocity()*FALCON_TO_RPM); //rpm
    flywheelVelocityErrorEntry_.SetDouble(desiredFlywheelVelocity_-robot_->GetFlywheelMotor1Velocity()*FALCON_TO_RPM);
    flywheelMotorOutputEntry_.SetDouble(robot_->FlywheelMotorOutput());
    currWristAngle_ = robot_->GetIntakeWristAngle();
    //printf("wrist angle: %f\n", currWristAngle_);
    intakeWristAngleEntry_.SetDouble(currWristAngle_);
	lastTime_ = currTime_;
	currTime_ = robot_->GetTime();
}

SuperstructureController::~SuperstructureController() {
    intakeWristAngleEntry_.Delete();
    flywheelPEntry_.Delete();
    flywheelIEntry_.Delete();
    flywheelDEntry_.Delete();
    flywheelFEntry_.Delete();
    autoWinchEntry_.Delete();
    flywheelVelocityErrorEntry_.Delete();
    flywheelVelocityEntry_.Delete();
    wristPEntry_.Delete();
    elevatorBottomLightSensorEntry_.Delete();
    elevatorTopLightSensorEntry_.Delete();
}
