/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#pragma once

#include "RobotModel.h"
#include "auto/AutoCommand.h"

class WaitingCommand : public AutoCommand {
public:
	/**
	 * Assigns the waitTimeSec and creates the timer
	 */
	WaitingCommand(double myWaitTimeSec);

	/**
	 * Destructor
	 */
	virtual ~WaitingCommand();

	/**
	 * Starts the timer
	 */
	void Init();

	/**
	 * Checks if the timer meets the waitTimeSec. If so, isDone is set to true.
	 */
	void Update(double currTimeSec, double deltaTimeSec);

	/**
	 * @return isDone
	 */
	bool IsDone();

	void Reset();
private:
	double waitTimeSec_;
	Timer *timer_;
	bool isDone_;
};