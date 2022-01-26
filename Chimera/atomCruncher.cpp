#include "stdafx.h"
#include "atomCruncher.h"

moveSequence atomCruncher::getRearrangeMoves(std::string rearrangeType) {
	// atomCruncher contains gmoog and images. Generate moves based on these.
	//input->gmoog->initialPositionsX;
	//input->gmoog->initialPositionsY;
	//input->rearrangerAtomQueue[0];
	//input->gmoog->targetPositions;
	//input->gmoog->targetNumber;
	//input->nAtom;

	moveSequence moveseq;

	if (rearrangeType == "scrunchx")
	{
		UINT8 iy = 0;
		for (auto const& channelBoolY : gmoog->initialPositionsY)
		{
			if (channelBoolY)
			{
				moveSingle single;
				UINT8 ix = 0;
				for (auto const& channelBoolX : gmoog->initialPositionsX)
				{
					if (channelBoolX && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
					{
						single.startAOX.push_back(ix); //Place tweezers on all atoms in row
					}
					ix++;
				}
				single.startAOY.push_back(iy); //Single tone in y
				single.endAOY.push_back(iy); //y does not move
				if (single.startAOX.empty()) {
					single.startAOX.push_back(0); //Just for demonstration purposes, to avoid skipping when no atoms.
				}
				moveseq.moves.push_back(single);

				int nAtomsInRow = moveseq.moves.back().nx();
				int nGap = (gmoog->initialPositionsX.size() - nAtomsInRow) / 2;
				for (UINT8 ix2 = 0; ix2 < nAtomsInRow; ix2++)
				{
					moveseq.moves.back().endAOX.push_back(nGap + ix2); //Bunch up tweezers in center of row
				};
			}
			iy++;
		}
	}
	else if (rearrangeType == "scrunchy")
	{
		UINT8 ix = 0;
		for (auto const& channelBoolX : gmoog->initialPositionsX)
		{
			if (channelBoolX)
			{
				moveSingle single;
				UINT8 iy = 0;
				for (auto const& channelBoolY : gmoog->initialPositionsY)
				{
					if (channelBoolY && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
					{
						single.startAOY.push_back(iy); //Place tweezers on all atoms in row
					}
					iy++;
				}
				single.startAOX.push_back(ix); //Single tone in x
				single.endAOX.push_back(ix); //x does not move
				if (single.startAOY.empty()) {
					single.startAOY.push_back(0); //Just for demonstration purposes, to avoid skipping when no atoms.
				}
				moveseq.moves.push_back(single);

				int nAtomsInRow = moveseq.moves.back().ny();
				int nGap = (gmoog->initialPositionsY.size() - nAtomsInRow) / 2;
				for (UINT8 iy2 = 0; iy2 < nAtomsInRow; iy2++)
				{
					moveseq.moves.back().endAOY.push_back(nGap + iy2); //Bunch up tweezers in center of row
				};
			}
			ix++;
		}
	}
	else
	{
		thrower("Invalid rearrangement mode.");
	}
	
	return moveseq;
}