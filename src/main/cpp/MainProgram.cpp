/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "MainProgram.h"

#include <iostream>

#include <string>

#include <frc/smartdashboard/SmartDashboard.h>

void MainProgram::RobotInit() {
    robot_ = new RobotModel();
    humanControl_ = new ControlBoard();
    superstructureController_ = new SuperstructureController(robot_, humanControl_);
    robot_->SetSuperstructureController(superstructureController_);
    driveController_ = new DriveController(robot_, humanControl_);
    robot_->CreatePIDEntries(); 
    printf("created PID entries\n"); 
    robot_->ResetDriveEncoders();
    robot_->SetHighGear();
    aligningTape_ = false;

    isSocketBound_ = false;
    
    autoSequenceEntry_ = robot_->GetModeTab().Add("Auto Test Sequence", "t 0").GetEntry();
    sequence_ = autoSequenceEntry_.GetString("t 0");
    printf("I am alive.\n");
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
    robot_->SetHighGear();
    robot_->ResetDriveEncoders();
    robot_->ZeroNavXYaw();
    robot_->CreateNavX();


    //robot_->SetTestSequence("c 1.0 90.0 0");
    //robot_->SetTestSequence(sequence_);

    //robot_->SetTestSequence("d 1.0 c 3.0 180.0 0"); //for testing high gear and low gear
    //robot_->SetTestSequence("c 3.0 90.0 0 0");
    robot_->SetTestSequence("b 3560.0 s 3560.0");// c 4.0 90.0 1 1");
    
    //robot_->SetTestSequence("d 1.0 t 90.0 d 1.0 t 180.0 d 1.0 t -90.0 d 1.0 t 0.0"); //for testing high gear and low gear

    navX_ = new NavXPIDSource(robot_);
    talonEncoderSource_ = new TalonEncoderPIDSource(robot_);

    //robot_->SetTestSequence("d 1.0 t 90.0 d 1.0 t 180.0 d 1.0 t -90 d 1.0 t 0.0");

    testSequence_ = new TestMode(robot_, humanControl_);
    testSequence_->QueueFromString(robot_->GetTestSequence());


    //robot_->SetLight(true); //turn on light for auto

    testSequence_->Init();
    std::cout<< "init time: " << robot_->GetTime() << std::endl;

    // printf("done with init, moving to periodic\n");

    //robot_->SetTestSequence("d 1.0 t 90.0 d 1.0 t 180.0 d 1.0 t -90 d 1.0 t 0.0");

    //testSequence_ = new TestMode(robot_, humanControl_);
    //testSequence_->QueueFromString(robot_->GetTestSequence());

    // printf("before init\n");
    // testSequence_->Init();

    // printf("done with init, moving to periodic\n");


    // thingS_ = new VelocityPIDSource(robot_);
    // thingO_ = new VelocityPIDOutput();
    // thingAO_ = new AnglePIDOutput();
    // thing_ = new MotionProfileTestCommand(robot_, thingS_, robot_->GetNavXSource(), thingO_, thingAO_);
    // thing_->Init();

    currTime_ = robot_->GetTime();
    lastTime_ = currTime_;

    // tempNavXSource_ = new NavXPIDSource(robot_);
    // tempPivot_ = new PivotCommand(robot_, 90.0, true, tempNavXSource_);
    // tempPivot_->Init();
}

void MainProgram::AutonomousPeriodic() {
    robot_->RefreshShuffleboard();
    // if(!tempPivot_->IsDone()){
    //     tempPivot_->Update(0.0, 0.0);
    // }
    // lastTime_ = currTime_;
    // currTime_ = robot_->GetTime();

    // //printf("AM I NULL????? %d\n", testSequence_==nullptr);

    // if(!thing_->IsDone()){
    //     thing_->Update(currTime_, currTime_-lastTime_);
    // }
    if(!testSequence_->IsDone()){
        testSequence_->Update(currTime_, currTime_-lastTime_);
    }
}

void MainProgram::DisabledInit() {
    robot_ -> SetLight(false); //turn camera led light off end of auto
}

void MainProgram::TeleopInit() {
    std::cout << "in teleopinit\n" << std::flush;
    robot_->ResetDriveEncoders();

    robot_->StartCompressor();

    matchTime_ = frc::Timer::GetMatchTime();
    aligningTape_ = false;

    std::cout << "before zmq\n" << std::flush;
    //zmq::context_t * 
    context_ = new zmq::context_t(2); //same context for send + receive zmq
    //context2_ = new zmq::context_t(1);
    connectRecvZMQ();
    //connectSendZMQ();
    std::cout << "end of teleopinit\n" << std::flush;
}

void MainProgram::TeleopPeriodic() {

    //printf("left distance is %f and right distance is %f\n", robot_->GetLeftDistance(), robot_->GetRightDistance());
    humanControl_->ReadControls();
    driveController_->Update();
    //std::cout << "before superstructure\n" << std::flush;
    superstructureController_->Update(false);
    //std::cout << "updated drive and superstructure\n" << std::flush;
    //\superstructureController_->WristUpdate();
    robot_->GetColorFromSensor();
    robot_->MatchColor();
    
    //std::cout << "updated colors\n" << std::flush;
    

    matchTime_ = frc::Timer::GetMatchTime();
    //sendZMQ();//sending here bc. returns after each if below and i don't want to change everything hehe

    //align tapes not at trench (like auto)
    std::cout << "checking tape align\n" << std::flush;
    
    if (!aligningTape_ && humanControl_->JustPressed(ControlBoard::Buttons::kAlignButton)){
        std::cout << "READY TO START ZMQ READ\n" << std::flush;
        string temp = readZMQ();
        if(!readAll(temp)){
            aligningTape_ = true;
            alignTapeCommand = new AlignTapeCommand(robot_, humanControl_, navX_, talonEncoderSource_, false); //nav, talon variables don't exist yet
            alignTapeCommand->Init();
            printf("starting teleop align tapes\n");
            return;
        } else {
            printf("exited jetson align, nothing read\n");
        }
    } else if (aligningTape_){
        // printf("in part align tape :))\n");
        /*
        //humanControl_->ReadControls();
        //autoJoyVal_ = humanControl_->GetJoystickValue(ControlBoard::kLeftJoy, ControlBoard::kY);
        //autoJoyVal_ = driveController_->HandleDeadband(autoJoyVal_, driveController_->GetThrustDeadband()); //TODO certain want this deadband?
        if(autoJoyVal_ != 0.0 || aCommand == NULL){ //TODO mild sketch, check deadbands more
            printf("WARNING: EXITED align.  autoJoyVal_ is %f after deadband or aCommand is NULL %d\n\n", autoJoyVal_, aCommand==NULL);
            delete aCommand;
            aCommand = NULL;
            aligningTape_ = false;
        } else */
        std::cout << "AM I NULL ALIGN??" << (alignTapeCommand==NULL) << std::endl << std::flush;
        if(!alignTapeCommand->IsDone()){
            alignTapeCommand->Update(0.0, 0.0); //(currTimeSec_, deltaTimeSec_); - variables don't exist yet
            printf("updated align tape command\n");
        } else { //isDone() is true
            delete alignTapeCommand;
            alignTapeCommand = NULL;
            aligningTape_ = false;
            printf("destroyed align tape command\n");
        }
        return;
    }

    //trench align tapes
    if (!aligningTape_ && humanControl_->JustPressed(ControlBoard::Buttons::kTrenchAlignButton)){
        aligningTape_ = true;
        trenchAlignTapeCommand = new TrenchAlignTapeCommand(robot_, humanControl_, navX_, talonEncoderSource_, false); //nav, talon variables don't exist yet
        trenchAlignTapeCommand->Init();
        printf("starting teleop align tapes\n");
        return;
    } else if (aligningTape_){
        // printf("in part align tape :))\n");
        /*
        //humanControl_->ReadControls();
        //autoJoyVal_ = humanControl_->GetJoystickValue(ControlBoard::kLeftJoy, ControlBoard::kY);
        //autoJoyVal_ = driveController_->HandleDeadband(autoJoyVal_, driveController_->GetThrustDeadband()); //TODO certain want this deadband?
        if(autoJoyVal_ != 0.0 || aCommand == NULL){ //TODO mild sketch, check deadbands more
            printf("WARNING: EXITED align.  autoJoyVal_ is %f after deadband or aCommand is NULL %d\n\n", autoJoyVal_, aCommand==NULL);
            delete aCommand;
            aCommand = NULL;
            aligningTape_ = false;
        } else */
        if(!trenchAlignTapeCommand->IsDone()){
            trenchAlignTapeCommand->Update(0.0, 0.0); //(currTimeSec_, deltaTimeSec_); - variables don't exist yet
            printf("updated align tape command\n");
        } else { //isDone() is true
            delete trenchAlignTapeCommand;
            trenchAlignTapeCommand = NULL;
            aligningTape_ = false;
            printf("destroyed align tape command\n");
        }
        return;
    }

}
/*
void MainProgram::DisabledPeriodic() {
    humanControl_->ReadControls();
    superstructureController_->DisabledUpdate();
}*/

void MainProgram::TestPeriodic() {}

void MainProgram::ResetControllers() {
    driveController_->Reset();
    superstructureController_->Reset();
}

void MainProgram::connectRecvZMQ() {
    //connect to zmq socket to receive from jetson
    /*try {
		printf("in try connect to jetson\n");
        subscriber_ = new zmq::socket_t(*context_, ZMQ_SUB);
        //change to dynamic jetson address
        subscriber_->connect("tcp://10.18.68.12:5808");
		printf("jetson connected to socket\n");
        int confl = 1;
		subscriber_->setsockopt(ZMQ_CONFLATE, &confl, sizeof(confl));
		subscriber_->setsockopt(ZMQ_RCVTIMEO, 1000); //TODO THIS MIGHT ERROR
		subscriber_->setsockopt(ZMQ_SUBSCRIBE, "", 0); //filter for nothing
    } catch(const zmq::error_t &exc) {
		printf("ERROR: TRY CATCH FAILED IN ZMQ CONNECT RECEIVE\n");
		std::cerr << exc.what();
	}*/
    std::cout << "reached end of connect recv zmq\n" << std::flush;
}

string MainProgram::readZMQ() {
    try {
		printf("in try connect to jetson in readZMQ\n");
        subscriber_ = new zmq::socket_t(*context_, ZMQ_SUB);
        //change to dynamic jetson address
        subscriber_->connect("tcp://10.18.68.12:5808");
		printf("jetson connected to socket\n");
        int confl = 1;
		subscriber_->setsockopt(ZMQ_CONFLATE, &confl, sizeof(confl));
		subscriber_->setsockopt(ZMQ_RCVTIMEO, 1000); //TODO THIS MIGHT ERROR
		subscriber_->setsockopt(ZMQ_SUBSCRIBE, "", 0); //filter for nothing
    } catch(const zmq::error_t &exc) {
		printf("ERROR: TRY CATCH FAILED IN ZMQ CONNECT RECEIVE\n");
		std::cerr << exc.what();
	}
    
    printf("starting read from jetson\n");
	string contents = s_recv(*subscriber_);
	printf("contents from jetson: %s \n", contents.c_str());
    return contents;
}

// void MainProgram::readDistance(string contents){

// }

// void MainProgram::readDetected(string contents){

// }

bool MainProgram::readAll(string contents) {
    printf("ready to read from jetson\n");
    
    stringstream ss(contents); //split string contents into a vector
	vector<string> result;
    bool abort;

	while(ss.good()) {
		string substr;
		getline( ss, substr, ' ' );
		if (substr == "") {
			continue;
		}
		result.push_back( substr );
	}
	
	if(!contents.empty() && result.size() > 1) {
        robot_->SetDeltaAngle( stod(result.at(0)) );
		robot_->SetDistance( stod(result.at(1)) );
        abort = false;
	} else {
		abort = true;
		printf("contents empty in alignwithtape\n");
	}

    //jetson string is hasTarget, angle (deg from center), raw distance (ft)
	if(result.size() > 1) {
        printf("received values\n"); //TODO MAYBE ERROR CHECK ZMQ SEND ON JETSON SIDE
		robot_->SetDeltaAngle( stod(result.at(1)) );
		robot_->SetDistance( stod(result.at(2)) );//1.6;
	} else {
		//abort_ = true;
		robot_->SetDeltaAngle(0.0);
		robot_->SetDistance(0.0);
        printf("returning because nothing received\n");
	}
	printf("desired delta angle at %f in AlignWithTapeCommand\n", robot_->GetDeltaAngle());
    printf("desired delta distance at %f in AlignWithTapeCommand\n", robot_->GetDistance());
		
	/*} catch (const std::exception &exc) {
		printf("TRY CATCH FAILED IN READFROMJETSON\n");
		std::cout << exc.what() << std::endl;
		desiredDeltaAngle_ = 0.0;
		// desiredDistance_ = 0.0;
	}*/

    printf("end of read angle with %d\n", abort);
    return abort;

}

void MainProgram::connectSendZMQ() {
    //zmq socket to send message to jetson
    try{
    std::cout << "start connect send zmq\n" << std::flush;
    publisher_ = new zmq::socket_t(*context_, ZMQ_PUB);
    std::cout << "done connect socket zmq\n" << std::flush;
    if(!isSocketBound_){
        publisher_->bind("tcp://*:5806");
        isSocketBound_ = true;
    }
    int confl = 1;
    std::cout << "setting socket zmq\n" << std::flush;
    publisher_->setsockopt(ZMQ_CONFLATE, &confl, sizeof(confl));
    std::cout << "done setting socket zmq\n" << std::flush;
    } catch (const zmq::error_t &exc) {
		printf("TRY CATCH FAILED IN ZMQ CONNECT SEND\n");
		std::cerr << exc.what();
	}

}

void MainProgram::sendZMQ() {
    string message = "matchtime = " + to_string(matchTime_) + ", aligningTape = " + to_string(aligningTape_);
    cout << message << endl;
    zmq_send((void *)publisher_, message.c_str(), message.size(), 0);
}


#ifndef RUNNING_FRC_TESTS
int main() { return frc::StartRobot<MainProgram>(); }
#endif
