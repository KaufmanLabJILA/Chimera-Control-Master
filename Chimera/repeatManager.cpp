#include "stdafx.h"
#include "repeatManager.h"
#include <cmath>
#include <functional>

const std::pair<int, int> repeatInfoId::root = { 0,0 };

repeatInfo::repeatInfo(unsigned identifier)
    : identifier(identifier)
{
}

void repeatInfo::calculateVariations(std::vector<variableType>& params, UINT variations)
{
    repeatNums.resize(variations);
    repeatAddedTimes.resize(variations);
    for (auto variationNum : range(variations)) {
        double repeatNumDouble = repeatNum.evaluate(params, variationNum);
        unsigned repeatNumRound = std::round(repeatNumDouble);
		if (repeatNumRound < 0) {
			thrower("Repeat number: " + repeatNum.expressionStr + " evaluates to: " + str(repeatNumDouble, 5)
				+ " in variation " + str(variationNum) + ", is negative.");
		}
		// Taking out this block so default behavior is just rounding to nearest whole number.
        //if (std::abs(repeatNumDouble - repeatNumRound) > 1e-5) {
        //    thrower("Repeat number: " + repeatNum.expressionStr + " evaluates to: " + str(repeatNumDouble, 5)
        //       + " in variation " + str(variationNum) + ", is more than 1e-5 away from its closest integer " + str(repeatNumRound, 1)
        //        + ". This is not normal. To avoid unexpected rounding error, please make the repeat number an integer.");
        //}
        repeatNums[variationNum] = static_cast<unsigned>(repeatNumRound);

        double variableTime = 0;
        // add together current values for all variable times.
        if (repeatAddedTime.first.size() != 0) {
            for (auto varTime : repeatAddedTime.first) {
                variableTime += varTime.evaluate(params, variationNum);
            }
        }
        repeatAddedTimes[variationNum] = variableTime + repeatAddedTime.second;
    }
}

void repeatInfo::saveCalculationResults()
{
    repeatAddedTimesSave = repeatAddedTimes;
}

void repeatInfo::loadCalculationResults()
{
    repeatAddedTimes = repeatAddedTimesSave;
}


repeatManager::repeatManager()
    : repeatInfoIncrement(0)
    , repeatRoot(std::make_unique<TreeItem<repeatInfo>>(repeatInfo(repeatInfoIncrement)))
{
}

TreeItem<repeatInfo>* repeatManager::getRepeatRoot()
{
    return repeatRoot.get();
}

repeatInfoId repeatManager::getCurrentActiveID()
{
    repeatInfoId id;
    if (!currentActiveItems.empty()) {
        auto* current = currentActiveItems.back();
        id.repeatIdentifier = current->data().identifier;
        id.repeatTreeMap = current->itemID();
    }
    return id;
}

bool repeatManager::isRepeating()
{
    return !currentActiveItems.empty();
}

bool repeatManager::repeatHappend()
{
    return repeatInfoIncrement > 0;
}

TreeItem<repeatInfo>* repeatManager::addNewRepeat()
{
    repeatInfoIncrement++;
    auto* child = new TreeItem<repeatInfo>(repeatInfo(repeatInfoIncrement));
    if (!isRepeating()) {
        // meaning the start of the repeat that is not inside of any other repeat. So add to root's children.
        repeatRoot->appendChild(child);
    }
    else {
        // there is another (maybe several) repeat going on already. Add to this already existing repeat.
        currentActiveItems.back()->appendChild(child);
    }
    currentActiveItems.push_back(child);
    return child;
}

void repeatManager::fininshCurrentRepeat()
{
    if (currentActiveItems.empty()) {
        thrower("ERROR! Tried to end repeat structure in master script, but you weren't repeating!");
    }
    currentActiveItems.pop_back();
}

TreeItem<repeatInfo>* repeatManager::getCurrentActiveItem()
{
    if (currentActiveItems.empty()) {
        thrower("There is no currently actively repeating items!");
    }
    return currentActiveItems.back();
}


void repeatManager::calculateVariations(std::vector<variableType>& params, UINT variations)
{
    auto allDes = repeatRoot->getAllDescendant();
    for (auto* item : allDes) {
        item->data().calculateVariations(params, variations);
        item->data().saveCalculationResults();
    }
}

void repeatManager::saveCalculationResults()
{
    auto allDes = repeatRoot->getAllDescendant();
    for (auto* item : allDes) {
        item->data().saveCalculationResults();
    }
}

void repeatManager::loadCalculationResults()
{
    auto allDes = repeatRoot->getAllDescendant();
    for (auto* item : allDes) {
        item->data().loadCalculationResults();
    }
}
