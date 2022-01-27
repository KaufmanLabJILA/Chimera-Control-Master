#include "stdafx.h"
#include "atomCruncher.h"

void atomCruncher::filterAtomQueue(void) {
	//exclude atoms that were not on a target site.
	for (size_t i = 0; i < gmoog->initialPositions.size(); i++)
	{
		(*rearrangerAtomQueue)[0][i] = (*rearrangerAtomQueue)[0][i] * (gmoog->initialPositions)[i];
	}
}

void atomCruncher::scrunchX(moveSequence& moveseq, bool firstStep) {
	UINT8 iy = 0;
	UINT8 nGapY = (gmoog->initialPositionsY.size() - gmoog->nTweezerY) / 2;

	for (auto const& channelBoolY : gmoog->initialPositionsY)
	{
		if ((firstStep && channelBoolY) || (!firstStep && iy >= nGapY && iy < gmoog->nTweezerY + nGapY)) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			UINT8 ix = 0;
			for (auto const& channelBoolX : gmoog->initialPositionsX)
			{
				if ((firstStep && channelBoolX && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
					|| (!firstStep && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy]))
				{
					single.startAOX.push_back(ix); //Place tweezers on all atoms in row
					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
				}
				ix++;
			}
			single.startAOY.push_back(iy); //Single tone in y
			single.endAOY.push_back(iy); //y does not move
			//if (single.startAOX.empty()) {
			//	single.startAOX.push_back(0); //Just for demonstration purposes, to avoid skipping when no atoms.
			//}
			moveseq.moves.push_back(single);

			UINT8 nAtomsInRow = moveseq.moves.back().nx();
			UINT8 nGap = (gmoog->initialPositionsX.size() - nAtomsInRow) / 2;
			for (UINT8 ix2 = 0; ix2 < nAtomsInRow; ix2++)
			{
				moveseq.moves.back().endAOX.push_back(nGap + ix2); //Bunch up tweezers in center of row
				(*rearrangerAtomQueue)[0][nGap + ix2 + (gmoog->initialPositionsX.size())*iy] = 1; //place atom in dropoff location
			};
		}
		iy++;
	}
}

void atomCruncher::scrunchY(moveSequence& moveseq, bool firstStep) {
	UINT8 ix = 0;
	int nGapX = (gmoog->initialPositionsX.size() - gmoog->nTweezerX) / 2;

	for (auto const& channelBoolX : gmoog->initialPositionsX)
	{
		if ((firstStep && channelBoolX) || (!firstStep && ix >= nGapX && ix < gmoog->nTweezerX + nGapX))
		{
			moveSingle single;
			UINT8 iy = 0;
			for (auto const& channelBoolY : gmoog->initialPositionsY)
			{
				if ((firstStep && channelBoolY && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
					|| (!firstStep && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy]))
				{
					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
				}
				iy++;
			}
			single.startAOX.push_back(ix); //Single tone in x
			single.endAOX.push_back(ix); //x does not move
			//if (single.startAOY.empty()) {
			//	single.startAOY.push_back(0); //Just for demonstration purposes, to avoid skipping when no atoms.
			//}
			moveseq.moves.push_back(single);

			int nAtomsInRow = moveseq.moves.back().ny();
			int nGap = (gmoog->initialPositionsY.size() - nAtomsInRow) / 2;
			for (UINT8 iy2 = 0; iy2 < nAtomsInRow; iy2++)
			{
				moveseq.moves.back().endAOY.push_back(nGap + iy2); //Bunch up tweezers in center of row
				(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*(nGap + iy2)] = 1; //place atom in dropoff location
			};
		}
		ix++;
	}
}

moveSequence atomCruncher::getRearrangeMoves(std::string rearrangeType) {
	// atomCruncher contains gmoog and images. Generate moves based on these.
	//input->gmoog->initialPositionsX;
	//input->gmoog->initialPositionsY;
	//input->rearrangerAtomQueue[0];
	//input->gmoog->targetPositions;
	//input->gmoog->targetNumber;
	//input->nAtom;

	moveSequence moveseq;
	filterAtomQueue();

	if (rearrangeType == "scrunchx")
	{
		scrunchX(moveseq, true);
	}
	else if (rearrangeType == "scrunchy")
	{
		scrunchY(moveseq, true);
	}
	else if (rearrangeType == "scrunchxy")
	{
		scrunchX(moveseq, true);
		scrunchY(moveseq, false);
	}
	else if (rearrangeType == "scrunchyx")
	{
		scrunchY(moveseq, true);
		scrunchX(moveseq, false);
	}
	else
	{
		thrower("Invalid rearrangement mode.");
	}
	
	return moveseq;
}