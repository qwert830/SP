#ifndef GAMETIMER_H
#define GAMETIMER_H

const double FPS = double(1) / double(60);

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const; // in seconds
	float DeltaTime()const; // in seconds
	bool FrameTime(); 

	void Reset(); 
	void Start(); 
	void Stop();  
	void Tick();  

private:
	double mSecondsPerCount;
	double mDeltaTime;
	double mFrameTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;



	bool mStopped;
};

#endif // GAMETIMER_H