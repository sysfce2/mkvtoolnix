#include "common/common_pch.h"

#include <ebml/EbmlDummy.h>
#include <matroska/KaxSegment.h>

#include "common/kax_element_names.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/info/model.h"
#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/model.h"

using namespace libmatroska;

namespace mtx { namespace gui { namespace Info {

class ModelPrivate {
public:
  std::unique_ptr<Util::KaxInfo> m_info;
  QVector<QStandardItem *> m_treeInsertionPosition;
};

Model::Model(QObject *parent)
  : QStandardItemModel{parent}
  , p_ptr{new ModelPrivate}
{
  setColumnCount(4);
  retranslateUi();
}

Model::~Model() {
}

void
Model::setInfo(std::unique_ptr<Util::KaxInfo> info) {
  p_func()->m_info = std::move(info);
}

Util::KaxInfo &
Model::info() {
  return *p_func()->m_info;
}

EbmlElement *
Model::elementFromIndex(QModelIndex const &idx) {
  if (!idx.isValid())
    return {};

  auto item = itemFromIndex(idx);
  if (item)
    return elementFromItem(*item);

  return {};
}

EbmlElement *
Model::elementFromItem(QStandardItem &item)
  const {
  return reinterpret_cast<EbmlElement *>(item.data(ElementRole).toULongLong());
}

QList<QStandardItem *>
Model::newItems()
  const {
  return { new QStandardItem{}, new QStandardItem{}, new QStandardItem{}, new QStandardItem{} };
}

QList<QStandardItem *>
Model::itemsForRow(QModelIndex const &idx) {
  QList<QStandardItem *> items;

  for (int column = 0, numColumns = columnCount(), row = idx.row(); column < numColumns; ++column)
    items << itemFromIndex(idx.sibling(row, column));

  return items;
}

void
Model::retranslateUi() {
  auto p = p_func();

  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Elements"), Q("elements") },
    { QY("Content"),  Q("content")  },
    { QY("Position"), Q("position") },
    { QY("Size"),     Q("size")     },
  });

  horizontalHeaderItem(2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  horizontalHeaderItem(3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  if (!p->m_info)
    return;

  Util::walkTree(*this, QModelIndex{}, [this](QModelIndex const &idx) {
    auto element = elementFromIndex(idx);
    if (element) {
      auto items = itemsForRow(idx);
      setItemsFromElement(items, *element);
    }
  });
}

void
Model::setItemsFromElement(QList<QStandardItem *> &items,
                           EbmlElement &element) {
  auto p    = p_func();
  auto name = kax_element_names_c::get(element);
  std::string content;

  if (name.empty())
    name = (boost::format(Y("Unknown (ID: 0x%1%)")) % p->m_info->format_ebml_id_as_hex(element)).str();

  else if (dynamic_cast<EbmlDummy *>(&element))
    name = (boost::format(Y("Known element, but invalid at this position: %1% (ID: 0x%2%)")) % name % p->m_info->format_ebml_id_as_hex(element)).str();

  else
    content = p->m_info->format_element_value(element);

  items[0]->setText(Q(name));
  items[1]->setText(Q(content));
  items[2]->setText(Q("%1").arg(element.GetElementPosition()));
  items[3]->setText(Q("%1").arg(element.HeadSize() + element.GetSize()));

  items[2]->setTextAlignment(Qt::AlignRight);
  items[3]->setTextAlignment(Qt::AlignRight);

  items[0]->setData(reinterpret_cast<qulonglong>(&element),          ElementRole);
  items[0]->setData(static_cast<qint64>(EbmlId(element).GetValue()), EbmlIdRole);
}

void
Model::reset() {
  auto p = p_func();

  beginResetModel();

  removeRows(0, rowCount());
  p->m_treeInsertionPosition.clear();
  p->m_treeInsertionPosition << invisibleRootItem();

  endResetModel();
}

void
Model::addElement(int level,
                  EbmlElement *element,
                  bool readFully) {
  auto p = p_func();

  if (!element)
    return;

  auto items = newItems();
  setItemsFromElement(items, *element);

  if (!readFully && dynamic_cast<EbmlMaster *>(element)) {
    items[0]->setData(true,  DeferredLoadRole);
    items[0]->setData(false, LoadedRole);
  }

  while (p->m_treeInsertionPosition.size() > (level + 1))
    p->m_treeInsertionPosition.removeLast();

  p->m_treeInsertionPosition.last()->appendRow(items);

  p->m_treeInsertionPosition << items[0];
}

void
Model::addElementStructure(QStandardItem &parent,
                           EbmlElement &element) {
  auto items = newItems();
  setItemsFromElement(items, element);

  parent.appendRow(items);

  auto master = dynamic_cast<EbmlMaster *>(&element);
  if (!master)
    return;

  for (auto child : *master)
    addElementStructure(*items[0], *child);
}

bool
Model::hasChildren(const QModelIndex &parent)
  const {
  if (!parent.isValid())
    return QStandardItemModel::hasChildren(parent);

  auto item = itemFromIndex(parent);
  if (!item->data(DeferredLoadRole).toBool())
    return QStandardItemModel::hasChildren(parent);

  if (!item->data(LoadedRole).toBool())
    return true;

  auto element = dynamic_cast<EbmlMaster *>(elementFromItem(*item));
  return element ? !!element->ListSize() : false;
}

void
Model::forgetLevel1ElementChildren(QModelIndex const &idx) {
  if (!idx.isValid())
    return;

  auto item = itemFromIndex(idx);
  if (!item->data(DeferredLoadRole).toBool() || !item->data(DeferredLoadRole).toBool())
    return;

  item->removeRows(0, item->rowCount());
  item->setData(false, LoadedRole);

  auto element = dynamic_cast<EbmlMaster *>(elementFromItem(*item));
  if (!element)
    return;

  for (auto child : *element)
    delete child;
  element->RemoveAll();
}

void
Model::addChildrenOfLevel1Element(QModelIndex const &idx) {
  if (!idx.isValid())
    return;

  auto element = elementFromIndex(idx);
  auto master  = dynamic_cast<EbmlMaster *>(element);
  auto parent  = itemFromIndex(idx);
  auto items   = itemsForRow(idx);

  if (element)
    setItemsFromElement(items, *element);

  if (!master)
    return;

  for (auto child : *master)
    addElementStructure(*parent, *child);
}

}}}