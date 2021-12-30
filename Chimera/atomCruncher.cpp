#include "stdafx.h"
#include "atomCruncher.h"

moveSequence atomCruncher::getRearrangeMoves(void) {
	// atomCruncher contains gmoog and images. Generate moves based on these.
	//input->gmoog->initialPositionsX;
	//input->gmoog->initialPositionsY;
	//input->rearrangerAtomQueue[0];
	//input->gmoog->targetPositions;
	//input->gmoog->targetNumber;
	//input->nAtom;

	moveSequence moveseq;

	for (size_t iy = 0; iy < gmoog->nTweezerY; iy++)
	{
		moveSingle single;
		for (size_t ix = 0; ix < gmoog->nTweezerX; ix++)
		{
			if ((*rearrangerAtomQueue)[0][ix + (gmoog->nTweezerX)*iy])
			{
				single.startAOX.push_back(ix); //Place tweezers on all atoms in row
				single.startAOY.push_back(iy);
			}
		}
		moveseq.moves.push_back(single);
	}

	for (size_t iy = 0; iy < gmoog->nTweezerY; iy++)
	{
		int nAtomsInRow = moveseq.moves[iy].nx();
		int nGap = (gmoog->nTweezerX - nAtomsInRow) / 2;
		for (size_t ix = 0; ix < nAtomsInRow; ix++)
		{
			moveseq.moves[iy].endAOX.push_back(nGap + ix); //Bunch up tweezers in center of row
			moveseq.moves[iy].endAOY.push_back(iy);
		}
	}

	return moveseq;
}