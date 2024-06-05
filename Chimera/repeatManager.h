#pragma once
#include <memory>
#include "TreeItem.h"
//#include <GeneralObjects/commonTypes.h>
#include "Expression.h"
//#include <ParameterSystemStructures.h>

struct repeatInfoId
{
	static const std::pair<int, int> root;
	unsigned repeatIdentifier = 0; // identifier in the repeatInfo, which uniquely labels this repeat.
	std::pair<int, int> repeatTreeMap = { 0,0 }; // record the index of the repeatInfo in the tree. (level, index), see TreeItem.
	bool placeholder = false; // if true, meaning the command that contains this repeatInfoId is only for advancing time for that device class. And this command is inserted by Chimera automatically
};

struct repeatInfo
{
	const unsigned identifier;
	Expression repeatNum;
	timeType repeatAddedTime;
	// evaluated for all variations
	std::vector<unsigned> repeatNums;
	std::vector<double> repeatAddedTimes;

	std::vector<double> repeatAddedTimesSave;

	repeatInfo(unsigned identifier);
	void calculateVariations(std::vector<variableType>& vars, UINT variations);
	void saveCalculationResults();
	void loadCalculationResults();
};

class repeatManager
{
public:
	repeatManager(const repeatManager&) = delete;
	repeatManager& operator= (const repeatManager&) = delete;
	repeatManager();
	TreeItem<repeatInfo>* getRepeatRoot();
	repeatInfoId getCurrentActiveID();
	bool isRepeating();
	bool repeatHappend();
	TreeItem<repeatInfo>* addNewRepeat();
	void fininshCurrentRepeat();
	TreeItem<repeatInfo>* getCurrentActiveItem();
	void calculateVariations(std::vector<variableType>& vars, UINT variations);
	void saveCalculationResults();
	void loadCalculationResults();
private:
	// used to label each tree item with this unique identifier, used in getCurrentActiveID().
	unsigned repeatInfoIncrement; 
	std::unique_ptr<TreeItem<repeatInfo>> repeatRoot;
	std::vector<TreeItem<repeatInfo>*> currentActiveItems;
};