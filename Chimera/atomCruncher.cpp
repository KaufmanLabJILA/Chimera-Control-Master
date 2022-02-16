#include "stdafx.h"
#include "atomCruncher.h"

void atomCruncher::getTweezerOffset(int* xOffPixels, int* yOffPixels, int* indexSubpixelMask) {
	///modifies input pointers to pass newly computed offset values in units of pixels and subpixel mask offset.

	//First find brightest pixel There is probably a more efficient way of doing this.

	std::vector<int> macroSignals;

	int indPixImg;
	int indPixMask;

	for (int yShift = -2; yShift < 3; yShift++) //TODO: for now range is just hard coded, 
	{
		for (int xShift = -2; xShift < 3; xShift++)
		{
			int integratedSignal = 0;
			//iterate over whole image - just throw out boundary when shifting.
			for (int iy = 0; iy < imageDims.height - abs(yShift); iy++)
			{
				for (int ix = 0; ix < imageDims.width - abs(xShift); ix++)
				{
					//select appropriate pixel in image and mask and take product. Also subtracting background in this step - doing it all at once to avoid saving image twice, but also want to keep raw image for plotting.

					if (xShift >= 0 && yShift >= 0) // Handle indexing for negative mask roll.
					{
						indPixImg = (ix + xShift) + (iy + yShift) * imageDims.width; //positive roll of masks means starting with offset on image indexing.
						indPixMask = (ix) + (iy) * imageDims.width;
					}
					else if (xShift >= 0 && yShift < 0)
					{
						indPixImg = (ix + xShift) + (iy) * imageDims.width;
						indPixMask = (ix) + (iy - yShift) * imageDims.width; //negative roll of masks means starting with offset on mask indexing.
					}
					else if (xShift < 0 && yShift >= 0)
					{
						indPixImg = (ix) + (iy + yShift) * imageDims.width;
						indPixMask = (ix - xShift) + (iy) * imageDims.width;
					}
					else if (xShift < 0 && yShift < 0)
					{
						indPixImg = (ix) + (iy) * imageDims.width;
						indPixMask = (ix - xShift) + (iy - yShift) * imageDims.width;
					}
					//int tmp = (*imageQueue)[0].size();
					integratedSignal += ((*imageQueue)[0][indPixImg] - bgImg[indPixImg]) * (subpixelMaskSingle[indPixMask]);
				}
			}
			macroSignals.push_back(integratedSignal);
		}
	}

	int maxSignalIndex = std::max_element(macroSignals.begin(), macroSignals.end()) - macroSignals.begin(); //find brightest signal
	*xOffPixels = maxSignalIndex % 5 - 2;
	*yOffPixels = maxSignalIndex / 5 - 2;

	std::vector<int> subpixelSignals;
	//Now given maximum pixel, repeat procedure for subpixel masks.
	for (int i = 0; i < nSubpixel; i++)
	{
		int integratedSignal = 0;
		//iterate over whole image - just throw out boundary when shifting.
		for (int iy = 0; iy < imageDims.height - abs(*yOffPixels); iy++)
		{
			for (int ix = 0; ix < imageDims.width - abs(*xOffPixels); ix++)
			{
				//select appropriate pixel in image and mask and take product. Also subtracting background in this step - doing it all at once to avoid saving image twice, but also want to keep raw image for plotting.

				if (*xOffPixels >= 0 && *yOffPixels >= 0) // Handle indexing for negative mask roll.
				{
					indPixImg = (ix + *xOffPixels) + (iy + *yOffPixels) * imageDims.width; //positive roll of masks means starting with offset on image indexing.
					indPixMask = (ix)+(iy)* imageDims.width + i * imageDims.width * imageDims.height;
				}
				else if (*xOffPixels >= 0 && yOffPixels < 0)
				{
					indPixImg = (ix + *xOffPixels) + (iy)* imageDims.width;
					indPixMask = (ix)+(iy - *yOffPixels) * imageDims.width + i * imageDims.width * imageDims.height; //negative roll of masks means starting with offset on mask indexing.
				}
				else if (*xOffPixels < 0 && *yOffPixels >= 0)
				{
					indPixImg = (ix)+(iy + *yOffPixels) * imageDims.width;
					indPixMask = (ix - *xOffPixels) + (iy)* imageDims.width + i * imageDims.width * imageDims.height;
				}
				else if (*xOffPixels < 0 && *yOffPixels < 0)
				{
					indPixImg = (ix)+(iy)* imageDims.width;
					indPixMask = (ix - *xOffPixels) + (iy - *yOffPixels) * imageDims.width + i * imageDims.width * imageDims.height;
				}

				integratedSignal += ((*imageQueue)[0][indPixImg] - bgImg[indPixImg]) * (subpixelMasks[indPixMask]);
			}
		}
		subpixelSignals.push_back(integratedSignal);
	}
	*indexSubpixelMask = std::max_element(subpixelSignals.begin(), subpixelSignals.end()) - subpixelSignals.begin(); //find brightest signal
}

void atomCruncher::filterAtomQueue(void) {
	//exclude atoms that were not on a target site.
	for (int i = 0; i < positions.size(); i++)
	{
		(*rearrangerAtomQueue)[0][i] = (*rearrangerAtomQueue)[0][i] * (positions)[i];
	}
}

void atomCruncher::scrunchX(moveSequence& moveseq, bool centered = false) {
	int iy = 0;
	for (auto const& channelBoolY : positionsY)
	{
		if (channelBoolY) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			int ix = 0;
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

			int nAtomsInRow = moveseq.moves.back().nx();
			int nGap = 0;
			if (centered)
			{
				nGap = positionsX.size()/2 - gmoog->scrunchSpacing * (nAtomsInRow/2);
				nGap = (nGap < 0) ? 0 : nGap;
			}
			for (int ix2 = 0; ix2 < nAtomsInRow; ix2++)
			{
				if (nGap + (gmoog->scrunchSpacing) * ix2 >= positionsX.size())
				{
					moveseq.moves.back().endAOX.push_back(-2); // remove atom from higher frequency side
				}
				else
				{
					moveseq.moves.back().endAOX.push_back(nGap + (gmoog->scrunchSpacing) * ix2);
					(*rearrangerAtomQueue)[0][nGap + (gmoog->scrunchSpacing) * ix2 + (positionsX.size())*iy] = 1; //place atom in dropoff location
				}
			};
		}
		iy++;
	}
}

void atomCruncher::scrunchY(moveSequence& moveseq, bool centered = false) {
	int ix = 0;
	for (auto const& channelBoolX : positionsX)
	{
		if (channelBoolX) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			int iy = 0;
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

			int nAtomsInRow = moveseq.moves.back().ny();
			int nGap = 0;
			if (centered)
			{
				nGap = positionsY.size() / 2 - gmoog->scrunchSpacing * (nAtomsInRow / 2);
				nGap = (nGap < 0) ? 0:nGap;
			}
			for (int iy2 = 0; iy2 < nAtomsInRow; iy2++)
			{
				if (nGap + (gmoog->scrunchSpacing) * iy2 >= positionsY.size())
				{
					moveseq.moves.back().endAOY.push_back(-2); // remove atom from higher frequency side
				}
				else
				{
					moveseq.moves.back().endAOY.push_back(nGap + (gmoog->scrunchSpacing) * iy2); //Bunch up tweezers in center of row
					(*rearrangerAtomQueue)[0][ix + (positionsX.size())*(nGap + iy2 * (gmoog->scrunchSpacing))] = 1; //place atom in dropoff location
				}
			};
		}
		ix++;
	}
}

void atomCruncher::scrunchYFixedLength(moveSequence& moveseq, int nPerColumn, bool centered = false) {
	int ix = 0;
	for (auto const& channelBoolX : positionsX)
	{
		if (channelBoolX) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
		{
			moveSingle single;
			int iy = 0;
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

			int nAtomsInRow = moveseq.moves.back().ny();
			int nGap = 0;
			if (centered)
			{
				nGap = positionsY.size() / 2 - gmoog->scrunchSpacing * (nPerColumn / 2);
				nGap = (nGap < 0) ? 0 : nGap;
			}
			for (int iy2 = 0; iy2 < nAtomsInRow; iy2++)
			{
				if (iy2 < nAtomsInRow - nPerColumn)
				{
					moveseq.moves.back().endAOY.push_back(-1); // remove atom from lower frequency side
				}
				else if (nGap + (gmoog->scrunchSpacing) * (iy2 - (nAtomsInRow - nPerColumn)) >= positionsY.size())
				{
					moveseq.moves.back().endAOY.push_back(-2); // remove atom from higher frequency side
				}
				else
				{
					moveseq.moves.back().endAOY.push_back(nGap + (gmoog->scrunchSpacing) * (iy2 - (nAtomsInRow - nPerColumn))); //Bunch up tweezers, offset to compensate for removed atoms.
					(*rearrangerAtomQueue)[0][ix + (positionsX.size())*(nGap + iy2 * (gmoog->scrunchSpacing))] = 1; //place atom in dropoff location
				}
			};
		}
		ix++;
	}
}

int atomCruncher::equalizeY(moveSequence& moveseq) {
	///Equalize the number of atoms in each column. Returns target atoms/column

	//switch to coordinate representation for convenience.
	std::vector<int> positionCoordinatesX;
	{
		int ix = 0;
		for (auto const& channelBoolX : positionsX)
		{
			if (channelBoolX) {
				positionCoordinatesX.push_back(ix);
			}
			ix++;
		}
	}

	std::vector<int> positionCoordinatesY;
	{
		int iy = 0;
		for (auto const& channelBoolY : positionsY)
		{
			if (channelBoolY) {
				positionCoordinatesY.push_back(iy);
			}
			iy++;
		}
	}

	// Calculate mean atom number per column.
	int nxMean = 0;
	std::vector<int> nxList;
	for (auto const& coordX : positionCoordinatesX)
	{
		int nx = 0;
		for (auto const& coordY : positionCoordinatesY)
		{
			if ((*rearrangerAtomQueue)[0][coordX + (positionsX.size())*coordY])
				nx++;
		}
		nxMean += nx;
		nxList.push_back(nx);
	}
	nxMean /= (gmoog->nTweezerX);
	for (auto& element : nxList) // nxList now contains excess atoms.
		element -= nxMean;

	//iterate through columns, and move to adjacent column as needed.
	int ixTweezer = 0; //tweezer index, can step by multiple lattice sites.
	for (auto const& coordX : positionCoordinatesX)
	{
		if (ixTweezer >= positionCoordinatesX.size() - 1)
		{
			break; //end on second last load column.
		}
		if (nxList[ixTweezer] > 0) // move excess atoms out of column
		{
			moveSingle single;
			for (auto const& coordY : positionCoordinatesY)
			{
				if ((*rearrangerAtomQueue)[0][coordX + (positionsX.size())*coordY]
					&& !((*rearrangerAtomQueue)[0][positionCoordinatesX[ixTweezer + 1] + (positionsX.size())*coordY])) // move only if site in adjacent loaded column is empty.
				{
					single.startAOY.push_back(coordY); //select atoms in column to move
					single.endAOY.push_back(coordY);
					(*rearrangerAtomQueue)[0][coordX + (positionsX.size())*coordY] = 0;
					(*rearrangerAtomQueue)[0][positionCoordinatesX[ixTweezer + 1] + (positionsX.size())*coordY] = 1; //remove atom from pickup location and place in target
					nxList[ixTweezer]--;
					nxList[ixTweezer + 1]++; //keep track of column atom numbers
				}
				if (nxList[ixTweezer] <= 0)
				{
					break; //stop if all excess atoms have been moved.
				}
			}
			single.startAOX.push_back(coordX); //Single tone in x
			single.endAOX.push_back(positionCoordinatesX[ixTweezer + 1]); //x moves to next load column
			moveseq.moves.push_back(single);
		}
		if (nxList[ixTweezer] < 0) // pull missing atoms into column
		{
			moveSingle single;
			for (auto const& coordY : positionCoordinatesY)
			{
				if (!(*rearrangerAtomQueue)[0][coordX + (positionsX.size())*coordY]
					&& ((*rearrangerAtomQueue)[0][positionCoordinatesX[ixTweezer + 1] + (positionsX.size())*coordY])) // move only if site in adjacent loaded column is full.
				{
					single.startAOY.push_back(coordY); //select atoms in column to move
					single.endAOY.push_back(coordY);
					(*rearrangerAtomQueue)[0][coordX + (positionsX.size())*coordY] = 1;
					(*rearrangerAtomQueue)[0][positionCoordinatesX[ixTweezer + 1] + (positionsX.size())*coordY] = 0; //remove atom from pickup location and place in target
					nxList[ixTweezer]++;
					nxList[ixTweezer + 1]--; //keep track of column atom numbers
				}
				if (nxList[ixTweezer] >= 0)
				{
					break; //stop if all excess atoms have been moved.
				}
			}
			single.startAOX.push_back(positionCoordinatesX[ixTweezer + 1]); //x moves from next load column
			single.endAOX.push_back(coordX); //Single tone in x
			moveseq.moves.push_back(single);
		}
		ixTweezer++;
	}
	return nxMean;
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

		////Additional handling to remove every othe atom in first column.

		////Update initial atom locations appropriately.
		//size_t nAtomsInRow = gmoog->nTweezerX;
		//size_t nGap = 0;
		//for (size_t ix = 0; ix < positionsX.size(); ix++)
		//{
		//	positionsX[ix] = false;
		//	if (ix >= nGap && ix % (gmoog->scrunchSpacing) == 0 && ix < nGap + (gmoog->scrunchSpacing) * nAtomsInRow)
		//	{
		//		positionsX[ix] = true;
		//	}
		//}

		//moveSingle single;
		//size_t iy = 0;
		//size_t ix = 0;
		//for (auto const& channelBoolY : positionsY)
		//{
		//	if (channelBoolY && (*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy] && iy % 4 == 0)
		//	{
		//		single.startAOY.push_back(iy); //Place tweezers on all atoms in row
		//		single.endAOY.push_back(iy); //y does not move.
		//		(*rearrangerAtomQueue)[0][ix + (positionsX.size())*iy] = 0; //remove atom from pickup location
		//	}
		//	iy++;
		//}
		//single.startAOX.push_back(ix); //Single tone in x
		//single.endAOX.push_back(-1); //sweep x to remove atoms.
		//moveseq.moves.push_back(single);
	}
	else if (rearrangeType == "scrunchy")
	{
		scrunchY(moveseq);
	}
	else if (rearrangeType == "equalscrunchy")
	{
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn);
	}
	else if (rearrangeType == "centerscrunchx")
	{
		scrunchX(moveseq, true);
	}
	else if (rearrangeType == "centerscrunchy")
	{
		scrunchY(moveseq, true);
	}
	else if (rearrangeType == "scrunchxy")
	{
		scrunchX(moveseq);

		//Update initial atom locations appropriately.
		int nAtomsInRow = gmoog->nTweezerX;
		int nGap = 0;
		for (int ix = 0; ix < positionsX.size(); ix++)
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

		int nAtomsInRow = gmoog->nTweezerY;
		int nGap = 0;
		for (int iy = 0; iy < positionsY.size(); iy++)
		{
			positionsY[iy] = false;
			if (iy >= nGap && iy% (gmoog->scrunchSpacing) == 0 && iy < nGap + (gmoog->scrunchSpacing) * nAtomsInRow)
			{
				positionsY[iy] = true;
			}
		}

		scrunchX(moveseq);
	}
	else if (rearrangeType == "centerscrunchyx") {
		scrunchY(moveseq, true);

		int nAtomsInRow = gmoog->nTweezerY;
		int nGap = positionsY.size() / 2 - gmoog->scrunchSpacing * (nAtomsInRow / 2);
		for (int iy = 0; iy < positionsY.size(); iy++)
		{
			positionsY[iy] = false;
			if (iy >= nGap && iy % (gmoog->scrunchSpacing) == 0 && iy < nGap + (gmoog->scrunchSpacing) * nAtomsInRow)
			{
				positionsY[iy] = true;
			}
		}

		scrunchX(moveseq, true);
	}
	else if (rearrangeType == "equaltetris")
	{
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn);

		UINT wx = (positionsX.size()); //For convenience, could remove.
		UINT wy = (positionsY.size());

		//std::copy(gmoog->targetPositions.begin(), gmoog->targetPositions.end(), targetPositionsTemp);
		targetPositionsTemp = gmoog->targetPositions; //Make a copy of the target positions that can be modified.

		std::vector<UINT8> rearrangerAtomVect(((*rearrangerAtomQueue)[0]).size()); //Dumb hacky fix.
		for (int i = 0; i < rearrangerAtomVect.size(); i++)
		{
			rearrangerAtomVect[i] = (*rearrangerAtomQueue)[0][i];
		}

		//std::vector<bool> targetColumn(positionsY.size(), false);

		int iySource = (gmoog->scrunchSpacing)*(nPerColumn - 1); //Start taking atoms from top-most scrunched row, iterate towards bottom.
		int nRowSource = sourceRowSum(iySource, rearrangerAtomVect); //number of atoms remaining in source row
		int iyTarget = wy - 1;

		while (iyTarget >= 0 && iySource >= 0) //iterate through target rows from the opposite side from scrunch. iyTarget must be int so that it can go negative for this condition to fail.
		{
			int ixTarget = 0;
			int ixSource = 0;
			moveSingle single;

			while (ixTarget < wx) //iterate through all target positions in row
			{
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) //if atom required at target
				{
					if (nRowSource < 1) //check if a source atom is available in current source row
					{
						iySource -= (gmoog->scrunchSpacing); //if no atoms, move to the next source row and reset available atoms in row
						if (iySource >= 0)
						{
							nRowSource = sourceRowSum(iySource, rearrangerAtomVect);
						}
						break;
					}
					while (!rearrangerAtomVect[ixSource + wx * iySource]) //find next atom in source row. This should be guaranteed due to counting nRowSource.
					{
						ixSource++;
					}
					single.startAOX.push_back(ixSource);
					rearrangerAtomVect[ixSource + wx * iySource] = 0; //erase used source position.
					nRowSource--; //remove an atom from the source row

					single.endAOX.push_back(ixTarget); //place tweezer at desired final location.
					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				ixTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (iySource < 0) //TODO: work out why this is getting triggered. Should always have enough atoms.
			{
				break;
			}
			if (single.nx() > 0)
			{
				single.startAOY.push_back(iySource); //tweezers at desired source row
				single.endAOY.push_back(iyTarget); //tweezers at desired target row

				moveseq.moves.push_back(single); //add the move to the sequence, which either fully populates a target row, or fully depletes a source row.
			}
			if (ixTarget >= wx) //only iterate to next target row if entire row has been populated/checked.
			{
				iyTarget--;
			}
		}
	}
	else if (rearrangeType == "equaltetris2")
	{
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn);

		UINT wx = (positionsX.size()); //For convenience, could remove.
		UINT wy = (positionsY.size());

		//std::copy(gmoog->targetPositions.begin(), gmoog->targetPositions.end(), targetPositionsTemp);
		targetPositionsTemp = gmoog->targetPositions; //Make a copy of the target positions that can be modified.

		std::vector<UINT8> rearrangerAtomVect(((*rearrangerAtomQueue)[0]).size()); //Dumb hacky fix.
		for (int i = 0; i < rearrangerAtomVect.size(); i++)
		{
			rearrangerAtomVect[i] = (*rearrangerAtomQueue)[0][i];
		}

		//std::vector<bool> targetColumn(positionsY.size(), false);

		int iySource = (gmoog->scrunchSpacing)*(nPerColumn - 1); //Start taking atoms from top-most scrunched row, iterate towards bottom.
		int nRowSource = sourceRowSum(iySource, rearrangerAtomVect); //number of atoms remaining in source row
		int iyTarget = wy - 1;

		while (iyTarget >= 0 && iySource >= 0) //iterate through target rows from the opposite side from scrunch. iyTarget must be int so that it can go negative for this condition to fail.
		{
			int ixTarget = 0;
			int ixSource = 0;
			moveSingle fromReservoir, scrunch, toTarget;

			while (ixTarget < wx) //iterate through all target positions in row
			{
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) //if atom required at target
				{
					if (nRowSource < 1) //check if a source atom is available in current source row
					{
						iySource -= (gmoog->scrunchSpacing); //if no atoms, move to the next source row and reset available atoms in row
						if (iySource >= 0)
						{
							nRowSource = sourceRowSum(iySource, rearrangerAtomVect);
						}
						break;
					}
					while (!rearrangerAtomVect[ixSource + wx * iySource]) //find next atom in source row. This should be guaranteed due to counting nRowSource.
					{
						ixSource++;
					}
					fromReservoir.startAOX.push_back(ixSource);
					fromReservoir.endAOX.push_back(ixSource);
					scrunch.startAOX.push_back(ixSource);

					rearrangerAtomVect[ixSource + wx * iySource] = 0; //erase used source position.
					nRowSource--; //remove an atom from the source row

					scrunch.endAOX.push_back(ixTarget); //place tweezer at desired final location.
					toTarget.startAOX.push_back(ixTarget);
					toTarget.endAOX.push_back(ixTarget);

					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				ixTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (iySource < 0) //TODO: work out why this is getting triggered. Should always have enough atoms.
			{
				break;
			}
			if (fromReservoir.nx() > 0)
			{
				fromReservoir.startAOY.push_back(iySource); //tweezers at desired source row
				fromReservoir.endAOY.push_back(iySource + (iyTarget- iySource)/2); //tweezers at desired target row

				scrunch.startAOY.push_back(iySource + (iyTarget - iySource) / 2);
				scrunch.endAOY.push_back(iySource + (iyTarget - iySource) / 2);

				toTarget.startAOY.push_back(iySource + (iyTarget - iySource) / 2);
				toTarget.endAOY.push_back(iyTarget);

				moveseq.moves.push_back(fromReservoir);
				moveseq.moves.push_back(scrunch);
				moveseq.moves.push_back(toTarget); //add the move to the sequence, which either fully populates a target row, or fully depletes a source row.
			}
			if (ixTarget >= wx) //only iterate to next target row if entire row has been populated/checked.
			{
				iyTarget--;
			}
		}
	}
	else if (rearrangeType == "tetris")
	{
		UINT wx = (positionsX.size()); //For convenience, could remove.
		UINT wy = (positionsY.size());

		//std::copy(gmoog->targetPositions.begin(), gmoog->targetPositions.end(), targetPositionsTemp);
		targetPositionsTemp = gmoog->targetPositions; //Make a copy of the target positions that can be modified.

		std::vector<UINT8> rearrangerAtomVect(((*rearrangerAtomQueue)[0]).size()); //Dumb hacky fix.
		for (int i = 0; i < rearrangerAtomVect.size(); i++)
		{
			rearrangerAtomVect[i] = (*rearrangerAtomQueue)[0][i];
		}

		UINT maxAtomRow = 0;
		UINT numAtomRow = 0;
		for (int iy = 0; iy < positionsY.size(); iy++) //Find which row has most atoms.
		{
			numAtomRow = 0;
			for (int ix = 0; ix < positionsX.size(); ix++)
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
		int iy = 0;
		for (auto const& channelBoolY : positionsY)
		{
			if (channelBoolY) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
			{
				moveSingle single;
				int ix = 0;
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

				int nAtomsInRow = moveseq.moves.back().nx();
				int nGap = (maxAtomRow - nAtomsInRow) * (gmoog->scrunchSpacing);
				//(positionsX.size() - 2 * nAtomsInRow) / 2;
				for (int ix2 = 0; ix2 < nAtomsInRow; ix2++)
				{
					moveseq.moves.back().endAOX.push_back(nGap + (gmoog->scrunchSpacing) * ix2); //Bunch up tweezers in center of row
					(*rearrangerAtomQueue)[0][nGap + (gmoog->scrunchSpacing) * ix2 + (positionsX.size())*iy] = 1; //place atom in dropoff location
				};
			}
			iy++;
		}
		//////
		
		for (int i = 0; i < rearrangerAtomVect.size(); i++) //Dumb hacky fix.
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
						if (ixSource >=0)
						{
							nColumnSource = sourceColumnSum(ixSource, rearrangerAtomVect);
						}
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
			if (ixSource < 0) //TODO: work out why this is getting triggered. Should always have enough atoms.
			{
				break;
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
	else if (rearrangeType == "tetris2")
	{
		UINT wx = (positionsX.size()); //For convenience, could remove.
		UINT wy = (positionsY.size());

		//std::copy(gmoog->targetPositions.begin(), gmoog->targetPositions.end(), targetPositionsTemp);
		targetPositionsTemp = gmoog->targetPositions; //Make a copy of the target positions that can be modified.

		std::vector<UINT8> rearrangerAtomVect(((*rearrangerAtomQueue)[0]).size()); //Dumb hacky fix.
		for (int i = 0; i < rearrangerAtomVect.size(); i++)
		{
			rearrangerAtomVect[i] = (*rearrangerAtomQueue)[0][i];
		}

		UINT maxAtomRow = 0;
		UINT numAtomRow = 0;
		for (int iy = 0; iy < positionsY.size(); iy++) //Find which row has most atoms.
		{
			numAtomRow = 0;
			for (int ix = 0; ix < positionsX.size(); ix++)
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
		int iy = 0;
		for (auto const& channelBoolY : positionsY)
		{
			if (channelBoolY) //If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
			{
				moveSingle single;
				int ix = 0;
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

				int nAtomsInRow = moveseq.moves.back().nx();
				int nGap = (maxAtomRow - nAtomsInRow) * (gmoog->scrunchSpacing);
				//(positionsX.size() - 2 * nAtomsInRow) / 2;
				for (int ix2 = 0; ix2 < nAtomsInRow; ix2++)
				{
					moveseq.moves.back().endAOX.push_back(nGap + (gmoog->scrunchSpacing) * ix2); //Bunch up tweezers in center of row
					(*rearrangerAtomQueue)[0][nGap + (gmoog->scrunchSpacing) * ix2 + (positionsX.size())*iy] = 1; //place atom in dropoff location
				};
			}
			iy++;
		}
		//////

		for (int i = 0; i < rearrangerAtomVect.size(); i++) //Dumb hacky fix.
		{
			rearrangerAtomVect[i] = (*rearrangerAtomQueue)[0][i];
		}

		//std::vector<bool> targetColumn(positionsY.size(), false);

		int ixSource = (gmoog->scrunchSpacing)*(maxAtomRow - 1); //Start taking atoms from right-most scrunched column, iterate towards left.
		int nColumnSource = sourceColumnSum(ixSource, rearrangerAtomVect); //number of atoms remaining in source column
		int ixTarget = wx - 1;

		while (ixTarget >= 0 && ixSource >= 0) //iterate through target columns from the opposite side from scrunch. ixTarget must be int so that it can go negative for this condition to fail.
		{
			int iyTarget = 0;
			int iySource = 0;
			moveSingle fromReservoir, scrunch, toTarget;

			while (iyTarget < wy) //iterate through all target positions in column
			{
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) //if atom required at target
				{
					if (nColumnSource < 1) //check if a source atom is available in current source column
					{
						ixSource -= (gmoog->scrunchSpacing); //if no atoms, move to the next source column and reset available atoms in column
						if (ixSource >= 0)
						{
							nColumnSource = sourceColumnSum(ixSource, rearrangerAtomVect);
						}
						break;
					}
					while (!rearrangerAtomVect[ixSource + wx * iySource]) //find next atom in source column. This should be guaranteed due to counting nColumnSource.
					{
						iySource++;
					}
					fromReservoir.startAOY.push_back(iySource);
					fromReservoir.endAOY.push_back(iySource);
					scrunch.startAOY.push_back(iySource);

					rearrangerAtomVect[ixSource + wx * iySource] = 0; //erase used source position.
					nColumnSource--; //remove an atom from the source column

					scrunch.endAOY.push_back(iyTarget); //place tweezer at desired final location.
					toTarget.startAOY.push_back(iyTarget);
					toTarget.endAOY.push_back(iyTarget);

					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				iyTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (ixSource < 0) //TODO: work out why this is getting triggered. Should always have enough atoms.
			{
				break;
			}
			if (fromReservoir.ny() > 0)
			{
				fromReservoir.startAOX.push_back(ixSource); //tweezers at desired source column
				fromReservoir.endAOX.push_back(ixSource + (ixTarget - ixSource) / 2); //tweezers half way to target column.
				scrunch.startAOX.push_back(ixSource + (ixTarget - ixSource) / 2); //scrunch occurs at half way point.
				scrunch.endAOX.push_back(ixSource + (ixTarget - ixSource) / 2);
				toTarget.startAOX.push_back(ixSource + (ixTarget - ixSource) / 2);
				toTarget.endAOX.push_back(ixTarget);


				moveseq.moves.push_back(fromReservoir); //add the move to the sequence, which either fully populates a target column, or fully depletes a source column.
				moveseq.moves.push_back(scrunch);
				moveseq.moves.push_back(toTarget);
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

int atomCruncher::sourceRowSum(int iRow, std::vector<UINT8> rearrangerAtomVect)
{
	int nRowSource = 0; //number of atoms remaining in source column
	for (int ix = 0; ix < (positionsX.size()); ix++) //calling size() shouldn't be slow, but check if this is limiting.
	{
		if (rearrangerAtomVect[ix + (positionsX.size()) * iRow])
		{
			nRowSource++;
		}
	}

	return nRowSource;
}

int atomCruncher::sourceColumnSum(int iColumn, std::vector<UINT8> rearrangerAtomVect)
{
	int nColumnSource = 0; //number of atoms remaining in source column
	for (int iy = 0; iy < (positionsY.size()); iy++) //calling size() shouldn't be slow, but check if this is limiting.
	{
		if (rearrangerAtomVect[iColumn + (positionsX.size()) * iy])
		{
			nColumnSource++;
		}
	}

	return nColumnSource;
}