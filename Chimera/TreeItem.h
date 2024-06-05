#pragma once
// A template class for items used in a Tree data structure
// The initial purpose of this class is for sorting out the 'repeat' in the master script
//#include <GeneralUtilityFunctions/Thrower.h>
#include <vector>
#include <algorithm>
#include <utility>

template <class type>
class TreeItem 
{
public:
    // the type must have a default construcor or a copy/assigment constructor
    explicit TreeItem(const type& data, TreeItem* parentItem = nullptr);
    TreeItem(); 
    ~TreeItem();

    void appendChild(TreeItem* child);
    void removeChild(TreeItem* child); // this will not release the child, but only take it out of the m_childItems.
    TreeItem* child(int row);
    std::vector<TreeItem*> getChildren();
    std::vector<TreeItem*> getAllDescendant();
    int childCount() const;
    type getData() const;
    type& data();
    
    TreeItem* parentItem();
    TreeItem* rootItem();

    int index() const; // index of this item among its siblings
    int level() const; // level of this item in the tree structure, equvi. to number of levels of parents this item has.
    std::pair<int, int> itemID() const; // return (level, index), root will return (0,0)
private:
    std::vector<TreeItem*> m_childItems;
    type m_itemData;
    TreeItem* m_parentItem;

};

template <class type>
TreeItem<type>::TreeItem(const type& data, TreeItem* parent)
    : m_itemData(data), m_parentItem(parent) {}

template<class type>
TreeItem<type>::TreeItem() 
    : m_itemData(), m_parentItem(nullptr) {}

template <class type>
TreeItem<type>::TreeItem::~TreeItem()
{
    for (auto* child : m_childItems) {
        delete child;
    }
}

template <class type>
void TreeItem<type>::appendChild(TreeItem* item)
{
    if (item == nullptr) {
        thrower("Appending a non-initialized TreeItem pointer to the children. This is not allowed.");
    }
    if (item->m_parentItem != nullptr) {
        item->m_parentItem->removeChild(item);
    }
    item->m_parentItem = this;
    m_childItems.push_back(item);
}

template<class type>
inline void TreeItem<type>::removeChild(TreeItem* child)
{
    auto& allChilds = m_childItems;
    auto itr = std::find(allChilds.cbegin(), allChilds.cend(), const_cast<TreeItem*>(this));
    if (itr == allChilds.cend()) {
        thrower("Can not find the the target child in the child list.");
    }
    m_childItems.erase(itr);
}

template <class type>
TreeItem<type>* TreeItem<type>::TreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

template<class type>
std::vector<TreeItem<type>*> TreeItem<type>::getChildren()
{
    return m_childItems;
}

template<class type>
std::vector<TreeItem<type>*> TreeItem<type>::getAllDescendant()
{
    auto allDes = std::vector<TreeItem*>();
    std::function<void(TreeItem*)> traverseDepthFirst =
        [&, this](TreeItem* item) {
        if (item == nullptr) {
            return;
        }
        if (item != this) {
            allDes.push_back(item);
        }
        for (auto* it : item->getChildren()) {
            traverseDepthFirst(it);
        }
    };
    traverseDepthFirst(this);
    return allDes;
}

template <class type>
int TreeItem<type>::childCount() const
{
    return m_childItems.size();
}

template <class type>
type TreeItem<type>::getData() const
{
    return m_itemData;
}

template<class type>
type& TreeItem<type>::data()
{
    return m_itemData;
}

template <class type>
TreeItem<type>* TreeItem<type>::parentItem()
{
    return m_parentItem;
}

template<class type>
TreeItem<type>* TreeItem<type>::rootItem()
{
    TreeItem* parent = m_parentItem;
    while (parent != nullptr) {
        parent = m_parentItem->m_parentItem;
    }
    return parent;
}

template <class type>
int TreeItem<type>::index() const
{
    int row = 0;
    if (m_parentItem) {
        auto& allChilds = m_parentItem->m_childItems;
        auto itr = std::find(allChilds.cbegin(), allChilds.cend(), const_cast<TreeItem*>(this));
        if (itr != allChilds.cend()) {
            row = std::distance(allChilds.cbegin(), itr);
        }
        else {
            thrower("Can not find the item iself in its parent! A low level bug.");
        }
    }
    return row;
}

template<class type>
int TreeItem<type>::level() const
{
    int level = 0; // root has level = 0
    const TreeItem* node = this;
    while (node->m_parentItem != nullptr) {
        ++level;
        node = node->m_parentItem;
    }
    return level;
}

template<class type>
std::pair<int, int> TreeItem<type>::itemID() const
{
    return std::pair<int, int>(level(), index());
}
