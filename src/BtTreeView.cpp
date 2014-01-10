/*
 * BtTreeView.cpp is part of Brewtarget and was written by Mik
 * Firestone (mikfire@gmail.com).  Copyright is granted to Philip G. Lee
 * (rocketman768@gmail.com), 2009-2013.
 *
 * Brewtarget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Brewtarget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <QDrag>
#include <QMenu>
#include <QDebug>
#include <QHeaderView>
#include "BtTreeView.h"
#include "BtTreeModel.h"
#include "BtTreeFilterProxyModel.h"
#include "database.h"
#include "recipe.h"
#include "equipment.h"
#include "fermentable.h"
#include "hop.h"
#include "misc.h"
#include "yeast.h"
#include "brewnote.h"
#include "style.h"

BtTreeView::BtTreeView(QWidget *parent) :
   QTreeView(parent)
{
   // Set some global properties that all the kids will use.
   setAllColumnsShowFocus(true);
   setContextMenuPolicy(Qt::CustomContextMenu);
   setRootIsDecorated(false);

   setDragEnabled(true);
   setAcceptDrops(true);
   setDropIndicatorShown(true);
   setSelectionMode(QAbstractItemView::ExtendedSelection);
//   setDragDropMode(QAbstractItemView::InternalMove);

}

BtTreeModel* BtTreeView::model()
{
   return _model;
}

bool BtTreeView::removeRow(const QModelIndex &index)
{
   QModelIndex modelIndex = filter->mapToSource(index);
   QModelIndex parent = _model->parent(modelIndex);
   int position       = modelIndex.row();

   return _model->removeRows(position,1,parent);
}

bool BtTreeView::isParent(const QModelIndex& parent, const QModelIndex& child)
{
   QModelIndex modelParent = filter->mapToSource(parent);
   QModelIndex modelChild = filter->mapToSource(child);
   return modelParent == _model->parent(modelChild);
}

QModelIndex BtTreeView::parent(const QModelIndex& child)
{
   if ( ! child.isValid() )
      return QModelIndex();

   QModelIndex modelChild = filter->mapToSource(child);
   if ( modelChild.isValid())
      return filter->mapFromSource(_model->parent(modelChild));

   return QModelIndex();
}

QModelIndex BtTreeView::first()
{
   return filter->mapFromSource(_model->first());
}

Recipe* BtTreeView::recipe(const QModelIndex &index) const
{
   return _model->recipe(filter->mapToSource(index));
}

QString BtTreeView::folderName(QModelIndex index)
{
   if ( _model->type(filter->mapToSource(index)) == BtTreeItem::FOLDER)
      return _model->folder(filter->mapToSource(index))->fullPath();

   return _model->thing(filter->mapToSource(index))->folder();
}

QModelIndex BtTreeView::findElement(BeerXMLElement* thing)
{
   return filter->mapFromSource(_model->findElement(thing));
}

Equipment* BtTreeView::equipment(const QModelIndex &index) const
{
   return _model->equipment(filter->mapToSource(index));
}

Fermentable* BtTreeView::fermentable(const QModelIndex &index) const
{
   return _model->fermentable(filter->mapToSource(index));
}

Hop* BtTreeView::hop(const QModelIndex &index) const
{
   return _model->hop(filter->mapToSource(index));
}

Misc* BtTreeView::misc(const QModelIndex &index) const
{
   return _model->misc(filter->mapToSource(index));
}

Yeast* BtTreeView::yeast(const QModelIndex &index) const
{
   return _model->yeast(filter->mapToSource(index));
}

Style* BtTreeView::style(const QModelIndex &index) const
{
   return _model->style(filter->mapToSource(index));
}

BrewNote* BtTreeView::brewNote(const QModelIndex &index) const
{
   if ( ! index.isValid() ) 
      return NULL;

   return _model->brewNote(filter->mapToSource(index));
}

QModelIndex BtTreeView::findBrewNote(BrewNote* bNote)
{
   return filter->mapFromSource(_model->findBrewNote(bNote));
}

BtFolder* BtTreeView::folder(const QModelIndex &index) const
{
   if ( ! index.isValid() ) 
      return NULL;

   return _model->folder(filter->mapToSource(index));
}

QModelIndex BtTreeView::findFolder(BtFolder* folder)
{
   return filter->mapFromSource(_model->findFolder(folder->fullPath(), NULL, false));
}

void BtTreeView::addFolder(QString folder)
{
   _model->addFolder(folder);
}

void BtTreeView::renameFolder(BtFolder* victim, QString newName)
{
   _model->renameFolder(victim,newName);
}

int BtTreeView::type(const QModelIndex &index)
{
   return _model->type(filter->mapToSource(index));
}

void BtTreeView::mousePressEvent(QMouseEvent *event)
{
   if (event->button() == Qt::LeftButton)
   {
      dragStart = event->pos();
      doubleClick = false;
   }

   // Send the event on its way up to the parent
   QTreeView::mousePressEvent(event);
}

void BtTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
   if (event->button() == Qt::LeftButton)
      doubleClick = true;

   // Send the event on its way up to the parent
   QTreeView::mouseDoubleClickEvent(event);
}

void BtTreeView::mouseMoveEvent(QMouseEvent *event)
{
   // Return if the left button isn't down
   if (!(event->buttons() & Qt::LeftButton))
      return;

   // Return if the length of movement isn't far enough.
   if ((event->pos() - dragStart).manhattanLength() < QApplication::startDragDistance())
      return;

   if ( doubleClick )
      return;

   QDrag *drag = new QDrag(this);
   QMimeData *data = mimeData(selectionModel()->selectedRows());

   drag->setMimeData(data);
   drag->start(Qt::CopyAction);
} 

void BtTreeView::keyPressEvent(QKeyEvent *event)
{
   switch( event->key() )
   {
      case Qt::Key_Space:
      case Qt::Key_Select:
      case Qt::Key_Enter:
      case Qt::Key_Return:
         emit BtTreeView::doubleClicked(selectedIndexes().first());
         return;
   }
   QTreeView::keyPressEvent(event);
}

QMimeData* BtTreeView::mimeData(QModelIndexList indexes) 
{
   QMimeData *mimeData = new QMimeData();
   QByteArray encodedData;
   QString name = "";
   int _type, id, itsa;

   QDataStream stream(&encodedData, QIODevice::WriteOnly);

   // From what I've been able to tell, the drop events are homogenous -- a
   // single drop event will be all equipment or all recipe or ...
   itsa = -1;
   foreach (QModelIndex index, indexes)
   {

      if (! index.isValid())
         continue;

      _type = type(index);
      id   = _model->thing(filter->mapToSource(index))->key();
      // Save this for later reference
      if ( _type != BtTreeItem::FOLDER && itsa == -1 )
         itsa = _type;

      stream << _type << id;
   }

   // Recipes, equipment and styles get dropped on the recipe pane
   if ( itsa == BtTreeItem::RECIPE || itsa == BtTreeItem::STYLE || itsa == BtTreeItem::EQUIPMENT ) 
      name = "application/x-brewtarget-recipe";
   // Everything other than folders get dropped on the ingredients pane
   else if ( itsa != -1 )
      name = "application/x-brewtarget-ingredient";
   // folders will be handled by themselves.
   else
      name = "application/x-brewtarget-folder";

   mimeData->setData(name,encodedData);
   return mimeData;
}

bool BtTreeView::multiSelected()
{
   QModelIndexList selected = selectionModel()->selectedRows();
   bool hasRecipe, hasSomethingElse;

   hasRecipe        = false;
   hasSomethingElse = false;

   if ( selected.count() == 0 ) 
      return false;

   foreach (QModelIndex selection, selected)
   {
      QModelIndex selectModel = filter->mapToSource(selection);
      if (_model->isRecipe(selectModel))
         hasRecipe = true;
      else
         hasSomethingElse = true;
   }

   return hasRecipe && hasSomethingElse;
}

void BtTreeView::setupContextMenu(QWidget* top, QWidget* editor, QMenu *sMenu, QMenu *fMenu, int type)
{

   _contextMenu = new QMenu(this);
   subMenu = new QMenu(this);

   switch(type) 
   {
      // the recipe case is a bit more complex, because we need to handle the brewnotes too
      case BtTreeItem::RECIPE:
         _contextMenu->addAction(tr("New Recipe"), editor, SLOT(newRecipe()));
         _contextMenu->addAction(tr("Brew It!"), top, SLOT(newBrewNote()));
         _contextMenu->addSeparator();

         subMenu->addAction(tr("Brew Again"), top, SLOT(reBrewNote()));
         subMenu->addAction(tr("Change date"), top, SLOT(changeBrewDate()));
         subMenu->addAction(tr("Recalculate eff"), top, SLOT(fixBrewNote()));
         subMenu->addAction(tr("Delete"), top, SLOT(deleteSelected()));

         break;
      case BtTreeItem::EQUIPMENT:
         _contextMenu->addAction(tr("New Equipment"), editor, SLOT(newEquipment()));
         _contextMenu->addSeparator();
         break;
      case BtTreeItem::FERMENTABLE:
         _contextMenu->addAction(tr("New Fermentable"), editor, SLOT(newFermentable()));
         _contextMenu->addSeparator();
         break;
      case BtTreeItem::HOP:
         _contextMenu->addAction(tr("New Hop"), editor, SLOT(newHop()));
         _contextMenu->addSeparator();
         break;
      case BtTreeItem::MISC:
         _contextMenu->addAction(tr("New Misc"), editor, SLOT(newMisc()));
         _contextMenu->addSeparator();
         break;
      case BtTreeItem::STYLE:
         _contextMenu->addAction(tr("New Style"), editor, SLOT(newStyle()));
         _contextMenu->addSeparator();
         break;
      case BtTreeItem::YEAST:
         _contextMenu->addAction(tr("New Yeast"), editor, SLOT(newYeast()));
         _contextMenu->addSeparator();
         break;
   }

   // submenus for new and folders
   _contextMenu->addMenu(sMenu);
   _contextMenu->addMenu(fMenu);
   _contextMenu->addSeparator();

   // Copy
   _contextMenu->addAction(tr("Copy"), top, SLOT(copySelected()));
   // Delete
   _contextMenu->addAction(tr("Delete"), top, SLOT(deleteSelected()));
   // export and import
   _contextMenu->addSeparator();
   _contextMenu->addAction(tr("Export"), top, SLOT(exportSelected()));
   _contextMenu->addAction(tr("Import"), top, SLOT(importFiles()));
   
}

QMenu* BtTreeView::contextMenu(QModelIndex selected)
{
   if ( type(selected) == BtTreeItem::BREWNOTE )
      return subMenu;

   return _contextMenu;
}

// Bad form likely

RecipeTreeView::RecipeTreeView(QWidget *parent)
   : BtTreeView(parent)
{
   _model = new BtTreeModel(this, BtTreeModel::RECIPEMASK);
   filter = new BtTreeFilterProxyModel(this, BtTreeModel::RECIPEMASK);
   filter->setSourceModel(_model);
   setModel(filter);
   filter->setDynamicSortFilter(true);
   
   setExpanded(findElement(0), true);
   setSortingEnabled(true);
   sortByColumn(0,Qt::AscendingOrder);
   // Resizing before you set the model doesn't do much.
   resizeColumnToContents(0);
}

EquipmentTreeView::EquipmentTreeView(QWidget *parent)
   : BtTreeView(parent)
{
   _model = new BtTreeModel(this, BtTreeModel::EQUIPMASK);
   filter = new BtTreeFilterProxyModel(this, BtTreeModel::EQUIPMASK);
   filter->setSourceModel(_model);
   setModel(filter);
   filter->setDynamicSortFilter(true);

   setExpanded(findElement(0), true);
   setSortingEnabled(true);
   sortByColumn(0,Qt::AscendingOrder);
   resizeColumnToContents(0);
}

// Icky ick ikcy
FermentableTreeView::FermentableTreeView(QWidget *parent)
   : BtTreeView(parent)
{
   _model = new BtTreeModel(this, BtTreeModel::FERMENTMASK);
   filter = new BtTreeFilterProxyModel(this, BtTreeModel::FERMENTMASK);
   filter->setSourceModel(_model);
   setModel(filter);
   filter->setDynamicSortFilter(true);
  
   filter->dumpObjectInfo(); 
   setExpanded(findElement(0), true);
   setSortingEnabled(true);
   sortByColumn(0,Qt::AscendingOrder);
   resizeColumnToContents(0);
}

// More Ick
HopTreeView::HopTreeView(QWidget *parent)
   : BtTreeView(parent)
{
   _model = new BtTreeModel(this, BtTreeModel::HOPMASK);
   filter = new BtTreeFilterProxyModel(this, BtTreeModel::HOPMASK);
   filter->setSourceModel(_model);
   setModel(filter);
   filter->setDynamicSortFilter(true);
   
   setExpanded(findElement(0), true);
   setSortingEnabled(true);
   sortByColumn(0,Qt::AscendingOrder);
   resizeColumnToContents(0);
}

// Ick some more
MiscTreeView::MiscTreeView(QWidget *parent)
   : BtTreeView(parent)
{
   _model = new BtTreeModel(this, BtTreeModel::MISCMASK);
   filter = new BtTreeFilterProxyModel(this, BtTreeModel::MISCMASK);
   filter->setSourceModel(_model);
   setModel(filter);
   filter->setDynamicSortFilter(true);
   
   setExpanded(findElement(0), true);
   setSortingEnabled(true);
   sortByColumn(0,Qt::AscendingOrder);
   resizeColumnToContents(0);
}

// Will this ick never end?
YeastTreeView::YeastTreeView(QWidget *parent)
   : BtTreeView(parent)
{
   _model = new BtTreeModel(this, BtTreeModel::YEASTMASK);
   filter = new BtTreeFilterProxyModel(this, BtTreeModel::YEASTMASK);
   filter->setSourceModel(_model);
   setModel(filter);
   filter->setDynamicSortFilter(true);
   
   setExpanded(findElement(0), true);
   setSortingEnabled(true);
   sortByColumn(0,Qt::AscendingOrder);
   resizeColumnToContents(0);
}

// Nope. Apparently not, cause I keep adding more
StyleTreeView::StyleTreeView(QWidget *parent)
   : BtTreeView(parent)
{
   _model = new BtTreeModel(this, BtTreeModel::STYLEMASK);
   filter = new BtTreeFilterProxyModel(this, BtTreeModel::STYLEMASK);
   filter->setSourceModel(_model);
   setModel(filter);
   filter->setDynamicSortFilter(true);
   
   setExpanded(findElement(0), true);
   setSortingEnabled(true);
   sortByColumn(0,Qt::AscendingOrder);
   resizeColumnToContents(0);
}
