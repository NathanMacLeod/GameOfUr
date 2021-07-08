#pragma once


class Timer {
public:
	Timer() {
		timeLeft = 0;
	}

	void setTimer(float time) {
		timeLeft = time;
	}

	bool update(float timePassed) {
		timeLeft -= timePassed;
		return timeLeft <= 0;
	}

	float getCurrTime() {
		return timeLeft;
	}

private:
	float timeLeft;
};