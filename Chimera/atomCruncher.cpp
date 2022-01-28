#include "stdafx.h"
#include "atomCruncher.h"

void atomCruncher::filterAtomQueue(void) {
	//exclude atoms that were not on a target site.
	for (size_t i = 0; i < gmoog->initialPositions.size(); i++)
	{
		(*rearrangerAtomQueue)[0][i] = (*rearrangerAtomQueue)[0][i] * (gmoog->initialPositions)[i];
	}
}

void atomCruncher::scrunchX(moveSequence& moveseq) {
	size_t iy = 0;
	for (auto const& channelBoolY : gmoog->initialPositionsY)
	{
		if (channelBoolY) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			size_t ix = 0;
			for (auto const& channelBoolX : gmoog->initialPositionsX)
			{
				if (channelBoolX && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
				{
					single.startAOX.push_back(ix); //Place tweezers on all atoms in row
					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
				}
				ix++;
			}
			single.startAOY.push_back(iy); //Single tone in y
			single.endAOY.push_back(iy); //y does not move
			moveseq.moves.push_back(single);

			size_t nAtomsInRow = moveseq.moves.back().nx();
			size_t nGap = 0;
			//(gmoog->initialPositionsX.size() - 2 * nAtomsInRow) / 2;
			for (size_t ix2 = 0; ix2 < nAtomsInRow; ix2++)
			{
				moveseq.moves.back().endAOX.push_back(nGap + (gmoog->scrunchSpacing) * ix2); //Bunch up tweezers in center of row
				(*rearrangerAtomQueue)[0][nGap + (gmoog->scrunchSpacing) * ix2 + (gmoog->initialPositionsX.size())*iy] = 1; //place atom in dropoff location
			};
		}
		iy++;
	}
	//for (auto const& channelBoolY : gmoog->initialPositionsY)
	//{
	//	if ((firstStep && channelBoolY) || (!firstStep && iy >= nGapY && iy < gmoog->nTweezerY + nGapY)) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
	//	{
	//		moveSingle single;
	//		size_t ix = 0;
	//		for (auto const& channelBoolX : gmoog->initialPositionsX)
	//		{
	//			if ((firstStep && channelBoolX && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
	//				|| (!firstStep && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy]))
	//			{
	//				single.startAOX.push_back(ix); //Place tweezers on all atoms in row
	//				(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
	//			}
	//			ix++;
	//		}
	//		single.startAOY.push_back(iy); //Single tone in y
	//		single.endAOY.push_back(iy); //y does not move
	//		//if (single.startAOX.empty()) {
	//		//	single.startAOX.push_back(0); //Just for demonstration purposes, to avoid skipping when no atoms.
	//		//}
	//		moveseq.moves.push_back(single);

	//		size_t nAtomsInRow = moveseq.moves.back().nx();
	//		size_t nGap = (gmoog->initialPositionsX.size() - 2*nAtomsInRow) / 2;
	//		for (size_t ix2 = 0; ix2 < nAtomsInRow; ix2++)
	//		{
	//			moveseq.moves.back().endAOX.push_back(nGap + 2*ix2); //Bunch up tweezers in center of row
	//			(*rearrangerAtomQueue)[0][nGap + 2*ix2 + (gmoog->initialPositionsX.size())*iy] = 1; //place atom in dropoff location
	//		};
	//	}
	//	iy++;
	//}
	//if (firstStep)
	//{
	//	size_t iy = 0;
	//	for (auto const& channelBoolY : gmoog->initialPositionsY)
	//	{
	//		if (channelBoolY) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
	//		{
	//			moveSingle single;
	//			size_t ix = 0;
	//			for (auto const& channelBoolX : gmoog->initialPositionsX)
	//			{
	//				if (channelBoolX && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
	//				{
	//					single.startAOX.push_back(ix); //Place tweezers on all atoms in row
	//					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
	//				}
	//				ix++;
	//			}
	//			single.startAOY.push_back(iy); //Single tone in y
	//			single.endAOY.push_back(iy); //y does not move
	//			moveseq.moves.push_back(single);

	//			size_t nAtomsInRow = moveseq.moves.back().nx();
	//			size_t nGap = (gmoog->initialPositionsX.size() - 2 * nAtomsInRow) / 2;
	//			for (size_t ix2 = 0; ix2 < nAtomsInRow; ix2++)
	//			{
	//				moveseq.moves.back().endAOX.push_back(nGap + 2 * ix2); //Bunch up tweezers in center of row
	//				(*rearrangerAtomQueue)[0][nGap + 2 * ix2 + (gmoog->initialPositionsX.size())*iy] = 1; //place atom in dropoff location
	//			};
	//		}
	//		iy++;
	//	}
	//}
	//else if (!firstStep)
	//{
	//	size_t iy = 0;
	//	size_t nGapY = (gmoog->initialPositionsY.size() - 2 * (gmoog->nTweezerY)) / 2;
	//	for (auto const& channelBoolY : gmoog->initialPositionsY)
	//	{
	//		if ( iy >= nGapY && iy < 2*(gmoog->nTweezerY) + nGapY)// If not first step, choose the rows that atoms were scrunched to.
	//		{
	//			moveSingle single;
	//			size_t ix = 0;
	//			for (auto const& channelBoolX : gmoog->initialPositionsX)
	//			{
	//				if (channelBoolX && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
	//				{
	//					single.startAOX.push_back(ix); //Place tweezers on all atoms in row
	//					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
	//				}
	//				ix++;
	//			}
	//			single.startAOY.push_back(iy); //Single tone in y
	//			single.endAOY.push_back(iy); //y does not move
	//			//if (single.startAOX.empty()) {
	//			//	single.startAOX.push_back(0); //Just for demonstration purposes, to avoid skipping when no atoms.
	//			//}
	//			moveseq.moves.push_back(single);

	//			size_t nAtomsInRow = moveseq.moves.back().nx();
	//			size_t nGap = (gmoog->initialPositionsX.size() - 2 * nAtomsInRow) / 2;
	//			for (size_t ix2 = 0; ix2 < nAtomsInRow; ix2++)
	//			{
	//				moveseq.moves.back().endAOX.push_back(nGap + 2 * ix2); //Bunch up tweezers in center of row
	//				(*rearrangerAtomQueue)[0][nGap + 2 * ix2 + (gmoog->initialPositionsX.size())*iy] = 1; //place atom in dropoff location
	//			};
	//		}
	//		iy += 2;
	//	}
	//}
}

void atomCruncher::scrunchY(moveSequence& moveseq) {
	size_t ix = 0;
	for (auto const& channelBoolX : gmoog->initialPositionsX)
	{
		if (channelBoolX) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			size_t iy = 0;
			for (auto const& channelBoolY : gmoog->initialPositionsY)
			{
				if (channelBoolY && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
				{
					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
				}
				iy++;
			}
			single.startAOX.push_back(ix); //Single tone in x
			single.endAOX.push_back(ix); //x does not move
			moveseq.moves.push_back(single);

			size_t nAtomsInRow = moveseq.moves.back().ny();
			size_t nGap = 0;
			//(gmoog->initialPositionsY.size() - 2 * nAtomsInRow) / 2;
			for (size_t iy2 = 0; iy2 < nAtomsInRow; iy2++)
			{
				moveseq.moves.back().endAOY.push_back(nGap + (gmoog->scrunchSpacing) * iy2); //Bunch up tweezers in center of row
				(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*(nGap + iy2*(gmoog->scrunchSpacing))] = 1; //place atom in dropoff location
			};
		}
		ix++;
	}
	//if (firstStep)
	//{
	//	size_t ix = 0;
	//	for (auto const& channelBoolX : gmoog->initialPositionsX)
	//	{
	//		if (channelBoolX) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
	//		{
	//			moveSingle single;
	//			size_t iy = 0;
	//			for (auto const& channelBoolY : gmoog->initialPositionsY)
	//			{
	//				if (channelBoolY && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
	//				{
	//					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
	//					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
	//				}
	//				iy++;
	//			}
	//			single.startAOX.push_back(ix); //Single tone in x
	//			single.endAOX.push_back(ix); //x does not move
	//			moveseq.moves.push_back(single);

	//			size_t nAtomsInRow = moveseq.moves.back().ny();
	//			size_t nGap = (gmoog->initialPositionsY.size() - 2 * nAtomsInRow) / 2;
	//			for (size_t iy2 = 0; iy2 < nAtomsInRow; iy2++)
	//			{
	//				moveseq.moves.back().endAOY.push_back(nGap + 2 * iy2); //Bunch up tweezers in center of row
	//				(*rearrangerAtomQueue)[0][nGap + ix + (gmoog->initialPositionsX.size())*iy2*2] = 1; //place atom in dropoff location
	//			};
	//		}
	//		ix++;
	//	}
	//}
	//else if (!firstStep)
	//{
	//	size_t ix = 0;
	//	size_t nGapX = (gmoog->initialPositionsX.size() - 2 * (gmoog->nTweezerX)) / 2;
	//	for (auto const& channelBoolX : gmoog->initialPositionsX)
	//	{
	//		if (ix >= nGapX && ix < 2 * (gmoog->nTweezerX) + nGapX)// If not first step, choose the rows that atoms were scrunched to.
	//		{
	//			moveSingle single;
	//			size_t iy = 0;
	//			for (auto const& channelBoolY : gmoog->initialPositionsY)
	//			{
	//				if (channelBoolY && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
	//				{
	//					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
	//					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
	//				}
	//				iy++;
	//			}
	//			single.startAOX.push_back(ix); //Single tone in x
	//			single.endAOX.push_back(ix); //x does not move
	//			moveseq.moves.push_back(single);

	//			size_t nAtomsInRow = moveseq.moves.back().ny();
	//			size_t nGap = (gmoog->initialPositionsY.size() - 2 * nAtomsInRow) / 2;
	//			for (size_t iy2 = 0; iy2 < nAtomsInRow; iy2++)
	//			{
	//				moveseq.moves.back().endAOY.push_back(nGap + 2 * iy2); //Bunch up tweezers in center of row
	//				(*rearrangerAtomQueue)[0][nGap + ix + (gmoog->initialPositionsX.size())*iy2*2] = 1; //place atom in dropoff location
	//			};
	//		}
	//		ix += 2;
	//	}
	//}
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
		scrunchX(moveseq);
	}
	else if (rearrangeType == "scrunchy")
	{
		scrunchY(moveseq);
	}
	//else if (rearrangeType == "scrunchxy")
	//{
	//	scrunchX(moveseq);
	//	size_t ix = 0;
	//	size_t nGapX = (gmoog->initialPositionsX.size() - 2 * (gmoog->nTweezerX)) / 2;
	//	for (auto const& channelBoolX : gmoog->initialPositionsX)
	//	{
	//		if (ix >= nGapX && ix < 2 * (gmoog->nTweezerX) + nGapX)// If not first step, choose the rows that atoms were scrunched to.
	//		{
	//			moveSingle single;
	//			size_t iy = 0;
	//			for (auto const& channelBoolY : gmoog->initialPositionsY)
	//			{
	//				if (channelBoolY && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
	//				{
	//					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
	//					(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
	//				}
	//				iy++;
	//			}
	//			single.startAOX.push_back(ix); //Single tone in x
	//			single.endAOX.push_back(ix); //x does not move
	//			moveseq.moves.push_back(single);

	//			size_t nAtomsInRow = moveseq.moves.back().ny();
	//			size_t nGap = (gmoog->initialPositionsY.size() - 2 * nAtomsInRow) / 2;
	//			for (size_t iy2 = 0; iy2 < nAtomsInRow; iy2++)
	//			{
	//				moveseq.moves.back().endAOY.push_back(nGap + 2 * iy2); //Bunch up tweezers in center of row
	//				(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*(nGap+iy2*2)] = 1; //place atom in dropoff location
	//			};
	//		}
	//		ix += 2;
	//	}
	//}
	else if (rearrangeType == "scrunchyx")
	{
		scrunchY(moveseq);

		size_t nAtomsInRow = gmoog->nTweezerY;
		size_t nGap = 0;
		//(gmoog->initialPositionsY.size() - 2 * nAtomsInRow) / 2;
		for (size_t iy = 0; iy < gmoog->initialPositionsY.size(); iy++)
		{
			gmoog->initialPositionsY[iy] = false;
			if (iy >= nGap && iy% (gmoog->scrunchSpacing) == 0 && iy < nGap + (gmoog->scrunchSpacing) * nAtomsInRow)
			{
				gmoog->initialPositionsY[iy] = true;
			}
		}

		scrunchX(moveseq);

		//size_t iy = 0;
		//size_t nGapY = (gmoog->initialPositionsY.size() - 2 * (gmoog->nTweezerY)) / 2;
		//for (auto const& channelBoolY : gmoog->initialPositionsY)
		//{
		//	if ( iy >= nGapY && iy < 2*(gmoog->nTweezerY) + nGapY)// If not first step, choose the rows that atoms were scrunched to.
		//	{
		//		moveSingle single;
		//		size_t ix = 0;
		//		for (auto const& channelBoolX : gmoog->initialPositionsX)
		//		{
		//			if (channelBoolX && (*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy])
		//			{
		//				single.startAOX.push_back(ix); //Place tweezers on all atoms in row
		//				(*rearrangerAtomQueue)[0][ix + (gmoog->initialPositionsX.size())*iy] = 0; //remove atom from pickup location
		//			}
		//			ix++;
		//		}
		//		single.startAOY.push_back(iy); //Single tone in y
		//		single.endAOY.push_back(iy); //y does not move
		//		moveseq.moves.push_back(single);

		//		size_t nAtomsInRow = moveseq.moves.back().nx();
		//		size_t nGap = (gmoog->initialPositionsX.size() - 2 * nAtomsInRow) / 2;
		//		for (size_t ix2 = 0; ix2 < nAtomsInRow; ix2++)
		//		{
		//			moveseq.moves.back().endAOX.push_back(nGap + 2 * ix2); //Bunch up tweezers in center of row
		//			(*rearrangerAtomQueue)[0][nGap + 2 * ix2 + (gmoog->initialPositionsX.size())*iy] = 1; //place atom in dropoff location
		//		};
		//	}
		//	iy += 2;
		//}
	}
	else
	{
		thrower("Invalid rearrangement mode.");
	}
	
	return moveseq;
}