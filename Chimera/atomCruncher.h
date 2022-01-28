#pragma once
#include <atomic>
#include <vector>
#include <numeric>
#include <mutex>
#include "Windows.h"
#include <array>
#include "atomGrid.h"
#include "imageParameters.h"
#include "GigaMoog.h"

class atomCruncher
{
public:

	moveSequence getRearrangeMoves(std::string rearrangeType);
	void scrunchX(moveSequence& moveseq);
	void scrunchY(moveSequence& moveseq);
	void filterAtomQueue(void);

	chronoTimes* catchPicTime;
	chronoTimes* finTime;
	atomGrid gridInfo;
	// what the thread watches...
	std::atomic<bool>* cruncherThreadActive;
	std::vector<std::vector<long>>* imageQueue;
	std::vector<std::vector<bool>>* atomArrayQueue;

	std::vector<int16> masks; //Pass by value, do not want these modified by other threads.
	std::vector<long> bgImg;
	std::vector<int16> masksCrop;
	int nMask, maskWidX, maskWidY; //dimensions of masks. maskWidX, maskWidY are the WIDTH of each mask, not array dimensions
	// options
	bool plotterActive;
	bool plotterNeedsImages;
	bool rearrangerActive;
	UINT picsPerRep;
	// locks
	std::mutex* imageLock;
	std::mutex* plotLock;
	std::mutex* rearrangerLock;
	std::condition_variable* rearrangerConditionWatcher;
	// what the thread fills.

	gigaMoog* gmoog;

	std::vector<std::vector<long>>* plotterImageQueue;
	std::vector<std::vector<bool>>* plotterAtomQueue;
	std::vector<std::vector<bool>>* rearrangerAtomQueue;
	std::vector<UINT8> targetPositionsTemp;

	size_t nAtom = 0;
	std::array<int, 4> thresholds;
	imageParameters imageDims;
	UINT atomThresholdForSkip = UINT_MAX;
	std::atomic<bool>* skipNext;
	int sourceColumnSum(int iColumn, std::vector<UINT8> rearrangerAtomVect);
};
