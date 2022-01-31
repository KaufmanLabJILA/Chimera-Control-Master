#include "stdafx.h"
#include "atomCruncher.h"

void atomCruncher::filterAtomQueue(void) {
	//exclude atoms that were not on a target site.
	for (size_t i = 0; i < positions.size(); i++)
	{
		(*rearrangerAtomQueue)[0][i] = (*rearrangerAtomQueue)[0][i] * (positions)[i];
	}
}

void atomCruncher::scrunchX(moveSequence& moveseq) {
	size_t iy = 0;
	for (auto const& channelBoolY : positionsY)
	{
		if (channelBoolY) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			size_t ix = 0;
			for (auto const& channelBoolX : positionsX)
			{
				if (channelBoolX && (*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy])
				{
					single.startAOX.push_back(ix); //Place tweezers on all atoms in row
					(*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy] = 0; //remove atom from pickup location
				}
				ix++;
			}
			single.startAOY.push_back(iy); //Single tone in y
			single.endAOY.push_back(iy); //y does not move
			moveseq.moves.push_back(single);

			size_t nAtomsInRow = moveseq.moves.back().nx();
			size_t nGap = 0;
			//(positionsX.size() - 2 * nAtomsInRow) / 2;
			for (size_t ix2 = 0; ix2 < nAtomsInRow; ix2++)
			{
				moveseq.moves.back().endAOX.push_back(nGap + (gmoog->scrunchSpacing) * ix2); //Bunch up tweezers in center of row
				(*rearrangerAtomQueue)[0][nGap + (gmoog->scrunchSpacing) * ix2 + (positionsX.size())*iy] = 1; //place atom in dropoff location
			};
		}
		iy++;
	}
}

void atomCruncher::scrunchY(moveSequence& moveseq) {
	size_t ix = 0;
	for (auto const& channelBoolX : positionsX)
	{
		if (channelBoolX) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			size_t iy = 0;
			for (auto const& channelBoolY : positionsY)
			{
				if (channelBoolY && (*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy])
				{
					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
					(*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy] = 0; //remove atom from pickup location
				}
				iy++;
			}
			single.startAOX.push_back(ix); //Single tone in x
			single.endAOX.push_back(ix); //x does not move
			moveseq.moves.push_back(single);

			size_t nAtomsInRow = moveseq.moves.back().ny();
			size_t nGap = 0;
			//(positionsY.size() - 2 * nAtomsInRow) / 2;
			for (size_t iy2 = 0; iy2 < nAtomsInRow; iy2++)
			{
				moveseq.moves.back().endAOY.push_back(nGap + (gmoog->scrunchSpacing) * iy2); //Bunch up tweezers in center of row
				(*rearrangerAtomQueue)[0][ix + (positionsX.size())*(nGap + iy2*(gmoog->scrunchSpacing))] = 1; //place atom in dropoff location
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
	
	positions = gmoog->initialPositions;
	positionsX = gmoog->initialPositionsX;
	positionsY = gmoog->initialPositionsY;

	filterAtomQueue();

	if (rearrangeType == "scrunchx")
	{
		scrunchX(moveseq);
	}
	else if (rearrangeType == "scrunchy")
	{
		scrunchY(moveseq);
	}
	else if (rearrangeType == "scrunchxy")
	{
		scrunchX(moveseq);

		size_t nAtomsInRow = gmoog->nTweezerX;
		size_t nGap = 0;
		for (size_t ix = 0; ix < positionsX.size(); ix++)
		{
			positionsX[ix] = false;
			if (ix >= nGap && ix % (gmoog->scrunchSpacing) == 0 && ix < nGap + (gmoog->scrunchSpacing) * nAtomsInRow)
			{
				positionsX[ix] = true;
			}
		}

		scrunchY(moveseq);
	}
	else if (rearrangeType == "scrunchyx")
	{
		scrunchY(moveseq);

		size_t nAtomsInRow = gmoog->nTweezerY;
		size_t nGap = 0;
		for (size_t iy = 0; iy < positionsY.size(); iy++)
		{
			positionsY[iy] = false;
			if (iy >= nGap && iy% (gmoog->scrunchSpacing) == 0 && iy < nGap + (gmoog->scrunchSpacing) * nAtomsInRow)
			{
				positionsY[iy] = true;
			}
		}

		scrunchX(moveseq);
	}
	else if (rearrangeType == "tetris")
	{
		UINT wx = (positionsX.size()); //For convenience, could remove.
		UINT wy = (positionsY.size());

		//std::copy(gmoog->targetPositions.begin(), gmoog->targetPositions.end(), targetPositionsTemp);
		targetPositionsTemp = gmoog->targetPositions; //Make a copy of the target positions that can be modified.

		std::vector<UINT8> rearrangerAtomVect(((*rearrangerAtomQueue)[0]).size()); //Dumb hacky fix.
		for (size_t i = 0; i < rearrangerAtomVect.size(); i++)
		{
			rearrangerAtomVect[i] = (*rearrangerAtomQueue)[0][i];
		}

		UINT maxAtomRow = 0;
		UINT numAtomRow = 0;
		for (size_t iy = 0; iy < positionsY.size(); iy++) //Find which row has most atoms.
		{
			numAtomRow = 0;
			for (size_t ix = 0; ix < positionsX.size(); ix++)
			{
				if (rearrangerAtomVect[ix + wx * iy])
				{
					numAtomRow++;
				}
			}
			if (numAtomRow > maxAtomRow)
			{
				maxAtomRow = numAtomRow;
			}
		}

		//scrunchX(moveseq); //scrunch atoms to left side (index 0 side)
		////// This is slightly modified scrunchx to leave sharp edge to draw from.
		size_t iy = 0;
		for (auto const& channelBoolY : positionsY)
		{
			if (channelBoolY) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
			{
				moveSingle single;
				size_t ix = 0;
				for (auto const& channelBoolX : positionsX)
				{
					if (channelBoolX && (*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy])
					{
						single.startAOX.push_back(ix); //Place tweezers on all atoms in row
						(*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy] = 0; //remove atom from pickup location
					}
					ix++;
				}
				single.startAOY.push_back(iy); //Single tone in y
				single.endAOY.push_back(iy); //y does not move
				moveseq.moves.push_back(single);

				size_t nAtomsInRow = moveseq.moves.back().nx();
				size_t nGap = (maxAtomRow - nAtomsInRow) * (gmoog->scrunchSpacing);
				//(positionsX.size() - 2 * nAtomsInRow) / 2;
				for (size_t ix2 = 0; ix2 < nAtomsInRow; ix2++)
				{
					moveseq.moves.back().endAOX.push_back(nGap + (gmoog->scrunchSpacing) * ix2); //Bunch up tweezers in center of row
					(*rearrangerAtomQueue)[0][nGap + (gmoog->scrunchSpacing) * ix2 + (positionsX.size())*iy] = 1; //place atom in dropoff location
				};
			}
			iy++;
		}
		//////
		
		for (size_t i = 0; i < rearrangerAtomVect.size(); i++) //Dumb hacky fix.
		{
			rearrangerAtomVect[i] = (*rearrangerAtomQueue)[0][i];
		}

		//std::vector<bool> targetColumn(positionsY.size(), false);

		int ixSource = (gmoog->scrunchSpacing)*(maxAtomRow-1); //Start taking atoms from right-most scrunched column, iterate towards left.
		int nColumnSource = sourceColumnSum(ixSource, rearrangerAtomVect); //number of atoms remaining in source column
		int ixTarget = wx - 1;
		
		while (ixTarget >= 0 && ixSource >= 0) //iterate through target columns from the opposite side from scrunch. ixTarget must be int so that it can go negative for this condition to fail.
		{
			int iyTarget = 0;
			int iySource = 0;
			moveSingle single;

			while (iyTarget < wy) //iterate through all target positions in column
			{
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) //if atom required at target
				{
					if (nColumnSource<1) //check if a source atom is available in current source column
					{
						ixSource -= (gmoog->scrunchSpacing); //if no atoms, move to the next source column and reset available atoms in column
						nColumnSource = sourceColumnSum(ixSource, rearrangerAtomVect);
						break;
					}
					while (!rearrangerAtomVect[ixSource + wx * iySource]) //find next atom in source column. This should be guaranteed due to counting nColumnSource.
					{
						iySource++;
					}
					single.startAOY.push_back(iySource);
					rearrangerAtomVect[ixSource + wx * iySource] = 0; //erase used source position.
					nColumnSource--; //remove an atom from the source column

					single.endAOY.push_back(iyTarget); //place tweezer at desired final location.
					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				iyTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (single.ny() > 0)
			{
				single.startAOX.push_back(ixSource); //tweezers at desired source column
				single.endAOX.push_back(ixTarget); //tweezers at desired target column

				moveseq.moves.push_back(single); //add the move to the sequence, which either fully populates a target column, or fully depletes a source column.
				//if (true) //TODO: remove
				//{
				//	break;
				//}
			}
			if (iyTarget >= wy) //only iterate to next target column if entire column has been populated/checked.
			{
				ixTarget--;
			}
		}
	}
	else
	{
		thrower("Invalid rearrangement mode.");
	}
	
	return moveseq;
}

int atomCruncher::sourceColumnSum(int iColumn, std::vector<UINT8> rearrangerAtomVect)
{
	int nColumnSource = 0; //number of atoms remaining in source column
	for (size_t iy = 0; iy < (positionsY.size()); iy++) //calling size() shouldn't be slow, but check if this is limiting.
	{
		if (rearrangerAtomVect[iColumn + (positionsX.size()) * iy])
		{
			nColumnSource++;
		}
	}

	return nColumnSource;
}