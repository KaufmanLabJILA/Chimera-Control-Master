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

	void getTweezerOffset(int* xOffPixels, int* yOffPixels, int* indexSubpixelMask);

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
	std::vector<int16> subpixelMasks;
	std::vector<int16> subpixelMaskSingle; //Centered mask
	std::vector<long> bgImg;
	std::vector<int16> masksCrop;
	int nMask, maskWidX, maskWidY; //dimensions of masks. maskWidX, maskWidY are the WIDTH of each mask, not array dimensions. nMask is number of tweezer masks
	int nSubpixel; // nSubpixel is the number of subpixel Masks, always spread uniformly over 1 pixel spacing. Image width is extracted from this mask, and compared to the actual image settings to confirm consistency.
	// options
	bool plotterActive;
	bool plotterNeedsImages;
	bool rearrangerActive;
	bool autoTweezerOffsetActive;
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
	std::vector<double>* xOffsetAutoQueue;
	std::vector<double>* yOffsetAutoQueue;
	
	std::vector<UINT8> targetPositionsTemp;
	std::vector<bool> positions;
	std::vector<bool> positionsX; //Temporary storage of initial positions XY for multi-step rearrangement procedures.
	std::vector<bool> positionsY;

	size_t nAtom = 0;
	std::array<int, 4> thresholds;
	imageParameters imageDims;
	UINT atomThresholdForSkip = UINT_MAX;
	std::atomic<bool>* skipNext;
	int sourceColumnSum(int iColumn, std::vector<UINT8> rearrangerAtomVect);
};
